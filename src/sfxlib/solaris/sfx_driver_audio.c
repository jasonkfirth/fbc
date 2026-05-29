/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_audio.c

    Purpose:

        Implement Solaris/illumos playback through the traditional
        /dev/audio interface.

    Responsibilities:

        - open and configure the audio device
        - convert mixer float samples to signed 16-bit PCM
        - write blocking interleaved PCM frames to the device
        - reject null-device fallback during real platform smoke tests

    This file intentionally does NOT contain:

        - mixer logic
        - MIDI routing
        - X11 or console behavior

    Platform notes:

        illumos still exposes the Sun audio interface through audioio(7I).
        The driver asks for signed 16-bit native-endian linear PCM because
        that is the simplest format shared by the runtime mixer and the
        kernel audio layer.  The device path defaults to /dev/audio, but
        tests may override it with SFXLIB_SOLARIS_AUDIO_DEVICE.
*/

#include "fb_sfx_solaris.h"

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"
#include "../fb_sfx_driver_diag.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/audioio.h>
#include <sys/ioctl.h>

static int solaris_audio_fd = -1;
static int solaris_audio_channels = FB_SFX_DEFAULT_CHANNELS;
static int solaris_audio_sample_rate = FB_SFX_DEFAULT_RATE;

#define SOLARIS_AUDIO_WRITE_CHUNK_BYTES 512

static int solaris_audio_debug_enabled(void)
{
    const char *env = getenv("SFXLIB_DEBUG");
    if (env && *env && *env != '0')
        return 1;

    env = getenv("SFXLIB_SOLARIS_DEBUG");
    return (env && *env && *env != '0');
}

#define SOLARIS_AUDIO_DBG(...) \
    do { \
        if (solaris_audio_debug_enabled()) { \
            fprintf(stderr, "SFX_SOLARIS_AUDIO: "); \
            fprintf(stderr, __VA_ARGS__); \
        } \
    } while (0)

static const char *solaris_audio_device_path(void)
{
    const char *path;

    path = getenv("SFXLIB_SOLARIS_AUDIO_DEVICE");
    if (path && *path)
        return path;

    path = getenv("SFXLIB_AUDIO_DEVICE");
    if (path && *path)
        return path;

    return "/dev/audio";
}

static void solaris_audio_sleep_for_frames(int frames)
{
    struct timespec req;
    unsigned long long nanoseconds;

    if (frames <= 0 || solaris_audio_sample_rate <= 0)
        return;

    /*
        /dev/audio writes are opened non-blocking so the ES1370/audiopci path
        used by QEMU can never trap a process in the kernel.  That means this
        driver must provide the normal playback pacing itself; otherwise the
        background feeder can spin through buffers faster than the emulated
        device drains them.
    */
    nanoseconds = ((unsigned long long)frames * 1000000000ULL) /
                  (unsigned long long)solaris_audio_sample_rate;

    if (nanoseconds == 0)
        nanoseconds = 1000000ULL;

    req.tv_sec = (time_t)(nanoseconds / 1000000000ULL);
    req.tv_nsec = (long)(nanoseconds % 1000000000ULL);

    while (nanosleep(&req, &req) != 0 && errno == EINTR)
        ;
}

static void solaris_audio_close(void)
{
    if (solaris_audio_fd < 0)
        return;

    /*
        Do not call AUDIO_DRAIN here.  Real devices drain naturally on close,
        while the illumos audiopci driver used with QEMU ES1370 can wait
        indefinitely even after all queued samples have already reached the
        backend.  Shutdown must never trap a BASIC program in process exit.
    */

    close(solaris_audio_fd);
    solaris_audio_fd = -1;
    solaris_audio_channels = FB_SFX_DEFAULT_CHANNELS;
    solaris_audio_sample_rate = FB_SFX_DEFAULT_RATE;
}

static int solaris_audio_configure(int fd,
                                   int rate,
                                   int channels,
                                   int *actual_rate_out,
                                   int *actual_channels_out)
{
    audio_info_t info;
    int requested_rate;
    int requested_channels;

    requested_rate = (rate > 0) ? rate : FB_SFX_DEFAULT_RATE;
    requested_channels = (channels > 0) ? channels : FB_SFX_DEFAULT_CHANNELS;

    AUDIO_INITINFO(&info);
    info.play.sample_rate = requested_rate;
    info.play.channels = requested_channels;
    info.play.precision = 16;
    info.play.encoding = AUDIO_ENCODING_LINEAR;
    info.play.pause = 0;

    if (ioctl(fd, AUDIO_SETINFO, &info) < 0)
    {
        SOLARIS_AUDIO_DBG("AUDIO_SETINFO failed: %s\n", strerror(errno));
        return -1;
    }

    if (ioctl(fd, AUDIO_GETINFO, &info) < 0)
    {
        SOLARIS_AUDIO_DBG("AUDIO_GETINFO failed: %s\n", strerror(errno));
        return -1;
    }

    if (info.play.precision != 16 || info.play.encoding != AUDIO_ENCODING_LINEAR)
    {
        SOLARIS_AUDIO_DBG("device did not accept signed 16-bit linear PCM\n");
        return -1;
    }

    if (info.play.channels <= 0 || info.play.sample_rate <= 0)
    {
        SOLARIS_AUDIO_DBG("device reported invalid format\n");
        return -1;
    }

    *actual_rate_out = info.play.sample_rate;
    *actual_channels_out = info.play.channels;
    return 0;
}

static int solaris_audio_init(int rate, int channels, int buffer_frames, int flags)
{
    int fd;
    int actual_rate;
    int actual_channels;
    const char *device_path;

    (void)flags;

    if (solaris_audio_fd >= 0)
        return 0;

    if (fb_sfxSolarisInit() != 0)
        return -1;

    device_path = solaris_audio_device_path();
    fd = open(device_path, O_WRONLY | O_NONBLOCK);
    if (fd < 0)
    {
        SOLARIS_AUDIO_DBG("open %s failed: %s\n", device_path, strerror(errno));
        return -1;
    }

    if (solaris_audio_configure(fd, rate, channels, &actual_rate, &actual_channels) != 0)
    {
        close(fd);
        return -1;
    }

    if (fb_sfxSolarisActivate(actual_rate, actual_channels, buffer_frames) != 0)
    {
        close(fd);
        return -1;
    }

    solaris_audio_fd = fd;
    solaris_audio_channels = actual_channels;
    solaris_audio_sample_rate = actual_rate;

    SOLARIS_AUDIO_DBG("opened %s rate=%d channels=%d buffer=%d\n",
                      device_path,
                      actual_rate,
                      actual_channels,
                      buffer_frames);

    return 0;
}

static void solaris_audio_exit(void)
{
    fb_sfxSolarisDeactivate();
    solaris_audio_close();
    fb_sfxSolarisExit();
}

static int solaris_audio_write(const float *buffer, int frames)
{
    short *pcm;
    int samples;
    int bytes_total;
    int bytes_written;
    int frame_bytes;

    if (solaris_audio_fd < 0 || !buffer || frames <= 0)
        return -1;

    samples = frames * solaris_audio_channels;
    bytes_total = samples * (int)sizeof(short);
    frame_bytes = solaris_audio_channels * (int)sizeof(short);

    pcm = (short *)malloc((size_t)bytes_total);
    if (!pcm)
        return -1;

    fb_sfxDriverDiagnostics("Solaris audio",
                            buffer,
                            frames,
                            solaris_audio_channels);
    fb_sfxConvertFloatToS16(buffer, pcm, samples);

    bytes_written = 0;
    while (bytes_written < bytes_total)
    {
        ssize_t written;
        int request_bytes;

        /*
            illumos hardware drivers generally accept large writes, but the
            QEMU ES1370/audiopci path can block indefinitely when handed a
            full mixer buffer in one call.  Small frame-aligned writes keep
            both real devices and virtual test hardware draining steadily.
        */
        request_bytes = bytes_total - bytes_written;
        if (request_bytes > SOLARIS_AUDIO_WRITE_CHUNK_BYTES)
            request_bytes = SOLARIS_AUDIO_WRITE_CHUNK_BYTES;

        request_bytes -= request_bytes % frame_bytes;
        if (request_bytes <= 0)
            request_bytes = frame_bytes;

        written = write(solaris_audio_fd,
                        ((const char *)pcm) + bytes_written,
                        (size_t)request_bytes);

        if (written < 0)
        {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                SOLARIS_AUDIO_DBG("device backpressure, dropping %d bytes\n",
                                  bytes_total - bytes_written);
                free(pcm);
                solaris_audio_sleep_for_frames(frames);
                return frames;
            }

            SOLARIS_AUDIO_DBG("write failed: %s\n", strerror(errno));
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

    frames = bytes_written / frame_bytes;
    solaris_audio_sleep_for_frames(frames);
    return frames;
}

const FB_SFX_DRIVER fb_sfxDriverSolarisAudio =
{
    "Solaris audio",
    FB_SFX_DRIVER_CAP_BACKGROUND | FB_SFX_DRIVER_CAP_BLOCKING,
    solaris_audio_init,
    solaris_audio_exit,
    solaris_audio_write,
    NULL,
    NULL,
    NULL,
    NULL
};

#undef SOLARIS_AUDIO_WRITE_CHUNK_BYTES
#undef SOLARIS_AUDIO_DBG

/* end of sfx_driver_audio.c */
