/*
    FreeBASIC Sound Library support for RISC OS
    -------------------------------------------

    File: sfx_unix.c

    Purpose:

        Run the RISC OS audio mixer independently of the BASIC program.

    Responsibilities:

        - own the RISC OS sfxlib worker thread
        - pace mixer updates through the blocking DigitalRenderer stream
        - pause background feeding during foreground sound commands
        - provide bounded worker startup and shutdown operations

    This file intentionally does NOT contain:

        - /dev/dsp device configuration
        - PCM sample conversion
        - DigitalRenderer SWI calls
        - portable mixer implementation
*/

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "fb_sfx_riscos.h"

#include <pthread.h>
#include <unistd.h>

/* ------------------------------------------------------------------------- */
/* Worker policy                                                             */
/* ------------------------------------------------------------------------- */

#define FB_SFX_RISCOS_WORKER_MIN_FRAMES 256
#define FB_SFX_RISCOS_WORKER_MAX_FRAMES 1024

static pthread_t g_riscos_audio_thread;
static volatile int g_riscos_audio_thread_stop;
static int g_riscos_audio_thread_valid;
static int g_riscos_audio_worker_frames;

static int fb_sfxRiscosWorkerFrames(int buffer_frames)
{
    int frames;

    frames = (buffer_frames > 0)
        ? (buffer_frames / 4)
        : (FB_SFX_DEFAULT_BUFFER / 4);

    if (frames < FB_SFX_RISCOS_WORKER_MIN_FRAMES)
        frames = FB_SFX_RISCOS_WORKER_MIN_FRAMES;
    else if (frames > FB_SFX_RISCOS_WORKER_MAX_FRAMES)
        frames = FB_SFX_RISCOS_WORKER_MAX_FRAMES;

    return frames;
}

static int fb_sfxRiscosWorkerCanMix(void)
{
    int ready;

    /*
        Driver initialization starts the worker before sfx_core marks the
        subsystem initialized.  Taking the recursive runtime lock makes the
        worker wait for that transition and prevents a second lazy init.
    */

    fb_sfxRuntimeLock();
    ready = (__fb_sfx != NULL) &&
        __fb_sfx->initialized &&
        !__fb_sfx->shutting_down;
    fb_sfxRuntimeUnlock();

    return ready;
}

static void *fb_sfxRiscosAudioWorker(void *unused)
{
    (void)unused;

    while (!g_riscos_audio_thread_stop)
    {
        if (!fb_sfxRiscosWorkerCanMix() || fb_sfxForegroundFeedActive())
        {
            usleep(1000);
            continue;
        }

        /*
            UnixLib blocks /dev/dsp writes while all DigitalRenderer stream
            buffers are occupied.  That naturally paces this update loop.
            The short yield also bounds CPU use after a driver fallback.
        */

        fb_sfxUpdate(g_riscos_audio_worker_frames);
        usleep(1000);
    }

    return NULL;
}

/* ------------------------------------------------------------------------- */
/* Worker lifecycle                                                          */
/* ------------------------------------------------------------------------- */

int fb_sfxRiscosWorkerStart(int buffer_frames)
{
    int result;

    if (g_riscos_audio_thread_valid)
    {
        if (!g_riscos_audio_thread_stop)
            return 0;

        /*
            A device failure may stop the worker from within the worker
            itself.  A later driver start owns the outstanding join before it
            reuses the stop flag and thread handle.
        */

        if (pthread_equal(g_riscos_audio_thread, pthread_self()))
            return -1;

        pthread_join(g_riscos_audio_thread, NULL);
        g_riscos_audio_thread_valid = 0;
    }

    g_riscos_audio_worker_frames =
        fb_sfxRiscosWorkerFrames(buffer_frames);
    g_riscos_audio_thread_stop = 0;

    result = pthread_create(&g_riscos_audio_thread, NULL,
        fb_sfxRiscosAudioWorker, NULL);
    if (result != 0)
        return -1;

    g_riscos_audio_thread_valid = 1;
    return 0;
}

void fb_sfxRiscosWorkerStop(void)
{
    if (!g_riscos_audio_thread_valid)
        return;

    g_riscos_audio_thread_stop = 1;

    if (pthread_equal(g_riscos_audio_thread, pthread_self()))
        return;

    pthread_join(g_riscos_audio_thread, NULL);
    g_riscos_audio_thread_valid = 0;
}

/* end of sfx_unix.c */
