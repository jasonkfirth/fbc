/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_haiku.cpp

    Purpose:

        Implement the Haiku playback driver using BSoundPlayer.

    Responsibilities:

        - initialize and shut down Haiku playback
        - queue mixed float samples for the audio callback
        - convert callback output to signed 16-bit PCM
        - run the background feeder thread when threading is available

    This file intentionally does NOT contain:

        - Haiku capture buffering
        - MIDI routing
        - mixer or synthesis logic
        - BASIC command parsing
*/

#ifndef DISABLE_HAIKU

#include "fb_sfx_haiku.h"

#include "../fb_sfx.h"
#include "../fb_sfx_driver.h"
#include "../fb_sfx_internal.h"

#include <SoundPlayer.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if FB_SFX_MT_ENABLED
#include <pthread.h>
#endif

/* ------------------------------------------------------------------------- */
/* Global backend state                                                      */
/* ------------------------------------------------------------------------- */

FB_SFX_HAIKU_STATE fb_sfx_haiku =
{
    0,
    44100,
    2,
    1024,
    0
};

/* ------------------------------------------------------------------------- */
/* Ring buffer                                                               */
/* ------------------------------------------------------------------------- */

static float *rb_data = NULL;
static int rb_size = 0;
static int rb_count = 0;
static volatile int rb_write = 0;
static volatile int rb_read = 0;

#if FB_SFX_MT_ENABLED
static pthread_t g_audio_thread;
static int g_audio_thread_valid = 0;
static volatile int g_audio_thread_stop = 0;
#endif

static void rb_shutdown(void)
{
    if (rb_data)
    {
        free(rb_data);
        rb_data = NULL;
    }

    rb_size = 0;
    rb_count = 0;
    rb_write = 0;
    rb_read = 0;
}

static int rb_init(int frames, int channels)
{
    if (frames <= 0 || channels <= 0)
        return -1;

    if (frames > INT_MAX / channels || (frames * channels) > INT_MAX / 4)
        return -1;

    rb_size = frames * channels * 4; /* extra slack */
    rb_data = (float*)malloc(rb_size * sizeof(float));
    if (!rb_data) return -1;
    rb_count = 0;
    rb_write = rb_read = 0;
    return 0;
}

static int rb_free(void)
{
    if (rb_size <= 0)
        return 0;

    return rb_size - rb_count;
}

static int rb_push_locked(const float *in, int count)
{
    int accepted;
    int first;
    int second;

    if (!in || count <= 0 || rb_size <= 0)
        return 0;

    accepted = count;
    if (accepted > rb_free())
        accepted = rb_free();

    if (accepted <= 0)
        return 0;

    first = rb_size - rb_write;
    if (first > accepted)
        first = accepted;

    memcpy(rb_data + rb_write, in, (size_t)first * sizeof(float));

    rb_write += first;
    if (rb_write >= rb_size)
        rb_write = 0;

    second = accepted - first;
    if (second > 0)
    {
        memcpy(rb_data, in + first, (size_t)second * sizeof(float));
        rb_write = second;
    }

    rb_count += accepted;
    return accepted;
}

static int rb_pop_locked(float *out, int count)
{
    int actual;
    int first;
    int second;

    if (!out || count <= 0)
        return 0;

    if (rb_size <= 0 || rb_count <= 0)
    {
        memset(out, 0, (size_t)count * sizeof(float));
        return 0;
    }

    actual = count;
    if (actual > rb_count)
        actual = rb_count;

    first = rb_size - rb_read;
    if (first > actual)
        first = actual;

    memcpy(out, rb_data + rb_read, (size_t)first * sizeof(float));

    rb_read += first;
    if (rb_read >= rb_size)
        rb_read = 0;

    second = actual - first;
    if (second > 0)
    {
        memcpy(out + first, rb_data, (size_t)second * sizeof(float));
        rb_read = second;
    }

    rb_count -= actual;

    if (actual < count)
        memset(out + actual, 0, (size_t)(count - actual) * sizeof(float));

    return actual;
}

static int rb_push(const float *in, int count)
{
    int accepted;

    if (!fb_sfx_haiku.running || count <= 0)
        return 0;

    fb_sfxDriverIoLock();
    accepted = rb_push_locked(in, count);
    fb_sfxDriverIoUnlock();

    return accepted;
}

static int rb_pop(float *out, int count)
{
    int read;

    if (!fb_sfx_haiku.running)
    {
        if (out && count > 0)
            memset(out, 0, (size_t)count * sizeof(float));
        return 0;
    }

    fb_sfxDriverIoLock();
    read = rb_pop_locked(out, count);
    fb_sfxDriverIoUnlock();

    return read;
}

/* ------------------------------------------------------------------------- */
/* Worker thread                                                             */
/* ------------------------------------------------------------------------- */

static void haiku_sleep_ms(unsigned long milliseconds)
{
    struct timespec req;

    req.tv_sec = (time_t)(milliseconds / 1000UL);
    req.tv_nsec = (long)((milliseconds % 1000UL) * 1000000UL);
    nanosleep(&req, NULL);
}

static int haiku_worker_frames(void)
{
    int frames;

    frames = (fb_sfx_haiku.buffer_frames > 0)
        ? (fb_sfx_haiku.buffer_frames / 4)
        : (1024 / 4);

    if (frames < 256)
        frames = 256;
    else if (frames > 2048)
        frames = 2048;

    return frames;
}

#if FB_SFX_MT_ENABLED
static void *haiku_audio_worker(void *unused)
{
    (void)unused;

    while (!g_audio_thread_stop)
    {
        if (!fb_sfx_haiku.running)
        {
            haiku_sleep_ms(5);
            continue;
        }

        if (fb_sfxForegroundFeedActive())
        {
            haiku_sleep_ms(5);
            continue;
        }

        fb_sfxUpdate(haiku_worker_frames());
    }

    return NULL;
}

static int haiku_ensure_worker(void)
{
    if (g_audio_thread_valid)
        return 0;

    g_audio_thread_stop = 0;

    if (pthread_create(&g_audio_thread, NULL, haiku_audio_worker, NULL) != 0)
        return -1;

    g_audio_thread_valid = 1;
    return 0;
}
#endif

/* ------------------------------------------------------------------------- */
/* SoundPlayer                                                               */
/* ------------------------------------------------------------------------- */

static BSoundPlayer *g_player = NULL;

static void haiku_player_shutdown(void)
{
    if (g_player)
    {
        g_player->Stop();
        delete g_player;
        g_player = NULL;
    }
}

static void audio_callback(void *cookie, void *buffer, size_t size, const media_raw_audio_format &fmt)
{
    (void)cookie;
    (void)fmt;

    if (!__fb_sfx || !fb_sfx_haiku.running)
    {
        memset(buffer, 0, size);
        return;
    }

    int samples = size / sizeof(short);
    short *out = (short*)buffer;
    int remaining = samples;
    int offset = 0;
    float temp[2048];

    while (remaining > 0)
    {
        int chunk = remaining;

        if (chunk > (int)(sizeof(temp) / sizeof(temp[0])))
            chunk = (int)(sizeof(temp) / sizeof(temp[0]));

        rb_pop(temp, chunk);

        fb_sfxConvertFloatToS16(temp, out + offset, chunk);

        offset += chunk;
        remaining -= chunk;
    }
}

/* ------------------------------------------------------------------------- */
/* Driver core                                                               */
/* ------------------------------------------------------------------------- */

static int haiku_driver_init(int rate, int channels, int buffer, int flags)
{
    status_t err;
    int bytes_per_sample;

    (void)flags;

    if (fb_sfx_haiku.running)
        return 0;

    fb_sfx_haiku.sample_rate = rate > 0 ? rate : 44100;
    fb_sfx_haiku.channels = channels > 0 ? channels : 2;
    fb_sfx_haiku.buffer_frames = buffer > 0 ? buffer : 1024;

    if (rb_init(fb_sfx_haiku.buffer_frames, fb_sfx_haiku.channels) != 0)
        return -1;

    media_raw_audio_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.frame_rate = fb_sfx_haiku.sample_rate;
    fmt.channel_count = fb_sfx_haiku.channels;
    fmt.format = media_raw_audio_format::B_AUDIO_SHORT;
    fmt.byte_order = B_MEDIA_LITTLE_ENDIAN;
    bytes_per_sample = (int)sizeof(short);
    fmt.buffer_size = fb_sfx_haiku.buffer_frames *
                      fb_sfx_haiku.channels *
                      bytes_per_sample;

    if (fmt.buffer_size <= 0)
    {
        rb_shutdown();
        return -1;
    }

    g_player = new BSoundPlayer(&fmt, "fbsfx", audio_callback, NULL);
    if (!g_player)
    {
        rb_shutdown();
        return -1;
    }

    err = g_player->InitCheck();
    if (err != B_OK)
    {
        haiku_player_shutdown();
        rb_shutdown();
        return -1;
    }

    g_player->SetHasData(true);
    g_player->Start();

#if FB_SFX_MT_ENABLED
    if (haiku_ensure_worker() != 0)
    {
        haiku_player_shutdown();
        rb_shutdown();
        return -1;
    }
#endif

    fb_sfx_haiku.initialized = 1;
    fb_sfx_haiku.running = 1;

    return 0;
}

static void haiku_driver_exit(void)
{
    fb_sfx_haiku.running = 0;
    fb_sfx_haiku.initialized = 0;

#if FB_SFX_MT_ENABLED
    if (g_audio_thread_valid)
    {
        g_audio_thread_stop = 1;
        if (!pthread_equal(g_audio_thread, pthread_self()))
            pthread_join(g_audio_thread, NULL);
        g_audio_thread_valid = 0;
    }
#endif

    haiku_player_shutdown();
    rb_shutdown();
}

static int haiku_driver_write(const float *buffer, int frames)
{
    int count;
    int accepted;

    if (!fb_sfx_haiku.running || !buffer || frames <= 0)
        return 0;

    if (fb_sfx_haiku.channels <= 0)
        return -1;

    if (frames > INT_MAX / fb_sfx_haiku.channels)
        return -1;

    count = frames * fb_sfx_haiku.channels;
    accepted = rb_push(buffer, count);

    return accepted / fb_sfx_haiku.channels;
}

static void haiku_driver_poll(void)
{
}

/* ------------------------------------------------------------------------- */
/* Export                                                                    */
/* ------------------------------------------------------------------------- */

extern "C" const FB_SFX_DRIVER fb_sfxDriverHaiku =
{
    "Haiku",
    FB_SFX_DRIVER_CAP_BACKGROUND,
    haiku_driver_init,
    haiku_driver_exit,
    haiku_driver_write,
    NULL,
    haiku_driver_poll,
    NULL,
    NULL
};

#endif

/* end of sfx_driver_haiku.cpp */
