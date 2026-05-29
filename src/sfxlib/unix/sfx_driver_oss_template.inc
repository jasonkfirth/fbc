/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_oss_template.inc

    Purpose:

        Implement the shared OSS-style playback driver used by BSD
        targets that expose a /dev/dsp compatible device.

    Responsibilities:

        - open and configure the OSS playback device
        - convert the mixer float stream to signed 16-bit PCM
        - write blocking interleaved PCM frames to the device
        - read non-blocking signed 16-bit capture frames when OSS provides
          an input device

    This file intentionally does NOT contain:

        - target-specific driver registration
        - sndio/CoreAudio/ALSA/PulseAudio support
        - mixer or synthesis logic

    Device model:

        FreeBSD, NetBSD, and DragonFly expose OSS-compatible playback
        devices through /dev/dsp.  The runtime mixer remains platform
        neutral and hands each driver interleaved float samples.  This
        driver requests native-endian signed 16-bit PCM from OSS and
        performs the conversion immediately before writing.
*/

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"
#include "../fb_sfx_driver_diag.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/soundcard.h>

#ifndef FB_SFX_BSD_LABEL
#error "FB_SFX_BSD_LABEL must be defined before including sfx_driver_oss_template.inc"
#endif

#ifndef FB_SFX_BSD_DRIVER_NAME
#error "FB_SFX_BSD_DRIVER_NAME must be defined before including sfx_driver_oss_template.inc"
#endif

#ifndef FB_SFX_BSD_INIT_FN
#error "FB_SFX_BSD_INIT_FN must be defined before including sfx_driver_oss_template.inc"
#endif

#ifndef FB_SFX_BSD_EXIT_FN
#error "FB_SFX_BSD_EXIT_FN must be defined before including sfx_driver_oss_template.inc"
#endif

#ifndef FB_SFX_BSD_ACTIVATE_FN
#error "FB_SFX_BSD_ACTIVATE_FN must be defined before including sfx_driver_oss_template.inc"
#endif

#ifndef FB_SFX_BSD_DEACTIVATE_FN
#error "FB_SFX_BSD_DEACTIVATE_FN must be defined before including sfx_driver_oss_template.inc"
#endif

#ifndef FB_SFX_BSD_RUNNING_FN
#error "FB_SFX_BSD_RUNNING_FN must be defined before including sfx_driver_oss_template.inc"
#endif

static int bsd_oss_debug_enabled(void)
{
    const char *env = getenv("SFXLIB_DEBUG");
    return (env && *env && *env != '0');
}

#define BSD_OSS_DBG(...) \
    do { \
        if (bsd_oss_debug_enabled()) { \
            fprintf(stderr, "SFX_%s_OSS: ", FB_SFX_BSD_LABEL); \
            fprintf(stderr, __VA_ARGS__); \
        } \
    } while (0)

static int bsd_oss_fd = -1;
static int bsd_oss_channels = FB_SFX_DEFAULT_CHANNELS;
static int bsd_oss_capture_fd = -1;
static int bsd_oss_capture_channels = FB_SFX_DEFAULT_CHANNELS;

static const char *bsd_oss_device_path(void)
{
    const char *path;

    path = getenv("SFXLIB_OSS_DEVICE");
    if (path && *path)
        return path;

    path = getenv("SFXLIB_BSD_OSS_DEVICE");
    if (path && *path)
        return path;

    return "/dev/dsp";
}

static const char *bsd_oss_capture_device_path(void)
{
    const char *path;

    path = getenv("SFXLIB_OSS_CAPTURE_DEVICE");
    if (path && *path)
        return path;

    path = getenv("SFXLIB_BSD_OSS_CAPTURE_DEVICE");
    if (path && *path)
        return path;

    return bsd_oss_device_path();
}

static int bsd_oss_native_s16_format(void)
{
#if defined(AFMT_S16_NE)
    return AFMT_S16_NE;
#elif defined(BYTE_ORDER) && defined(BIG_ENDIAN) && (BYTE_ORDER == BIG_ENDIAN) && defined(AFMT_S16_BE)
    return AFMT_S16_BE;
#elif defined(AFMT_S16_LE)
    return AFMT_S16_LE;
#else
#error "No native signed 16-bit OSS audio format is available"
#endif
}

static float bsd_oss_s16_to_float(short value)
{
    if (value <= -32768)
        return -1.0f;

    return (float)value / 32767.0f;
}

static void bsd_oss_convert_s16_to_float(const short *src, float *dst, int samples)
{
    int i;

    for (i = 0; i < samples; ++i)
        dst[i] = bsd_oss_s16_to_float(src[i]);
}

static void bsd_oss_close_device(void)
{
    if (bsd_oss_fd < 0)
        return;

#ifdef SNDCTL_DSP_SYNC
    ioctl(bsd_oss_fd, SNDCTL_DSP_SYNC, 0);
#endif

    close(bsd_oss_fd);
    bsd_oss_fd = -1;
}

static void bsd_oss_close_capture_device(void)
{
    if (bsd_oss_capture_fd < 0)
        return;

    close(bsd_oss_capture_fd);
    bsd_oss_capture_fd = -1;
    bsd_oss_capture_channels = FB_SFX_DEFAULT_CHANNELS;
}

static int bsd_oss_configure_device(int fd,
                                    int rate,
                                    int channels,
                                    const char *device_path,
                                    const char *direction,
                                    int *actual_rate_out,
                                    int *actual_channels_out)
{
    int requested_format;
    int actual_format;
    int actual_channels;
    int actual_rate;

    requested_format = bsd_oss_native_s16_format();
    actual_format = requested_format;
    if (ioctl(fd, SNDCTL_DSP_SETFMT, &actual_format) < 0 ||
        actual_format != requested_format)
    {
        BSD_OSS_DBG("failed to select signed 16-bit %s format on %s\n",
                    direction,
                    device_path);
        return -1;
    }

    actual_channels = (channels > 0) ? channels : FB_SFX_DEFAULT_CHANNELS;
    if (ioctl(fd, SNDCTL_DSP_CHANNELS, &actual_channels) < 0 ||
        actual_channels != ((channels > 0) ? channels : FB_SFX_DEFAULT_CHANNELS))
    {
        BSD_OSS_DBG("failed to select %d %s channel(s) on %s\n",
                    (channels > 0) ? channels : FB_SFX_DEFAULT_CHANNELS,
                    direction,
                    device_path);
        return -1;
    }

    actual_rate = (rate > 0) ? rate : FB_SFX_DEFAULT_RATE;
    if (ioctl(fd, SNDCTL_DSP_SPEED, &actual_rate) < 0 || actual_rate <= 0)
    {
        BSD_OSS_DBG("failed to select %s rate on %s\n",
                    direction,
                    device_path);
        return -1;
    }

    if (actual_rate_out)
        *actual_rate_out = actual_rate;
    if (actual_channels_out)
        *actual_channels_out = actual_channels;

    return 0;
}

static int bsd_oss_init(int rate, int channels, int buffer_frames, int flags)
{
    int fd;
    int actual_channels;
    int actual_rate;
    const char *device_path;

    (void)flags;

    if (bsd_oss_fd >= 0)
        return 0;

    if (FB_SFX_BSD_INIT_FN() != 0)
        return -1;

    device_path = bsd_oss_device_path();
    fd = open(device_path, O_WRONLY);
    if (fd < 0)
    {
        BSD_OSS_DBG("open %s failed: %s\n", device_path, strerror(errno));
        return -1;
    }

    if (bsd_oss_configure_device(fd,
                                 rate,
                                 channels,
                                 device_path,
                                 "playback",
                                 &actual_rate,
                                 &actual_channels) != 0)
    {
        close(fd);
        return -1;
    }

    if (FB_SFX_BSD_ACTIVATE_FN(actual_rate, actual_channels, buffer_frames) != 0)
    {
        close(fd);
        return -1;
    }

    bsd_oss_fd = fd;
    bsd_oss_channels = actual_channels;

    BSD_OSS_DBG("opened %s rate=%d channels=%d buffer=%d\n",
                device_path,
                actual_rate,
                actual_channels,
                buffer_frames);

    return 0;
}

static void bsd_oss_exit(void)
{
    FB_SFX_BSD_DEACTIVATE_FN();
    bsd_oss_close_device();
    FB_SFX_BSD_EXIT_FN();
}

static int bsd_oss_write(const float *buffer, int frames)
{
    short *pcm;
    int samples;
    int bytes_total;
    int bytes_written;
    int frame_bytes;

    if (bsd_oss_fd < 0 || !buffer || frames <= 0)
        return -1;

    samples = frames * bsd_oss_channels;
    bytes_total = samples * (int)sizeof(short);
    frame_bytes = bsd_oss_channels * (int)sizeof(short);

    pcm = (short *)malloc((size_t)bytes_total);
    if (!pcm)
        return -1;

    fb_sfxDriverDiagnostics(FB_SFX_BSD_LABEL " OSS",
                            buffer,
                            frames,
                            bsd_oss_channels);
    fb_sfxConvertFloatToS16(buffer, pcm, samples);

    bytes_written = 0;
    while (bytes_written < bytes_total)
    {
        ssize_t written;

        written = write(bsd_oss_fd,
                        ((const char *)pcm) + bytes_written,
                        (size_t)(bytes_total - bytes_written));

        if (written < 0)
        {
            if (errno == EINTR)
                continue;

            BSD_OSS_DBG("write failed: %s\n", strerror(errno));
            free(pcm);
            return -1;
        }

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
    int fd;
    int requested_channels;
    int requested_rate;
    int actual_channels;
    int actual_rate;
    const char *device_path;

    if (bsd_oss_capture_fd >= 0)
        return 0;

    if (FB_SFX_BSD_INIT_FN() != 0)
        return -1;

    requested_channels = FB_SFX_DEFAULT_CHANNELS;
    requested_rate = FB_SFX_DEFAULT_RATE;
    if (__fb_sfx)
    {
        if (__fb_sfx->capture.channels > 0)
            requested_channels = __fb_sfx->capture.channels;
        else if (__fb_sfx->output_channels > 0)
            requested_channels = __fb_sfx->output_channels;

        if (__fb_sfx->capture.rate > 0)
            requested_rate = __fb_sfx->capture.rate;
        else if (__fb_sfx->samplerate > 0)
            requested_rate = __fb_sfx->samplerate;
    }

    device_path = bsd_oss_capture_device_path();
    fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        BSD_OSS_DBG("open capture %s failed: %s\n", device_path, strerror(errno));
        if (!FB_SFX_BSD_RUNNING_FN())
            FB_SFX_BSD_EXIT_FN();
        return -1;
    }

    if (bsd_oss_configure_device(fd,
                                 requested_rate,
                                 requested_channels,
                                 device_path,
                                 "capture",
                                 &actual_rate,
                                 &actual_channels) != 0)
    {
        close(fd);
        if (!FB_SFX_BSD_RUNNING_FN())
            FB_SFX_BSD_EXIT_FN();
        return -1;
    }

    bsd_oss_capture_fd = fd;
    bsd_oss_capture_channels = actual_channels;

    if (__fb_sfx)
    {
        __fb_sfx->capture.rate = actual_rate;
        __fb_sfx->capture.channels = actual_channels;
        fb_sfxCaptureBufferClear();
    }

    BSD_OSS_DBG("opened capture %s rate=%d channels=%d\n",
                device_path,
                actual_rate,
                actual_channels);

    return 0;
}

void fb_sfxPlatformCaptureStop(void)
{
    bsd_oss_close_capture_device();

    if (!FB_SFX_BSD_RUNNING_FN())
        FB_SFX_BSD_EXIT_FN();
}

int fb_sfxPlatformCaptureRead(float *buffer, int frames)
{
    short *pcm;
    int frame_bytes;
    int bytes_total;
    int bytes_read;
    int frames_read;
    int samples_read;

    if (bsd_oss_capture_fd < 0 || !buffer || frames <= 0)
        return -1;

    frame_bytes = bsd_oss_capture_channels * (int)sizeof(short);
    bytes_total = frames * frame_bytes;

    pcm = (short *)malloc((size_t)bytes_total);
    if (!pcm)
        return -1;

    bytes_read = 0;
    while (bytes_read < bytes_total)
    {
        ssize_t got;

        got = read(bsd_oss_capture_fd,
                   ((char *)pcm) + bytes_read,
                   (size_t)(bytes_total - bytes_read));

        if (got < 0)
        {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            BSD_OSS_DBG("capture read failed: %s\n", strerror(errno));
            free(pcm);
            return -1;
        }

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
    samples_read = frames_read * bsd_oss_capture_channels;
    bsd_oss_convert_s16_to_float(pcm, buffer, samples_read);
    free(pcm);

    return frames_read;
}

const FB_SFX_DRIVER FB_SFX_BSD_DRIVER_NAME =
{
    FB_SFX_BSD_LABEL " OSS",
    FB_SFX_DRIVER_CAP_BACKGROUND,
    bsd_oss_init,
    bsd_oss_exit,
    bsd_oss_write,
    NULL,
    NULL,
    NULL,
    NULL
};

#undef BSD_OSS_DBG
