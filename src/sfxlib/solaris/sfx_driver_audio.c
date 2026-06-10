/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_audio.c

    Purpose:

        Implement Solaris/illumos playback through the traditional
        /dev/audio interface.

    Responsibilities:

        - open and configure the audio device
        - convert mixer float samples to signed PCM and submit interleaved
          audio frames to the device
        - reject null-device fallback during real platform smoke tests

    This file intentionally does NOT contain:

        - mixer logic
        - MIDI routing
        - X11 or console behavior

    Platform notes:

        illumos still exposes the Sun audio interface through audioio(7I).
        The driver prefers signed 16-bit native-endian linear PCM and
        falls back to signed 8-bit linear when required by the device. The
        device path defaults to /dev/audio, and tests may override it with
        SFXLIB_SOLARIS_AUDIO_DEVICE.
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
#include <stropts.h>

static int solaris_audio_fd = -1;
static int solaris_audio_channels = FB_SFX_DEFAULT_CHANNELS;
static int solaris_audio_sample_rate = FB_SFX_DEFAULT_RATE;
static int solaris_audio_precision = 16;

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

static void solaris_audio_sleep_for_frames(int frames)
{
    struct timespec req;
    unsigned long long nanoseconds;

    if (frames <= 0 || solaris_audio_sample_rate <= 0)
        return;

    /*
        /dev/audio writes are opened non-blocking so the ES1370/audiopci path
        used by QEMU can never trap a process in the kernel.  This means the
        runtime must apply playback pacing itself, otherwise the worker thread
        can spin ahead of what the emulated backend is actually draining.
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

static signed char solaris_audio_float_to_s8(float sample)
{
    int converted;

    if (sample > 1.0f)
        sample = 1.0f;
    else if (sample < -1.0f)
        sample = -1.0f;

    converted = (int)(sample * 127.0f);
    if (converted > 127)
        converted = 127;
    else if (converted < -128)
        converted = -128;

    return (signed char)converted;
}

static int solaris_audio_apply_format(int fd,
                                     const audio_info_t *base_info,
                                     int rate,
                                     int channels,
                                     int precision,
                                     int encoding,
                                     audio_info_t *actual_info)
{
    audio_info_t info;

    if (base_info)
        info = *base_info;
    else
        AUDIO_INITINFO(&info);

    info.play.pause = 0;
    info.play.sample_rate = rate;
    info.play.channels = channels;

    if (precision)
    {
        info.play.precision = precision;
        info.play.encoding = encoding;
    }

    if (ioctl(fd, AUDIO_SETINFO, &info) == 0)
    {
        if (ioctl(fd, AUDIO_GETINFO, actual_info) == 0)
            return 0;

        SOLARIS_AUDIO_DBG("AUDIO_GETINFO after SETINFO failed: %s\n", strerror(errno));
        return -2;
    }

    SOLARIS_AUDIO_DBG("AUDIO_SETINFO failed: %s\n", strerror(errno));
    return -1;
}

static const char *solaris_audio_device_path(void)
{
    static const char *const fallback_devices[] = {
        "/dev/audio",
        "/dev/audio0",
        "/dev/dsp",
        "/dev/dsp0",
        "/dev/sound",
        NULL
    };
    size_t i;

    const char *path;

    path = getenv("SFXLIB_SOLARIS_AUDIO_DEVICE");
    if (path && *path)
        return path;

    path = getenv("SFXLIB_AUDIO_DEVICE");
    if (path && *path)
        return path;

    for (i = 0; fallback_devices[i] != NULL; ++i)
    {
        if (access(fallback_devices[i], F_OK) == 0)
            return fallback_devices[i];
    }

    return "/dev/audio";
}

static void solaris_audio_close(void)
{
    int fd;

    if (solaris_audio_fd < 0)
        return;

    /*
        The final close of /dev/audio performs an implicit AUDIO_DRAIN.
        Flush queued output first so shutdown does not wait for a virtual
        or real device to play stale samples during process exit.
    */

    fd = solaris_audio_fd;
    solaris_audio_fd = -1;

    if (ioctl(fd, I_FLUSH, FLUSHW) != 0)
        SOLARIS_AUDIO_DBG("I_FLUSH failed: %s\n", strerror(errno));

    close(fd);

    solaris_audio_channels = FB_SFX_DEFAULT_CHANNELS;
    solaris_audio_sample_rate = FB_SFX_DEFAULT_RATE;
    solaris_audio_precision = 16;
}

static int solaris_audio_configure(int fd,
                                   int rate,
                                   int channels,
                                   int *actual_rate_out,
                                   int *actual_channels_out)
{
    audio_info_t info;
    audio_info_t baseline_info;
    int i;
    int j;
    int k;
    int requested_rate;
    int requested_channels;
    int chs[2];
    int ch_count;
    int rates[2];
    int rate_count;
    int tries_count;
    int got_baseline_info;
    int success;

    struct
    {
        int precision;
        int encoding;
    } tries[] =
    {
        { 16, AUDIO_ENCODING_LINEAR },
        { 0, 0 }
    };

    requested_rate = (rate > 0) ? rate : FB_SFX_DEFAULT_RATE;
    requested_channels = (channels > 0) ? channels : FB_SFX_DEFAULT_CHANNELS;
    tries_count = (int)(sizeof(tries) / sizeof(tries[0]));
    got_baseline_info = 0;
    memset(&baseline_info, 0, sizeof(baseline_info));
    success = 0;

    chs[0] = requested_channels;
    ch_count = 1;
    if (requested_channels != 1)
    {
        chs[1] = 1;
        ch_count = 2;
    }

    rates[0] = requested_rate;
    rate_count = 1;

    if (ioctl(fd, AUDIO_GETINFO, &baseline_info) == 0)
    {
        got_baseline_info = 1;
        if (baseline_info.play.sample_rate > 0 &&
            baseline_info.play.sample_rate != (unsigned long)requested_rate)
        {
            rates[1] = (int)baseline_info.play.sample_rate;
            rate_count = 2;
        }
    }

    for (i = 0; i < ch_count && !success; ++i)
    {
        for (j = 0; j < rate_count && !success; ++j)
        {
            for (k = 0; k < tries_count && !success; ++k)
            {
                int apply_status;

                apply_status = solaris_audio_apply_format(fd,
                                                         got_baseline_info ? &baseline_info : NULL,
                                                         rates[j],
                                                         chs[i],
                                                         tries[k].precision,
                                                         tries[k].encoding,
                                                         &info);
                if (apply_status == 0)
                {
                    success = 1;
                    break;
                }

                if (apply_status == -2 && got_baseline_info)
                {
                    SOLARIS_AUDIO_DBG("AUDIO_GETINFO failed after successful SETINFO, falling back\n");
                    info = baseline_info;
                    success = 1;
                    break;
                }
            }
        }
    }

    if (!success)
    {
        if (!got_baseline_info)
        {
            return -1;
        }

        SOLARIS_AUDIO_DBG("using baseline /dev/audio settings after attempts failed\n");
        info = baseline_info;
    }

    if (info.play.encoding != AUDIO_ENCODING_LINEAR)
    {
        SOLARIS_AUDIO_DBG("unsupported audio encoding=%d\n", info.play.encoding);
        return -1;
    }

    if (info.play.precision == 8)
    {
        SOLARIS_AUDIO_DBG("using 8-bit linear path\n");
    }
    else if (info.play.precision != 16)
    {
        SOLARIS_AUDIO_DBG("unsupported sample precision=%d\n", info.play.precision);
        return -1;
    }

    if (info.play.channels <= 0 || info.play.sample_rate <= 0)
    {
        SOLARIS_AUDIO_DBG("device reported invalid format\n");
        return -1;
    }

    *actual_rate_out = info.play.sample_rate;
    *actual_channels_out = info.play.channels;
    solaris_audio_precision = info.play.precision;
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
    int sample_bytes;
    short *pcm_s16;
    signed char *pcm_s8;
    int samples;
    int bytes_total;
    int bytes_written;
    int frame_bytes;

    if (solaris_audio_fd < 0 || !buffer || frames <= 0)
        return -1;

    sample_bytes = (solaris_audio_precision == 8) ? (int)sizeof(signed char) : (int)sizeof(short);
    samples = frames * solaris_audio_channels;
    bytes_total = samples * sample_bytes;
    frame_bytes = solaris_audio_channels * sample_bytes;

    if (sample_bytes == 1)
    {
        pcm_s8 = (signed char *)malloc((size_t)bytes_total);
        if (!pcm_s8)
            return -1;

        for (samples = 0; samples < frames * solaris_audio_channels; ++samples)
        {
            pcm_s8[samples] = solaris_audio_float_to_s8(buffer[samples]);
        }
    }
    else
    {
        pcm_s16 = (short *)malloc((size_t)bytes_total);
        if (!pcm_s16)
            return -1;

        fb_sfxConvertFloatToS16(buffer, pcm_s16, samples);
    }

    if (samples <= 0)
    {
        if (sample_bytes == 1)
            free(pcm_s8);
        else
            free(pcm_s16);
        return -1;
    }

    fb_sfxDriverDiagnostics("Solaris audio",
                            buffer,
                            frames,
                            solaris_audio_channels);

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
                        ((const char *)(sample_bytes == 1 ? (void *)pcm_s8 : (void *)pcm_s16)) + bytes_written,
                        (size_t)request_bytes);

        if (written < 0)
        {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                int accepted_frames;

                accepted_frames = bytes_written / frame_bytes;

                if (accepted_frames > 0)
                    solaris_audio_sleep_for_frames(accepted_frames);

                if (sample_bytes == 1)
                    free(pcm_s8);
                else
                    free(pcm_s16);

                return accepted_frames;
            }

            SOLARIS_AUDIO_DBG("write failed: %s\n", strerror(errno));
            if (sample_bytes == 1)
                free(pcm_s8);
            else
                free(pcm_s16);
            return -1;
        }

        if (written == 0)
        {
            break;
        }

        bytes_written += (int)written;
    }

    if (sample_bytes == 1)
        free(pcm_s8);
    else
        free(pcm_s16);

    if (bytes_written <= 0)
        return 0;

    frames = bytes_written / frame_bytes;
    solaris_audio_sleep_for_frames(frames);
    return frames;
}

const FB_SFX_DRIVER fb_sfxDriverSolarisAudio =
{
    "Solaris audio",
    FB_SFX_DRIVER_CAP_BLOCKING,
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
