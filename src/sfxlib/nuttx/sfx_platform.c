/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_platform.c

    Purpose:

        Provide the NuttX target's shared sfxlib teardown hook.

    Responsibilities:

        - give the common sfxlib runtime a platform exit symbol
        - keep NuttX audio backend ownership inside sfxlib/nuttx
        - run buffered audio mixing outside the BASIC application thread

    This file intentionally does NOT contain:

        - mixer logic
        - hardware audio output
        - MIDI support
*/

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"

#include <nuttx/config.h>

#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <nuttx/atomic.h>

/* ------------------------------------------------------------------------- */
/* NuttX audio worker                                                        */
/* ------------------------------------------------------------------------- */

#define FB_SFX_NUTTX_WORKER_STACK_SIZE 8192
#define FB_SFX_NUTTX_WORKER_MIN_FRAMES 256
#define FB_SFX_NUTTX_WORKER_MAX_FRAMES 2048

static pthread_t g_nuttx_audio_thread;
static atomic_t g_nuttx_audio_thread_stop;
static int g_nuttx_audio_thread_valid;
static int g_nuttx_audio_worker_frames;

static int fb_sfxNuttXWorkerFrames(int buffer_frames)
{
    int frames;

    frames = (buffer_frames > 0)
        ? (buffer_frames / 4)
        : (FB_SFX_DEFAULT_BUFFER / 4);

    if (frames < FB_SFX_NUTTX_WORKER_MIN_FRAMES)
        frames = FB_SFX_NUTTX_WORKER_MIN_FRAMES;
    else if (frames > FB_SFX_NUTTX_WORKER_MAX_FRAMES)
        frames = FB_SFX_NUTTX_WORKER_MAX_FRAMES;

    return frames;
}

static int fb_sfxNuttXWorkerCanMix(void)
{
    int ready;

    /*
        Driver initialization starts this thread while the core still owns
        the recursive runtime lock.  Taking that lock here makes the worker
        wait until initialization is complete instead of trying to lazily
        initialize sfxlib a second time from another CPU.
    */

    fb_sfxRuntimeLock();
    ready = (__fb_sfx != NULL) &&
        __fb_sfx->initialized &&
        !__fb_sfx->shutting_down;
    fb_sfxRuntimeUnlock();

    return ready;
}

static void *fb_sfxNuttXAudioWorker(void *unused)
{
    (void)unused;

    while (!atomic_read(&g_nuttx_audio_thread_stop))
    {
        if (!fb_sfxNuttXWorkerCanMix() || fb_sfxForegroundFeedActive())
        {
            usleep(1000);
            continue;
        }

        fb_sfxUpdate(g_nuttx_audio_worker_frames);

        /*
            Hardware output normally blocks on a free pipeline buffer.  This
            short yield also bounds CPU use if the active driver changes to a
            non-blocking diagnostic backend while the worker is alive.
        */

        usleep(1000);
    }

    return NULL;
}

int fb_sfxNuttXWorkerStart(int buffer_frames)
{
    pthread_attr_t attr;
    int result;

    if (g_nuttx_audio_thread_valid)
    {
        if (!atomic_read(&g_nuttx_audio_thread_stop))
            return 0;

        /*
            A failed driver write can request shutdown from this worker.
            That self-stop cannot join its own pthread, so the next driver
            start owns the join before it clears and reuses the stop flag.
        */

        if (pthread_equal(g_nuttx_audio_thread, pthread_self()))
            return -1;

        pthread_join(g_nuttx_audio_thread, NULL);
        g_nuttx_audio_thread_valid = 0;
    }

    g_nuttx_audio_worker_frames = fb_sfxNuttXWorkerFrames(buffer_frames);
    atomic_set(&g_nuttx_audio_thread_stop, 0);

    result = pthread_attr_init(&attr);
    if (result != 0)
        return -1;

    result = pthread_attr_setstacksize(&attr,
        FB_SFX_NUTTX_WORKER_STACK_SIZE);

#if defined(CONFIG_RP23XX_RV_PIZERO_DVI_CORE1) && \
    defined(CONFIG_SMP) && (CONFIG_SMP_NCPUS > 1)
    if (result == 0)
    {
        cpu_set_t cpuset;

        /*
            CPU 1 is reserved for the DVI scanline interrupt.  Keep the audio
            worker on CPU 0 because NuttX scheduler critical sections can
            exceed the DVI scanline deadline even when the audio task itself
            runs below the DMA interrupt priority.
        */

        CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        result = pthread_attr_setaffinity_np(&attr,
            sizeof(cpuset), &cpuset);
    }
#endif

    if (result == 0)
    {
        result = pthread_create(&g_nuttx_audio_thread, &attr,
            fb_sfxNuttXAudioWorker, NULL);
    }

    pthread_attr_destroy(&attr);

    if (result != 0)
        return -1;

    g_nuttx_audio_thread_valid = 1;
    return 0;
}

void fb_sfxNuttXWorkerStop(void)
{
    if (!g_nuttx_audio_thread_valid)
        return;

    atomic_set(&g_nuttx_audio_thread_stop, 1);

    if (pthread_equal(g_nuttx_audio_thread, pthread_self()))
        return;

    pthread_join(g_nuttx_audio_thread, NULL);
    g_nuttx_audio_thread_valid = 0;
}

/* ------------------------------------------------------------------------- */
/* Platform shutdown                                                         */
/* ------------------------------------------------------------------------- */

void fb_sfxPlatformExit(void)
{
    fb_sfxNuttXWorkerStop();
}

/* end of sfx_platform.c */
