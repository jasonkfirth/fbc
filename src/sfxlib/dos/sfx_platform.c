/* FreeBASIC Sound Library: dos/sfx_platform.c
 *
 * Feed the DOS mixer from a real preemptive rtlib thread when the optional
 * provider is linked. The worker owns background updates, observes foreground
 * playback, and joins before sound buffers are released. Mixing stays out of
 * the timer ISR. Single-threaded DOS builds use the existing delay hook.
 */

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"

#if FB_SFX_DOS_THREADS

static FBTHREAD *audio_thread;
static int audio_stop;

static void audio_worker(void *unused)
{
    (void)unused;
    for (;;)
    {
        int frames, active, blocks;
        fb_sfxRuntimeLock();
        if (audio_stop)
        {
            fb_sfxRuntimeUnlock();
            return;
        }
        active = __fb_sfx && __fb_sfx->initialized && !fb_sfxForegroundFeedActive();
        frames = active ? __fb_sfx->buffer_frames : 0;
        /* Small writes keep stop/foreground transitions responsive. The
         * driver controls actual pacing through hardware completion.
         */
        if (frames > 1024)
            frames = 1024;
        blocks = active && __fb_sfx->driver &&
            (__fb_sfx->driver->capabilities & FB_SFX_DRIVER_CAP_BLOCKING);
        fb_sfxRuntimeUnlock();
        if (active && frames > 0 && blocks)
            fb_sfxUpdate(frames);
        else
            fb_Delay(10);
    }
}

int fb_sfxMsdosStartWorker(void)
{
    fb_sfxRuntimeLock();
    if (!audio_thread)
    {
        audio_stop = 0;
        audio_thread = fb_ThreadCreate(audio_worker, NULL, 0);
    }
    fb_sfxRuntimeUnlock();
    return audio_thread ? 0 : -1;
}

int fb_sfxMsdosWorkerActive(void)
{
    int active;
    fb_sfxRuntimeLock();
    active = audio_thread != NULL && !audio_stop && __fb_sfx &&
        __fb_sfx->driver && (__fb_sfx->driver->capabilities & FB_SFX_DRIVER_CAP_BLOCKING);
    fb_sfxRuntimeUnlock();
    return active;
}

void fb_sfxPlatformExit(void)
{
    FBTHREAD *thread;
    fb_sfxRuntimeLock();
    audio_stop = 1;
    thread = audio_thread;
    fb_sfxRuntimeUnlock();
    if (thread && thread != fb_ThreadSelf())
    {
        fb_ThreadWait(thread);
        fb_sfxRuntimeLock();
        audio_thread = NULL;
        fb_sfxRuntimeUnlock();
    }
}

#else

void fb_sfxPlatformExit(void) { }

#endif

/* end of sfx_platform.c */
