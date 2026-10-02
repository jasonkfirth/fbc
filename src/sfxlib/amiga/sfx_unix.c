/*
    FreeBASIC Sound Library support for AmigaOS
    ----------------------------------------

    File: sfx_unix.c

    Purpose:

        Feed native audio output independently of the BASIC program.

    Responsibilities:

        - own the Amiga sfxlib worker thread
        - pace updates through blocking AHI or Paula writes
        - pause during foreground sound commands
        - provide bounded and idempotent lifecycle operations

    This file intentionally does NOT contain:

        - AHI device requests
        - sample conversion
        - portable mixer implementation
*/

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "fb_sfx_amiga.h"

#include <pthread.h>
#include <proto/exec.h>
#include <proto/dos.h>

#define FB_SFX_AMIGA_WORKER_MIN_FRAMES 256
#define FB_SFX_AMIGA_WORKER_MAX_FRAMES 1024

static pthread_t g_amiga_audio_thread;
/* The mixer lock protects lifecycle flags. Joining happens outside that lock
   because the worker needs it to finish a pending update and observe stop. */
static int g_amiga_audio_thread_stop;
static int g_amiga_audio_thread_valid;
static int g_amiga_audio_thread_joining;
static int g_amiga_audio_worker_frames;

static int amiga_workerFrames(int buffer_frames)
{
    int frames;

    frames = (buffer_frames > 0)
        ? buffer_frames / 4
        : FB_SFX_DEFAULT_BUFFER / 4;
    if (frames < FB_SFX_AMIGA_WORKER_MIN_FRAMES)
        frames = FB_SFX_AMIGA_WORKER_MIN_FRAMES;
    else if (frames > FB_SFX_AMIGA_WORKER_MAX_FRAMES)
        frames = FB_SFX_AMIGA_WORKER_MAX_FRAMES;
    return frames;
}

static void *amiga_audioWorker(void *unused)
{
    (void)unused;
    SFX_DEBUG("Amiga audio worker started: task=%p", FindTask(NULL));

    while (TRUE)
    {
        int stop, ready, frames;
        fb_sfxRuntimeLock();
        stop = g_amiga_audio_thread_stop;
        frames = g_amiga_audio_worker_frames;
        ready = (__fb_sfx != NULL) && __fb_sfx->initialized &&
            !__fb_sfx->shutting_down;
        fb_sfxRuntimeUnlock();
        if (stop) break;
        if (!ready || fb_sfxForegroundFeedActive())
        {
            Delay(1);
            continue;
        }

        fb_sfxUpdate(frames);
    }

    /* The SDK's pthread trampoline treats NULL as a process exit request. */
    return &g_amiga_audio_worker_frames;
}

int fb_sfxAmigaWorkerStart(int buffer_frames)
{
    fb_sfxRuntimeLock();
    if (g_amiga_audio_thread_valid)
    {
        /* Driver failure can switch backends from this worker. Its own stop
           callback cannot join it; reuse it for the replacement backend unless
           a foreground shutdown has already claimed the join. */
        if (g_amiga_audio_thread_stop && !g_amiga_audio_thread_joining &&
            pthread_equal(g_amiga_audio_thread, pthread_self()))
        {
            g_amiga_audio_worker_frames = amiga_workerFrames(buffer_frames);
            g_amiga_audio_thread_stop = FALSE;
        }
        int result = g_amiga_audio_thread_stop ? -1 : 0;
        fb_sfxRuntimeUnlock();
        return result;
    }

    g_amiga_audio_worker_frames = amiga_workerFrames(buffer_frames);
    g_amiga_audio_thread_stop = FALSE;
    if (pthread_create(&g_amiga_audio_thread, NULL,
        amiga_audioWorker, NULL) != 0)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    g_amiga_audio_thread_valid = TRUE;
    fb_sfxRuntimeUnlock();
    return 0;
}

void fb_sfxAmigaWorkerStop(void)
{
    pthread_t worker;
    while (TRUE)
    {
        fb_sfxRuntimeLock();
        if (!g_amiga_audio_thread_valid) { fb_sfxRuntimeUnlock(); return; }
        g_amiga_audio_thread_stop = TRUE;
        worker = g_amiga_audio_thread;
        if (pthread_equal(worker, pthread_self())) { fb_sfxRuntimeUnlock(); return; }
        if (!g_amiga_audio_thread_joining) break;
        fb_sfxRuntimeUnlock();
        Delay(1);
    }
    g_amiga_audio_thread_joining = TRUE;
    fb_sfxRuntimeUnlock();
    pthread_join(worker, NULL);
    fb_sfxRuntimeLock();
    g_amiga_audio_thread_valid = FALSE;
    g_amiga_audio_thread_joining = FALSE;
    fb_sfxRuntimeUnlock();
}

/* end of sfx_unix.c */
