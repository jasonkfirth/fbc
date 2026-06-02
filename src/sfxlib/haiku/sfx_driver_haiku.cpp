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

#include <OS.h>
#include <SoundPlayer.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if FB_SFX_MT_ENABLED
#include <pthread.h>
#endif

#ifndef FB_SFX_DRIVER_CAP_BACKGROUND
#define FB_SFX_DRIVER_CAP_BACKGROUND 0
static int fb_sfxForegroundFeedActive(void)
{
    return 0;
}
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
static int rb_write = 0;
static int rb_read = 0;

#if FB_SFX_MT_ENABLED
static pthread_mutex_t g_ring_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_worker_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_audio_thread;
static int g_audio_thread_valid = 0;
static int g_audio_thread_stop = 0;
#endif

/*
    Ring synchronization

    BSoundPlayer invokes audio_callback() from Haiku's media thread.  That
    callback must never wait behind a producer thread that happens to be
    holding the generic driver I/O lock.  The producer side therefore uses a
    small ring-only mutex, while the callback uses trylock and outputs silence
    if the ring is momentarily busy.
*/

static void rb_lock(void)
{
#if FB_SFX_MT_ENABLED
    pthread_mutex_lock(&g_ring_mutex);
#endif
}

static void rb_unlock(void)
{
#if FB_SFX_MT_ENABLED
    pthread_mutex_unlock(&g_ring_mutex);
#endif
}

static int rb_try_lock(void)
{
#if FB_SFX_MT_ENABLED
    return pthread_mutex_trylock(&g_ring_mutex) == 0;
#else
    return 1;
#endif
}

static void haiku_sleep_ms(unsigned long milliseconds);

#if FB_SFX_MT_ENABLED
static void haiku_worker_stop_set(int value)
{
    pthread_mutex_lock(&g_worker_mutex);
    g_audio_thread_stop = value;
    pthread_mutex_unlock(&g_worker_mutex);
}

static int haiku_worker_stop_requested(void)
{
    int result;

    pthread_mutex_lock(&g_worker_mutex);
    result = g_audio_thread_stop;
    pthread_mutex_unlock(&g_worker_mutex);

    return result;
}
#endif

static void haiku_running_set(int value)
{
#if FB_SFX_MT_ENABLED
    pthread_mutex_lock(&g_worker_mutex);
#endif
    fb_sfx_haiku.running = value;
#if FB_SFX_MT_ENABLED
    pthread_mutex_unlock(&g_worker_mutex);
#endif
}

static int haiku_running_get(void)
{
    int result;

#if FB_SFX_MT_ENABLED
    pthread_mutex_lock(&g_worker_mutex);
#endif
    result = fb_sfx_haiku.running;
#if FB_SFX_MT_ENABLED
    pthread_mutex_unlock(&g_worker_mutex);
#endif

    return result;
}

static void rb_shutdown(void)
{
    rb_lock();

    if (rb_data)
    {
        free(rb_data);
        rb_data = NULL;
    }

    rb_size = 0;
    rb_count = 0;
    rb_write = 0;
    rb_read = 0;

    rb_unlock();
}

static int rb_init(int frames, int channels)
{
    int result;

    if (frames <= 0 || channels <= 0)
        return -1;

    if (frames > INT_MAX / channels || (frames * channels) > INT_MAX / 4)
        return -1;

    result = 0;
    rb_lock();

    if (rb_data)
    {
        free(rb_data);
        rb_data = NULL;
    }

    rb_size = frames * channels * 4; /* extra slack */
    rb_data = (float*)malloc(rb_size * sizeof(float));
    if (!rb_data)
    {
        rb_size = 0;
        rb_count = 0;
        rb_write = 0;
        rb_read = 0;
        result = -1;
    }
    else
    {
        rb_count = 0;
        rb_write = rb_read = 0;
    }

    rb_unlock();
    return result;
}

static void rb_clear(void)
{
    rb_lock();

    rb_count = 0;
    rb_write = 0;
    rb_read = 0;

    rb_unlock();
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

static int rb_pop(float *out, int count)
{
    int read;

    if (!rb_try_lock())
    {
        if (out && count > 0)
            memset(out, 0, (size_t)count * sizeof(float));
        return 0;
    }

    read = rb_pop_locked(out, count);
    rb_unlock();

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

static int rb_push_frames(const float *in, int frames, int channels)
{
    int accepted_frames;
    int idle_count;

    if (!in || frames <= 0 || channels <= 0)
        return 0;

    accepted_frames = 0;
    idle_count = 0;

    /*
        BSoundPlayer consumes audio asynchronously from the Media Kit callback.
        A quick foreground program can therefore produce samples faster than
        the callback can drain them, especially on slow Haiku hardware.

        The driver write contract is frame based, so only complete frames are
        queued.  If the ring is full, wait for the callback to make room instead
        of reporting a short write immediately.  The timeout is deliberately
        generous: one blocked 8192-frame write should normally clear in well
        under a quarter second at 44.1 kHz, while five seconds still prevents a
        missing or wedged audio server from trapping the program forever.
    */

    while (accepted_frames < frames && haiku_running_get())
    {
        int free_frames;
        int push_frames;
        int pushed_samples;
        int pushed_frames;

        rb_lock();

        free_frames = rb_free() / channels;
        push_frames = frames - accepted_frames;
        if (push_frames > free_frames)
            push_frames = free_frames;

        pushed_samples = rb_push_locked(in + (accepted_frames * channels),
                                        push_frames * channels);

        rb_unlock();

        pushed_frames = pushed_samples / channels;
        if (pushed_frames > 0)
        {
            accepted_frames += pushed_frames;
            idle_count = 0;
            continue;
        }

        if (++idle_count >= 5000)
            break;

        haiku_sleep_ms(1);
    }

    return accepted_frames;
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

    while (!haiku_worker_stop_requested())
    {
        if (!haiku_running_get())
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

    haiku_worker_stop_set(0);

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

#define FB_SFX_HAIKU_INIT_TIMEOUT_MS 1500

static void audio_callback(void *cookie, void *buffer, size_t size,
    const media_raw_audio_format &fmt);

typedef struct HAIKU_PLAYER_INIT_REQUEST
{
    media_raw_audio_format fmt;
    sem_id done_sem;
    sem_id lock_sem;
    BSoundPlayer *player;
    status_t status;
    int completed;
    int timed_out;
} HAIKU_PLAYER_INIT_REQUEST;

static void haiku_player_shutdown(void)
{
    if (g_player)
    {
        g_player->Stop();
        delete g_player;
        g_player = NULL;
    }
}

static int haiku_init_timeout_ms(void)
{
    const char *value;
    int timeout;

    value = getenv("FB_SFX_HAIKU_INIT_TIMEOUT_MS");
    if (!value || !*value)
        return FB_SFX_HAIKU_INIT_TIMEOUT_MS;

    timeout = atoi(value);
    if (timeout < 0)
        timeout = 0;

    return timeout;
}

static void haiku_request_lock(HAIKU_PLAYER_INIT_REQUEST *request)
{
    if (request && request->lock_sem >= B_OK)
        acquire_sem(request->lock_sem);
}

static void haiku_request_unlock(HAIKU_PLAYER_INIT_REQUEST *request)
{
    if (request && request->lock_sem >= B_OK)
        release_sem(request->lock_sem);
}

static void haiku_player_request_destroy(HAIKU_PLAYER_INIT_REQUEST *request)
{
    if (!request)
        return;

    if (request->done_sem >= B_OK)
        delete_sem(request->done_sem);

    if (request->lock_sem >= B_OK)
        delete_sem(request->lock_sem);

    free(request);
}

static int32 haiku_player_create_thread(void *arg)
{
    HAIKU_PLAYER_INIT_REQUEST *request = (HAIKU_PLAYER_INIT_REQUEST*)arg;
    BSoundPlayer *player;
    status_t status;
    int timed_out;

    player = new BSoundPlayer(&request->fmt, "fbsfx", audio_callback, NULL);
    if (!player)
        status = B_NO_MEMORY;
    else
        status = player->InitCheck();

    if (player && status != B_OK)
    {
        delete player;
        player = NULL;
    }

    haiku_request_lock(request);

    request->player = player;
    request->status = status;
    request->completed = 1;
    timed_out = request->timed_out;

    haiku_request_unlock(request);

    if (timed_out)
    {
        if (player)
            delete player;

        haiku_player_request_destroy(request);
        return 0;
    }

    release_sem(request->done_sem);
    return 0;
}

static BSoundPlayer *haiku_player_create(media_raw_audio_format *fmt)
{
    HAIKU_PLAYER_INIT_REQUEST *request;
    BSoundPlayer *player;
    thread_id thread;
    status_t wait_result;
    int timeout_ms;
    int completed;
    status_t status;

    if (!fmt)
        return NULL;

    request = (HAIKU_PLAYER_INIT_REQUEST*)calloc(1, sizeof(*request));
    if (!request)
        return NULL;

    request->fmt = *fmt;
    request->done_sem = create_sem(0, "fb_sfx_haiku_player_done");
    request->lock_sem = create_sem(1, "fb_sfx_haiku_player_lock");
    request->status = B_ERROR;

    if (request->done_sem < B_OK || request->lock_sem < B_OK)
    {
        haiku_player_request_destroy(request);
        return NULL;
    }

    thread = spawn_thread(
        haiku_player_create_thread,
        "fb_sfx_haiku_player_init",
        B_NORMAL_PRIORITY,
        request
    );

    if (thread < B_OK)
    {
        haiku_player_request_destroy(request);
        return NULL;
    }

    if (resume_thread(thread) != B_OK)
    {
        haiku_player_request_destroy(request);
        return NULL;
    }

    /*
        Some Haiku systems can block indefinitely while BSoundPlayer asks the
        Media Kit for a usable output.  Audio is optional for BASIC programs,
        so fail this driver quickly and let sfxlib's null driver keep the
        program running.
    */
    timeout_ms = haiku_init_timeout_ms();
    if (timeout_ms == 0)
    {
        wait_for_thread(thread, NULL);
        wait_result = B_OK;
    }
    else
    {
        wait_result = acquire_sem_etc(
            request->done_sem,
            1,
            B_RELATIVE_TIMEOUT,
            (bigtime_t)timeout_ms * 1000
        );
    }

    haiku_request_lock(request);

    completed = request->completed;
    status = request->status;

    if (wait_result == B_OK || completed)
    {
        player = request->player;
        request->player = NULL;
        haiku_request_unlock(request);

        haiku_player_request_destroy(request);

        if (status != B_OK)
        {
            if (player)
                delete player;
            return NULL;
        }

        return player;
    }

    request->timed_out = 1;
    haiku_request_unlock(request);

    return NULL;
}

static void audio_callback(void *cookie, void *buffer, size_t size, const media_raw_audio_format &fmt)
{
    (void)cookie;
    (void)fmt;

    if (!__fb_sfx)
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
    int bytes_per_sample;

    (void)flags;

    if (haiku_running_get())
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

    g_player = haiku_player_create(&fmt);
    if (!g_player)
    {
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
    haiku_running_set(1);

    return 0;
}

static void haiku_driver_exit(void)
{
    haiku_running_set(0);
    fb_sfx_haiku.initialized = 0;
    rb_clear();

#if FB_SFX_MT_ENABLED
    if (g_audio_thread_valid)
    {
        haiku_worker_stop_set(1);
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
    int accepted;

    if (!haiku_running_get() || !buffer || frames <= 0)
        return 0;

    if (fb_sfx_haiku.channels <= 0)
        return -1;

    if (frames > INT_MAX / fb_sfx_haiku.channels)
        return -1;

    accepted = rb_push_frames(buffer, frames, fb_sfx_haiku.channels);

    return accepted;
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
