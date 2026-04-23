/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_linux.c
*/

#ifndef DISABLE_LINUX

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"
#include "fb_sfx_linux.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#if FB_SFX_MT_ENABLED
#include <pthread.h>
#endif

extern const FB_SFX_DRIVER fb_sfxDriverAlsa;
extern const FB_SFX_DRIVER fb_sfxDriverPulse;
extern const FB_SFX_DRIVER __fb_sfxDriverNull;

int fb_sfxCaptureAlsaStart(void);
void fb_sfxCaptureAlsaStop(void);
int fb_sfxCaptureAlsaRead(float *buffer, int frames);

FB_SFX_LINUX_STATE fb_sfx_linux =
{
    0,
    FB_SFX_DEFAULT_RATE,
    FB_SFX_DEFAULT_CHANNELS,
    FB_SFX_DEFAULT_BUFFER,
    0
};

static int g_linux_debug_initialized = 0;
static int g_linux_debug_enabled = 0;

#if FB_SFX_MT_ENABLED
static pthread_t g_linux_audio_thread;
static int g_linux_audio_thread_valid = 0;
static volatile int g_linux_audio_thread_stop = 0;
#endif

static void fb_sfxLinuxSleepMs(unsigned long milliseconds)
{
    struct timespec req;

    req.tv_sec = (time_t)(milliseconds / 1000UL);
    req.tv_nsec = (long)((milliseconds % 1000UL) * 1000000UL);
    nanosleep(&req, NULL);
}

static int fb_sfxLinuxWorkerFrames(void)
{
    int frames;

    frames = (fb_sfx_linux.buffer_frames > 0)
        ? (fb_sfx_linux.buffer_frames / 4)
        : (FB_SFX_DEFAULT_BUFFER / 4);

    if (frames < 256)
        frames = 256;
    else if (frames > 2048)
        frames = 2048;

    return frames;
}

#if FB_SFX_MT_ENABLED
static void *fb_sfxLinuxAudioWorker(void *unused)
{
    (void)unused;

    while (!g_linux_audio_thread_stop)
    {
        if (!fb_sfx_linux.running)
        {
            fb_sfxLinuxSleepMs(5);
            continue;
        }

        fb_sfxUpdate(fb_sfxLinuxWorkerFrames());
    }

    return NULL;
}

static int fb_sfxLinuxEnsureWorker(void)
{
    if (g_linux_audio_thread_valid)
        return 0;

    g_linux_audio_thread_stop = 0;

    if (pthread_create(&g_linux_audio_thread, NULL, fb_sfxLinuxAudioWorker, NULL) != 0)
        return -1;

    g_linux_audio_thread_valid = 1;
    return 0;
}
#endif

void fb_sfxLinuxInitDebug(void)
{
    const char *env;

    if (g_linux_debug_initialized)
        return;

    g_linux_debug_initialized = 1;
    env = getenv("SFXLIB_LINUX_DEBUG");
    g_linux_debug_enabled = (env && *env && *env != '0');
}

int fb_sfxLinuxDebugEnabled(void)
{
    fb_sfxLinuxInitDebug();
    return g_linux_debug_enabled;
}

int fb_sfxLinuxInit(void)
{
    if (fb_sfx_linux.initialized)
        return 0;

    fb_sfxLinuxInitDebug();
    fb_sfx_linux.initialized = 1;
    fb_sfx_linux.running = 0;

    if (fb_sfxLinuxDebugEnabled())
    {
        fprintf(stderr,
            "SFX_LINUX: initialized (rate=%d channels=%d buffer=%d)\n",
            fb_sfx_linux.sample_rate,
            fb_sfx_linux.channels,
            fb_sfx_linux.buffer_frames);
    }

    return 0;
}

int fb_sfxLinuxActivate(int rate, int channels, int buffer_frames)
{
    if (fb_sfxLinuxInit() != 0)
        return -1;

    fb_sfx_linux.sample_rate = (rate > 0) ? rate : FB_SFX_DEFAULT_RATE;
    fb_sfx_linux.channels = (channels > 0) ? channels : FB_SFX_DEFAULT_CHANNELS;
    fb_sfx_linux.buffer_frames = (buffer_frames > 0) ? buffer_frames : FB_SFX_DEFAULT_BUFFER;

#if FB_SFX_MT_ENABLED
    if (fb_sfxLinuxEnsureWorker() != 0)
        return -1;
#endif

    fb_sfx_linux.running = 1;
    return 0;
}

void fb_sfxLinuxDeactivate(void)
{
    fb_sfx_linux.running = 0;
}

void fb_sfxLinuxExit(void)
{
    if (!fb_sfx_linux.initialized)
        return;

    fb_sfx_linux.running = 0;

#if FB_SFX_MT_ENABLED
    if (g_linux_audio_thread_valid)
    {
        g_linux_audio_thread_stop = 1;
        if (!pthread_equal(g_linux_audio_thread, pthread_self()))
            pthread_join(g_linux_audio_thread, NULL);
        g_linux_audio_thread_valid = 0;
    }
#endif

    fb_sfx_linux.initialized = 0;

    if (fb_sfxLinuxDebugEnabled())
        fprintf(stderr, "SFX_LINUX: shutdown\n");
}

int fb_sfxLinuxWrite(float *buffer, int frames)
{
    (void)buffer;
    (void)frames;

    if (!fb_sfx_linux.initialized || !fb_sfx_linux.running)
        return -1;

    return frames;
}

int fb_sfxLinuxRunning(void)
{
    return fb_sfx_linux.running;
}

int fb_sfxPlatformCaptureStart(void)
{
    return fb_sfxCaptureAlsaStart();
}

void fb_sfxPlatformCaptureStop(void)
{
    fb_sfxCaptureAlsaStop();
}

int fb_sfxPlatformCaptureRead(float *buffer, int frames)
{
    return fb_sfxCaptureAlsaRead(buffer, frames);
}

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
    &fb_sfxDriverPulse,
    &fb_sfxDriverAlsa,
    &__fb_sfxDriverNull,
    NULL
};

#endif

/* end of sfx_linux.c */
