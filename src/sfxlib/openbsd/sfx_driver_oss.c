/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_oss.c

    Purpose:

        Implement OpenBSD playback through sndio.

    Responsibilities:

        - open the selected sndio playback device
        - configure signed 16-bit native-endian playback
        - convert mixer floats to PCM frames for sndio
        - read signed 16-bit native-endian recording frames from sndio

    This file intentionally does NOT contain:

        - OSS compatibility code
        - mixer or synthesis logic
        - MIDI support

    OpenBSD audio model:

        OpenBSD applications normally talk to the sndio API instead of
        opening /dev/audio directly.  sndio can route through the sndiod
        server or to a raw device name such as rsnd/0.  The VM tests set
        SFXLIB_OPENBSD_SNDIO_DEVICE=rsnd/0 so the test proves the driver
        can feed a real emulated audio device even if sndiod is not
        already running.
*/

#ifndef DISABLE_OPENBSD

#include "fb_sfx_openbsd.h"

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"
#include "../fb_sfx_driver_diag.h"

#include <errno.h>
#include <sndio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct sio_hdl *openbsd_sio = NULL;
static struct sio_hdl *openbsd_sio_capture = NULL;
static int openbsd_sndio_initialized = 0;
static int openbsd_sndio_channels = FB_SFX_DEFAULT_CHANNELS;
static int openbsd_sndio_capture_channels = FB_SFX_DEFAULT_CHANNELS;

static int openbsd_sndio_debug_enabled(void)
{
    const char *env;

    env = getenv("SFXLIB_OPENBSD_DEBUG");
    if (env && *env && *env != '0')
        return 1;

    env = getenv("SFXLIB_SNDIO_DEBUG");
    return (env && *env && *env != '0');
}

#define OPENBSD_SNDIO_DBG(...) \
    do { \
        if (openbsd_sndio_debug_enabled()) { \
            fprintf(stderr, "SFX_OPENBSD_SNDIO: "); \
            fprintf(stderr, __VA_ARGS__); \
        } \
    } while (0)

static const char *openbsd_sndio_device_name(void)
{
    const char *device;

    device = getenv("SFXLIB_OPENBSD_SNDIO_DEVICE");
    if (device && *device)
        return device;

    device = getenv("SFXLIB_SNDIO_DEVICE");
    if (device && *device)
        return device;

    return SIO_DEVANY;
}

static const char *openbsd_sndio_capture_device_name(void)
{
    const char *device;

    device = getenv("SFXLIB_OPENBSD_SNDIO_CAPTURE_DEVICE");
    if (device && *device)
        return device;

    device = getenv("SFXLIB_SNDIO_CAPTURE_DEVICE");
    if (device && *device)
        return device;

    return openbsd_sndio_device_name();
}

static int openbsd_sndio_init(int rate, int channels, int buffer_frames, int flags)
{
    struct sio_par par;
    struct sio_hdl *hdl;
    const char *device;
    unsigned int requested_channels;

    (void)flags;

    if (openbsd_sndio_initialized)
        return 0;

    if (fb_sfxOpenbsdInit() != 0)
        return -1;

    device = openbsd_sndio_device_name();
    hdl = sio_open(device, SIO_PLAY, 0);
    if (!hdl)
    {
        OPENBSD_SNDIO_DBG("sio_open %s failed\n", device);
        return -1;
    }

    requested_channels = (unsigned int)((channels > 0) ? channels : FB_SFX_DEFAULT_CHANNELS);

    sio_initpar(&par);
    par.bits = 16;
    par.sig = 1;
    par.le = SIO_LE_NATIVE;
    par.pchan = requested_channels;
    par.rate = (unsigned int)((rate > 0) ? rate : FB_SFX_DEFAULT_RATE);
    par.appbufsz = (unsigned int)((buffer_frames > 0) ? buffer_frames : FB_SFX_DEFAULT_BUFFER);

    if (!sio_setpar(hdl, &par) || !sio_getpar(hdl, &par))
    {
        OPENBSD_SNDIO_DBG("failed to configure %s\n", device);
        sio_close(hdl);
        return -1;
    }

    if (par.bits != 16 || par.sig == 0 || par.le != SIO_LE_NATIVE ||
        par.pchan != requested_channels)
    {
        OPENBSD_SNDIO_DBG("unsupported format bits=%u sig=%u le=%u channels=%u\n",
                          par.bits,
                          par.sig,
                          par.le,
                          par.pchan);
        sio_close(hdl);
        return -1;
    }

    if (!sio_start(hdl))
    {
        OPENBSD_SNDIO_DBG("sio_start failed on %s\n", device);
        sio_close(hdl);
        return -1;
    }

    if (fb_sfxOpenbsdActivate((int)par.rate, (int)par.pchan, (int)par.appbufsz) != 0)
    {
        sio_close(hdl);
        return -1;
    }

    openbsd_sio = hdl;
    openbsd_sndio_initialized = 1;
    openbsd_sndio_channels = (int)par.pchan;

    OPENBSD_SNDIO_DBG("opened %s rate=%u channels=%u appbuf=%u\n",
                      device,
                      par.rate,
                      par.pchan,
                      par.appbufsz);

    return 0;
}

static void openbsd_sndio_exit(void)
{
    if (!openbsd_sndio_initialized)
        return;

    fb_sfxOpenbsdDeactivate();

    if (openbsd_sio)
    {
        sio_close(openbsd_sio);
        openbsd_sio = NULL;
    }

    fb_sfxOpenbsdExit();
    openbsd_sndio_initialized = 0;
    openbsd_sndio_channels = FB_SFX_DEFAULT_CHANNELS;
}

static int openbsd_sndio_write(const float *buffer, int frames)
{
    short *pcm;
    int samples;
    int bytes_total;
    int bytes_written;
    int frame_bytes;

    if (!openbsd_sio || !buffer || frames <= 0)
        return -1;

    samples = frames * openbsd_sndio_channels;
    bytes_total = samples * (int)sizeof(short);
    frame_bytes = openbsd_sndio_channels * (int)sizeof(short);

    pcm = (short *)malloc((size_t)bytes_total);
    if (!pcm)
        return -1;

    fb_sfxDriverDiagnostics("OpenBSD sndio",
                            buffer,
                            frames,
                            openbsd_sndio_channels);
    fb_sfxConvertFloatToS16(buffer, pcm, samples);

    bytes_written = 0;
    while (bytes_written < bytes_total)
    {
        size_t written;

        written = sio_write(openbsd_sio,
                            ((const char *)pcm) + bytes_written,
                            (size_t)(bytes_total - bytes_written));
        if (written == 0)
            break;

        bytes_written += (int)written;
    }

    free(pcm);

    if (bytes_written <= 0)
        return 0;

    return bytes_written / frame_bytes;
}

int fb_sfxPlatformCaptureStart(void)
{
    struct sio_par par;
    struct sio_hdl *hdl;
    const char *device;
    unsigned int requested_channels;
    unsigned int requested_rate;

    if (openbsd_sio_capture)
        return 0;

    if (fb_sfxOpenbsdInit() != 0)
        return -1;

    requested_channels = FB_SFX_DEFAULT_CHANNELS;
    requested_rate = FB_SFX_DEFAULT_RATE;
    if (__fb_sfx)
    {
        if (__fb_sfx->capture.channels > 0)
            requested_channels = (unsigned int)__fb_sfx->capture.channels;
        else if (__fb_sfx->output_channels > 0)
            requested_channels = (unsigned int)__fb_sfx->output_channels;

        if (__fb_sfx->capture.rate > 0)
            requested_rate = (unsigned int)__fb_sfx->capture.rate;
        else if (__fb_sfx->samplerate > 0)
            requested_rate = (unsigned int)__fb_sfx->samplerate;
    }

    device = openbsd_sndio_capture_device_name();
    hdl = sio_open(device, SIO_REC, 1);
    if (!hdl)
    {
        OPENBSD_SNDIO_DBG("sio_open capture %s failed\n", device);
        if (!fb_sfxOpenbsdRunning())
            fb_sfxOpenbsdExit();
        return -1;
    }

    sio_initpar(&par);
    par.bits = 16;
    par.sig = 1;
    par.le = SIO_LE_NATIVE;
    par.rchan = requested_channels;
    par.rate = requested_rate;
    par.appbufsz = FB_SFX_DEFAULT_BUFFER;

    if (!sio_setpar(hdl, &par) || !sio_getpar(hdl, &par))
    {
        OPENBSD_SNDIO_DBG("failed to configure capture %s\n", device);
        sio_close(hdl);
        if (!fb_sfxOpenbsdRunning())
            fb_sfxOpenbsdExit();
        return -1;
    }

    if (par.bits != 16 || par.sig == 0 || par.le != SIO_LE_NATIVE ||
        par.rchan != requested_channels)
    {
        OPENBSD_SNDIO_DBG("unsupported capture format bits=%u sig=%u le=%u channels=%u\n",
                          par.bits,
                          par.sig,
                          par.le,
                          par.rchan);
        sio_close(hdl);
        if (!fb_sfxOpenbsdRunning())
            fb_sfxOpenbsdExit();
        return -1;
    }

    if (!sio_start(hdl))
    {
        OPENBSD_SNDIO_DBG("sio_start capture failed on %s\n", device);
        sio_close(hdl);
        if (!fb_sfxOpenbsdRunning())
            fb_sfxOpenbsdExit();
        return -1;
    }

    openbsd_sio_capture = hdl;
    openbsd_sndio_capture_channels = (int)par.rchan;

    if (__fb_sfx)
    {
        __fb_sfx->capture.rate = (int)par.rate;
        __fb_sfx->capture.channels = (int)par.rchan;
        fb_sfxCaptureBufferClear();
    }

    OPENBSD_SNDIO_DBG("opened capture %s rate=%u channels=%u appbuf=%u\n",
                      device,
                      par.rate,
                      par.rchan,
                      par.appbufsz);

    return 0;
}

void fb_sfxPlatformCaptureStop(void)
{
    if (openbsd_sio_capture)
    {
        sio_close(openbsd_sio_capture);
        openbsd_sio_capture = NULL;
    }

    openbsd_sndio_capture_channels = FB_SFX_DEFAULT_CHANNELS;

    if (!fb_sfxOpenbsdRunning())
        fb_sfxOpenbsdExit();
}

int fb_sfxPlatformCaptureRead(float *buffer, int frames)
{
    short *pcm;
    int frame_bytes;
    int bytes_total;
    int bytes_read;
    int frames_read;
    int samples_read;

    if (!openbsd_sio_capture || !buffer || frames <= 0)
        return -1;

    frame_bytes = openbsd_sndio_capture_channels * (int)sizeof(short);
    bytes_total = frames * frame_bytes;

    pcm = (short *)malloc((size_t)bytes_total);
    if (!pcm)
        return -1;

    bytes_read = 0;
    while (bytes_read < bytes_total)
    {
        size_t got;

        got = sio_read(openbsd_sio_capture,
                       ((char *)pcm) + bytes_read,
                       (size_t)(bytes_total - bytes_read));
        if (got == 0)
            break;

        bytes_read += (int)got;
    }

    if (bytes_read <= 0)
    {
        free(pcm);
        return 0;
    }

    bytes_read -= bytes_read % frame_bytes;
    frames_read = bytes_read / frame_bytes;
    samples_read = frames_read * openbsd_sndio_capture_channels;
    fb_sfxConvertS16ToFloat(pcm, buffer, samples_read);
    free(pcm);

    return frames_read;
}

const FB_SFX_DRIVER fb_sfxDriverOpenbsdOss =
{
    "OpenBSD sndio",
    FB_SFX_DRIVER_CAP_BACKGROUND,
    openbsd_sndio_init,
    openbsd_sndio_exit,
    openbsd_sndio_write,
    NULL,
    NULL,
    NULL,
    NULL
};

#undef OPENBSD_SNDIO_DBG

#endif

/* end of sfx_driver_oss.c */
