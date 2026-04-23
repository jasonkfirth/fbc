/*
    Shared BSD backend scaffold.

    This file is included by per-target BSD backend shims. It provides
    Linux-style runtime state and background feed plumbing so future
    OSS/sndio/native BSD drivers can opt into unattended playback
    without redoing the worker model.
*/

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if FB_SFX_MT_ENABLED
#include <pthread.h>
#endif

#ifndef FB_SFX_BSD_LABEL
#error "FB_SFX_BSD_LABEL must be defined before including sfx_bsd_template.inc"
#endif

#ifndef FB_SFX_BSD_STATE_NAME
#error "FB_SFX_BSD_STATE_NAME must be defined before including sfx_bsd_template.inc"
#endif

#ifndef FB_SFX_BSD_DRIVER_NAME
#error "FB_SFX_BSD_DRIVER_NAME must be defined before including sfx_bsd_template.inc"
#endif

#ifndef FB_SFX_BSD_DEBUG_ENV
#error "FB_SFX_BSD_DEBUG_ENV must be defined before including sfx_bsd_template.inc"
#endif

#ifndef FB_SFX_BSD_INIT_FN
#error "FB_SFX_BSD_INIT_FN must be defined before including sfx_bsd_template.inc"
#endif

#ifndef FB_SFX_BSD_EXIT_FN
#error "FB_SFX_BSD_EXIT_FN must be defined before including sfx_bsd_template.inc"
#endif

#ifndef FB_SFX_BSD_WRITE_FN
#error "FB_SFX_BSD_WRITE_FN must be defined before including sfx_bsd_template.inc"
#endif

#ifndef FB_SFX_BSD_RUNNING_FN
#error "FB_SFX_BSD_RUNNING_FN must be defined before including sfx_bsd_template.inc"
#endif

#ifndef FB_SFX_BSD_ACTIVATE_FN
#error "FB_SFX_BSD_ACTIVATE_FN must be defined before including sfx_bsd_template.inc"
#endif

#ifndef FB_SFX_BSD_DEACTIVATE_FN
#error "FB_SFX_BSD_DEACTIVATE_FN must be defined before including sfx_bsd_template.inc"
#endif

extern const FB_SFX_DRIVER FB_SFX_BSD_DRIVER_NAME;
extern const FB_SFX_DRIVER __fb_sfxDriverNull;

FB_SFX_BSD_STATE FB_SFX_BSD_STATE_NAME =
{
    0,
    FB_SFX_DEFAULT_RATE,
    FB_SFX_DEFAULT_CHANNELS,
    FB_SFX_DEFAULT_BUFFER,
    0
};

static int g_bsd_debug_initialized = 0;
static int g_bsd_debug_enabled = 0;

#if FB_SFX_MT_ENABLED
static pthread_t g_bsd_audio_thread;
static int g_bsd_audio_thread_valid = 0;
static volatile int g_bsd_audio_thread_stop = 0;
#endif

static void fb_sfxBsdSleepMs(unsigned long milliseconds)
{
    struct timespec req;

    req.tv_sec = (time_t)(milliseconds / 1000UL);
    req.tv_nsec = (long)((milliseconds % 1000UL) * 1000000UL);
    nanosleep(&req, NULL);
}

static int fb_sfxBsdWorkerFrames(void)
{
    int frames;

    frames = (FB_SFX_BSD_STATE_NAME.buffer_frames > 0)
        ? (FB_SFX_BSD_STATE_NAME.buffer_frames / 4)
        : (FB_SFX_DEFAULT_BUFFER / 4);

    if (frames < 256)
        frames = 256;
    else if (frames > 2048)
        frames = 2048;

    return frames;
}

#if FB_SFX_MT_ENABLED
static void *fb_sfxBsdAudioWorker(void *unused)
{
    (void)unused;

    while (!g_bsd_audio_thread_stop)
    {
        if (!FB_SFX_BSD_STATE_NAME.running)
        {
            fb_sfxBsdSleepMs(5);
            continue;
        }

        fb_sfxUpdate(fb_sfxBsdWorkerFrames());
    }

    return NULL;
}

static int fb_sfxBsdEnsureWorker(void)
{
    if (g_bsd_audio_thread_valid)
        return 0;

    g_bsd_audio_thread_stop = 0;

    if (pthread_create(&g_bsd_audio_thread, NULL, fb_sfxBsdAudioWorker, NULL) != 0)
        return -1;

    g_bsd_audio_thread_valid = 1;
    return 0;
}
#endif

static void fb_sfxBsdInitDebug(void)
{
    const char *env;

    if (g_bsd_debug_initialized)
        return;

    g_bsd_debug_initialized = 1;
    env = getenv(FB_SFX_BSD_DEBUG_ENV);
    g_bsd_debug_enabled = (env && *env && *env != '0');
}

static int fb_sfxBsdDebugEnabled(void)
{
    fb_sfxBsdInitDebug();
    return g_bsd_debug_enabled;
}

#define BSD_DBG(...) \
    do { if (fb_sfxBsdDebugEnabled()) fprintf(stderr, "SFX_%s: " __VA_ARGS__, FB_SFX_BSD_LABEL); } while (0)

int FB_SFX_BSD_INIT_FN(void)
{
    if (FB_SFX_BSD_STATE_NAME.initialized)
        return 0;

    fb_sfxBsdInitDebug();
    FB_SFX_BSD_STATE_NAME.initialized = 1;
    FB_SFX_BSD_STATE_NAME.running = 0;

    BSD_DBG("initialized (rate=%d channels=%d buffer=%d)\n",
            FB_SFX_BSD_STATE_NAME.sample_rate,
            FB_SFX_BSD_STATE_NAME.channels,
            FB_SFX_BSD_STATE_NAME.buffer_frames);

    return 0;
}

int FB_SFX_BSD_ACTIVATE_FN(int rate, int channels, int buffer_frames)
{
    if (FB_SFX_BSD_INIT_FN() != 0)
        return -1;

    FB_SFX_BSD_STATE_NAME.sample_rate = (rate > 0) ? rate : FB_SFX_DEFAULT_RATE;
    FB_SFX_BSD_STATE_NAME.channels = (channels > 0) ? channels : FB_SFX_DEFAULT_CHANNELS;
    FB_SFX_BSD_STATE_NAME.buffer_frames = (buffer_frames > 0) ? buffer_frames : FB_SFX_DEFAULT_BUFFER;

#if FB_SFX_MT_ENABLED
    if (fb_sfxBsdEnsureWorker() != 0)
        return -1;
#endif

    FB_SFX_BSD_STATE_NAME.running = 1;
    return 0;
}

void FB_SFX_BSD_DEACTIVATE_FN(void)
{
    FB_SFX_BSD_STATE_NAME.running = 0;
}

void FB_SFX_BSD_EXIT_FN(void)
{
    if (!FB_SFX_BSD_STATE_NAME.initialized)
        return;

    FB_SFX_BSD_STATE_NAME.running = 0;

#if FB_SFX_MT_ENABLED
    if (g_bsd_audio_thread_valid)
    {
        g_bsd_audio_thread_stop = 1;
        pthread_join(g_bsd_audio_thread, NULL);
        g_bsd_audio_thread_valid = 0;
    }
#endif

    FB_SFX_BSD_STATE_NAME.initialized = 0;

    BSD_DBG("shutdown\n");
}

int FB_SFX_BSD_WRITE_FN(float *buffer, int frames)
{
    (void)buffer;
    (void)frames;

    if (!FB_SFX_BSD_STATE_NAME.initialized || !FB_SFX_BSD_STATE_NAME.running)
        return -1;

    return frames;
}

int FB_SFX_BSD_RUNNING_FN(void)
{
    return FB_SFX_BSD_STATE_NAME.running;
}

int fb_sfxPlatformCaptureStart(void)
{
    return -1;
}

void fb_sfxPlatformCaptureStop(void)
{
}

int fb_sfxPlatformCaptureRead(float *buffer, int frames)
{
    (void)buffer;
    (void)frames;
    return 0;
}

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
    &FB_SFX_BSD_DRIVER_NAME,
    &__fb_sfxDriverNull,
    NULL
};

#undef BSD_DBG
