/*
    FreeBASIC gfxlib2 Haiku backend
    File: haiku_lock.cpp
    Purpose: Serialize runtime drawing and consume native resize requests.
    Responsibilities: Own the driver mutex and present frames on unlock.
    This file contains no native window callbacks or framebuffer allocation.
*/

#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"
#include "haiku_debug.h"

#include <OS.h>

/* ------------------------------------------------------------------------- */
/* Internal mutex state                                                      */
/* ------------------------------------------------------------------------- */

static FBMUTEX *haiku_mutex = NULL;
static int lock_initialized = 0;

/* ------------------------------------------------------------------------- */
/* Ensure mutex exists                                                       */
/* ------------------------------------------------------------------------- */

static void fb_hHaikuEnsureLock(void)
{
    if (lock_initialized)
        return;

    lock_initialized = 1;

    haiku_mutex = fb_MutexCreate();

    if (!haiku_mutex)
        HAIKU_DEBUG("Mutex creation failed");
    else
        HAIKU_DEBUG("Mutex created");
}

/* ------------------------------------------------------------------------- */
/* Lock                                                                      */
/* ------------------------------------------------------------------------- */

void fb_hHaikuLock(void)
{
    fb_hHaikuInitDebug();

    fb_hHaikuEnsureLock();

    if (!haiku_mutex)
        return;

    fb_MutexLock(haiku_mutex);

    /*
        BView hooks run with the BWindow locked. They publish dimensions under
        backend_lock instead of taking this mutex, because presentation takes
        the driver mutex before the BWindow lock. Consume the coalesced request
        here so the generic resize layer observes it under the driver lock.
    */
    fb_hHaikuLockState();
    if (__fb_gfx)
    {
        fb_hMemCpy(__fb_gfx->key, fb_haiku.key_state, sizeof(fb_haiku.key_state));
        while (fb_haiku.key_head != fb_haiku.key_tail)
        {
            fb_hPostKey(fb_haiku.pending_keys[fb_haiku.key_head]);
            fb_haiku.key_head = (fb_haiku.key_head + 1) % MAX_EVENTS;
        }
    }
    if (fb_haiku.pending_width > 0 && fb_haiku.pending_height > 0)
    {
        if (__fb_gfx && __fb_gfx->scanline_size > 0)
        {
            int height = (fb_haiku.pending_height - 1) /
                __fb_gfx->scanline_size + 1;
            fb_hRequestResize(fb_haiku.pending_width, height);
        }
        fb_haiku.pending_width = 0;
        fb_haiku.pending_height = 0;
    }
    fb_hHaikuUnlockState();
}

/* ------------------------------------------------------------------------- */
/* Unlock                                                                    */
/* ------------------------------------------------------------------------- */

void fb_hHaikuUnlock(void)
{
    if (!haiku_mutex)
        return;

    /* --------------------------------------------------------------------- */
    /* Present framebuffer BEFORE releasing lock                             */
    /* --------------------------------------------------------------------- */

    fb_hHaikuUpdate();

    fb_MutexUnlock(haiku_mutex);
}

/* ------------------------------------------------------------------------- */
/* Destroy lock                                                              */
/* ------------------------------------------------------------------------- */

void fb_hHaikuDestroyLock(void)
{
    if (haiku_mutex)
    {
        fb_MutexDestroy(haiku_mutex);
        haiku_mutex = NULL;
        lock_initialized = 0;

        HAIKU_DEBUG("Mutex destroyed");
    }
}

#endif

/* end of haiku_lock.cpp */
