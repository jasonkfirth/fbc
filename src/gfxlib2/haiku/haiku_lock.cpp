
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
