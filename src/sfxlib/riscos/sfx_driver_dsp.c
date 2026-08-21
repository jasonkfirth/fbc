/*
    FreeBASIC Sound Library support for RISC OS
    -------------------------------------------

    File: sfx_driver_dsp.c

    Purpose:

        Deliver sfxlib PCM output to RISC OS through UnixLib /dev/dsp.

    Responsibilities:

        - configure signed 16-bit mono or stereo PCM output
        - convert sfxlib floating-point samples to device PCM
        - write complete frame sequences to DigitalRenderer
        - start and stop the RISC OS background mixer worker
        - report device activity through shared sfxlib diagnostics

    This file intentionally does NOT contain:

        - mixer or synthesizer logic
        - direct DigitalRenderer SWI wrappers
        - SoundDMA hardware control
        - MIDI or capture support

    Backend model:

        GCCSDK UnixLib implements the OSS /dev/dsp interface using the open
        DigitalRenderer module.  Keeping that compatibility layer here avoids
        duplicating its buffering, module loading, and RISC OS version fixes.
*/

#include "../fb_sfx.h"
#include "../fb_sfx_driver.h"
#include "../fb_sfx_driver_diag.h"
#include "../fb_sfx_internal.h"
#include "fb_sfx_riscos.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <unistd.h>

/* ------------------------------------------------------------------------- */
/* Device state                                                              */
/* ------------------------------------------------------------------------- */

#define FB_SFX_RISCOS_DSP_PATH "/dev/dsp"
#define FB_SFX_RISCOS_DSP_FRAGMENTS 8
#define FB_SFX_RISCOS_DRENDER_FRAMES 512

static int g_riscos_dsp_fd = -1;
static int g_riscos_dsp_channels = FB_SFX_DEFAULT_CHANNELS;
static int g_riscos_dsp_rate = FB_SFX_DEFAULT_RATE;
static short *g_riscos_dsp_pcm;
static int g_riscos_dsp_pcm_capacity;

static int riscos_dspEnsurePcmCapacity(int samples)
{
    int next_capacity;
    short *next_buffer;

    if (samples <= g_riscos_dsp_pcm_capacity)
        return 0;

    if (samples <= 0 || samples > (INT_MAX / (int)sizeof(short)))
        return -1;

    next_capacity = (g_riscos_dsp_pcm_capacity > 0)
        ? g_riscos_dsp_pcm_capacity
        : 1024;

    while (next_capacity < samples)
    {
        if (next_capacity > (INT_MAX / 2))
            return -1;
        next_capacity <<= 1;
    }

    next_buffer = (short *)realloc(g_riscos_dsp_pcm,
        (size_t)next_capacity * sizeof(short));
    if (next_buffer == NULL)
        return -1;

    g_riscos_dsp_pcm = next_buffer;
    g_riscos_dsp_pcm_capacity = next_capacity;
    return 0;
}

/* ------------------------------------------------------------------------- */
/* OSS device configuration                                                  */
/* ------------------------------------------------------------------------- */

static int riscos_dspFragmentExponent(int bytes)
{
    int exponent;
    int size;

    exponent = 0;
    size = 1;
    while (size < bytes && exponent < 30)
    {
        size <<= 1;
        exponent++;
    }

    return exponent;
}

static int riscos_dspConfigure(int fd, int rate, int channels)
{
    int format;
    int fragment_bytes;
    int fragment_spec;
    int block_size;

    format = AFMT_S16_LE;
    if (ioctl(fd, SNDCTL_DSP_SETFMT, &format) < 0 ||
        format != AFMT_S16_LE)
    {
        return -1;
    }

    if (ioctl(fd, SNDCTL_DSP_CHANNELS, &channels) < 0 ||
        (channels != 1 && channels != 2))
    {
        return -1;
    }

    if (ioctl(fd, SNDCTL_DSP_SPEED, &rate) < 0 || rate <= 0)
        return -1;

    fragment_bytes = FB_SFX_RISCOS_DRENDER_FRAMES * channels *
        (int)sizeof(short);
    fragment_spec = (FB_SFX_RISCOS_DSP_FRAGMENTS << 16) |
        riscos_dspFragmentExponent(fragment_bytes);

    if (ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &fragment_spec) < 0)
        return -1;

    block_size = 0;
    if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &block_size) < 0 ||
        block_size <= 0)
    {
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
/* Driver lifecycle                                                          */
/* ------------------------------------------------------------------------- */

static void riscos_dspExit(void)
{
    fb_sfxRiscosWorkerStop();

    if (g_riscos_dsp_fd >= 0)
    {
        close(g_riscos_dsp_fd);
        g_riscos_dsp_fd = -1;
    }

    free(g_riscos_dsp_pcm);
    g_riscos_dsp_pcm = NULL;
    g_riscos_dsp_pcm_capacity = 0;
}

static int riscos_dspInit(int rate, int channels, int buffer_frames, int flags)
{
    int fd;

    (void)flags;

    if (g_riscos_dsp_fd >= 0)
        return 0;

    if (rate <= 0)
        rate = FB_SFX_DEFAULT_RATE;
    if (channels <= 0)
        channels = FB_SFX_DEFAULT_CHANNELS;

    /* DigitalRenderer accepts either mono or stereo streams. */
    if (channels != 1 && channels != 2)
        return -1;

    fd = open(FB_SFX_RISCOS_DSP_PATH, O_WRONLY);
    if (fd < 0)
    {
        SFX_DEBUG("riscos_dsp: open failed: %s", strerror(errno));
        return -1;
    }

    if (riscos_dspConfigure(fd, rate, channels) != 0)
    {
        SFX_DEBUG("riscos_dsp: device configuration failed: %s",
            strerror(errno));
        close(fd);
        return -1;
    }

    g_riscos_dsp_fd = fd;
    g_riscos_dsp_rate = rate;
    g_riscos_dsp_channels = channels;

    if (fb_sfxRiscosWorkerStart(buffer_frames) != 0)
    {
        SFX_DEBUG("riscos_dsp: could not start mixer worker");
        riscos_dspExit();
        return -1;
    }

    SFX_DEBUG("riscos_dsp: initialized rate=%d channels=%d",
        g_riscos_dsp_rate, g_riscos_dsp_channels);
    return 0;
}

/* ------------------------------------------------------------------------- */
/* PCM delivery                                                              */
/* ------------------------------------------------------------------------- */

static int riscos_dspWrite(const float *samples, int frames)
{
    const unsigned char *source;
    int bytes_written;
    int frame_bytes;
    int sample_count;
    int total_bytes;

    if (g_riscos_dsp_fd < 0 || samples == NULL || frames <= 0)
        return -1;

    if (frames > (INT_MAX / g_riscos_dsp_channels))
        return -1;

    sample_count = frames * g_riscos_dsp_channels;
    if (riscos_dspEnsurePcmCapacity(sample_count) != 0)
        return -1;

    fb_sfxDriverDiagnostics("RISC OS DigitalRenderer", samples, frames,
        g_riscos_dsp_channels);
    fb_sfxConvertFloatToS16(samples, g_riscos_dsp_pcm, sample_count);

    frame_bytes = g_riscos_dsp_channels * (int)sizeof(short);
    total_bytes = sample_count * (int)sizeof(short);
    bytes_written = 0;
    source = (const unsigned char *)g_riscos_dsp_pcm;

    while (bytes_written < total_bytes)
    {
        int result;

        result = (int)write(g_riscos_dsp_fd,
            source + bytes_written,
            (size_t)(total_bytes - bytes_written));
        if (result > 0)
        {
            bytes_written += result;
            continue;
        }

        if (result < 0 && errno == EINTR)
            continue;

        if (bytes_written > 0)
            return bytes_written / frame_bytes;

        SFX_DEBUG("riscos_dsp: write failed: %s", strerror(errno));
        return -1;
    }

    return frames;
}

static int riscos_dspDeviceList(void)
{
    return 1;
}

/* ------------------------------------------------------------------------- */
/* Driver registration                                                       */
/* ------------------------------------------------------------------------- */

const FB_SFX_DRIVER fb_sfxDriverRiscosDsp =
{
    "RISC OS DigitalRenderer",
    FB_SFX_DRIVER_CAP_BACKGROUND,
    riscos_dspInit,
    riscos_dspExit,
    riscos_dspWrite,
    NULL,
    NULL,
    riscos_dspDeviceList,
    NULL
};

/* end of sfx_driver_dsp.c */
