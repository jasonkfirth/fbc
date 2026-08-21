/*
    FreeBASIC Sound Library support for AROS
    ----------------------------------------

    File: sfx_unix.c

    Purpose:

        Feed the AROS AHI output independently of the BASIC program.

    Responsibilities:

        - own the AROS sfxlib worker thread
        - pace updates through blocking ahi.device writes
        - pause during foreground sound commands
        - provide bounded and idempotent lifecycle operations

    This file intentionally does NOT contain:

        - AHI device requests
        - sample conversion
        - portable mixer implementation
*/

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "fb_sfx_aros.h"

#include <pthread.h>
#include <unistd.h>

#define FB_SFX_AROS_WORKER_MIN_FRAMES 256
#define FB_SFX_AROS_WORKER_MAX_FRAMES 1024

static pthread_t g_aros_audio_thread;
static volatile int g_aros_audio_thread_stop;
static int g_aros_audio_thread_valid;
static int g_aros_audio_worker_frames;

static int aros_workerFrames(int buffer_frames)
{
    int frames;

    frames = (buffer_frames > 0)
        ? buffer_frames / 4
        : FB_SFX_DEFAULT_BUFFER / 4;
    if (frames < FB_SFX_AROS_WORKER_MIN_FRAMES)
        frames = FB_SFX_AROS_WORKER_MIN_FRAMES;
    else if (frames > FB_SFX_AROS_WORKER_MAX_FRAMES)
        frames = FB_SFX_AROS_WORKER_MAX_FRAMES;
    return frames;
}

static int aros_workerCanMix(void)
{
    int ready;

    fb_sfxRuntimeLock();
    ready = (__fb_sfx != NULL) && __fb_sfx->initialized &&
        !__fb_sfx->shutting_down;
    fb_sfxRuntimeUnlock();
    return ready;
}

static void *aros_audioWorker(void *unused)
{
    (void)unused;

    while (!g_aros_audio_thread_stop)
    {
        if (!aros_workerCanMix() || fb_sfxForegroundFeedActive())
        {
            usleep(1000);
            continue;
        }

        fb_sfxUpdate(g_aros_audio_worker_frames);
        usleep(1000);
    }

    return NULL;
}

int fb_sfxArosWorkerStart(int buffer_frames)
{
    if (g_aros_audio_thread_valid)
    {
        if (!g_aros_audio_thread_stop)
            return 0;
        if (pthread_equal(g_aros_audio_thread, pthread_self()))
            return -1;
        pthread_join(g_aros_audio_thread, NULL);
        g_aros_audio_thread_valid = FALSE;
    }

    g_aros_audio_worker_frames = aros_workerFrames(buffer_frames);
    g_aros_audio_thread_stop = FALSE;
    if (pthread_create(&g_aros_audio_thread, NULL,
        aros_audioWorker, NULL) != 0)
    {
        return -1;
    }

    g_aros_audio_thread_valid = TRUE;
    return 0;
}

void fb_sfxArosWorkerStop(void)
{
    if (!g_aros_audio_thread_valid)
        return;

    g_aros_audio_thread_stop = TRUE;
    if (pthread_equal(g_aros_audio_thread, pthread_self()))
        return;

    pthread_join(g_aros_audio_thread, NULL);
    g_aros_audio_thread_valid = FALSE;
}

/* end of sfx_unix.c */
