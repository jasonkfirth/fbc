/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: fb_gfx_haiku.cpp

    Purpose:

        Provide global backend state and synchronization helpers shared
        across all Haiku backend modules.
*/

#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"
#include "haiku_debug.h"

#include <Locker.h>
#include <OS.h>

#include <new>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Global backend state                                                      */
/* ------------------------------------------------------------------------- */

FB_HAIKU_STATE fb_haiku;

/* ------------------------------------------------------------------------- */
/* GUI thread id (transitional global)                                       */
/* ------------------------------------------------------------------------- */

thread_id fb_haiku_event_thread = -1;


/* ------------------------------------------------------------------------- */
/* Internal helpers                                                          */
/* ------------------------------------------------------------------------- */

static void fb_hHaikuResetSyncFields(void)
{
    fb_haiku.backend_lock = NULL;
    fb_haiku.gui_ready_sem = -1;
    fb_haiku.gui_exit_sem  = -1;
    fb_haiku.gui_thread    = -1;

    fb_haiku_event_thread  = -1;
}


/* ------------------------------------------------------------------------- */
/* Synchronization creation                                                  */
/* ------------------------------------------------------------------------- */

int fb_hHaikuCreateStateSync(void)
{
    fb_hHaikuInitDebug();

    /*
        Defensive cleanup in case a previous initialization path left
        partially-created synchronization objects behind.
    */

    fb_hHaikuDestroyStateSync();

    fb_haiku.backend_lock = new(std::nothrow) BLocker("fb_haiku_backend_lock");

    if (!fb_haiku.backend_lock)
    {
        HAIKU_DEBUG("Failed to create backend lock");
        fb_hHaikuResetSyncFields();
        return -1;
    }

    fb_haiku.gui_ready_sem = create_sem(0, "fb_haiku_gui_ready");

    if (fb_haiku.gui_ready_sem < B_OK)
    {
        HAIKU_DEBUG("Failed to create gui_ready_sem");

        delete fb_haiku.backend_lock;
        fb_hHaikuResetSyncFields();

        return -1;
    }

    fb_haiku.gui_exit_sem = create_sem(0, "fb_haiku_gui_exit");

    if (fb_haiku.gui_exit_sem < B_OK)
    {
        HAIKU_DEBUG("Failed to create gui_exit_sem");

        delete_sem(fb_haiku.gui_ready_sem);
        fb_haiku.gui_ready_sem = -1;

        delete fb_haiku.backend_lock;
        fb_hHaikuResetSyncFields();

        return -1;
    }

    HAIKU_DEBUG("Created backend synchronization primitives");

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Synchronization destruction                                               */
/* ------------------------------------------------------------------------- */

void fb_hHaikuDestroyStateSync(void)
{
    fb_hHaikuInitDebug();

    if (fb_haiku.gui_ready_sem >= B_OK)
    {
        delete_sem(fb_haiku.gui_ready_sem);
        fb_haiku.gui_ready_sem = -1;
    }

    if (fb_haiku.gui_exit_sem >= B_OK)
    {
        delete_sem(fb_haiku.gui_exit_sem);
        fb_haiku.gui_exit_sem = -1;
    }

    if (fb_haiku.backend_lock)
    {
        delete fb_haiku.backend_lock;
        fb_haiku.backend_lock = NULL;
    }

    fb_haiku.gui_thread = -1;
    fb_haiku_event_thread = -1;

    HAIKU_DEBUG("Destroyed backend synchronization primitives");
}


/* ------------------------------------------------------------------------- */
/* Shared state lock                                                         */
/* ------------------------------------------------------------------------- */

void fb_hHaikuLockState(void)
{
    if (!fb_haiku.backend_lock)
        return;

    fb_haiku.backend_lock->Lock();
}


/* ------------------------------------------------------------------------- */
/* Shared state unlock                                                       */
/* ------------------------------------------------------------------------- */

void fb_hHaikuUnlockState(void)
{
    if (!fb_haiku.backend_lock)
        return;

    fb_haiku.backend_lock->Unlock();
}


/* ------------------------------------------------------------------------- */
/* Global state constructor (safe initialization)                            */
/* ------------------------------------------------------------------------- */

__attribute__((constructor))
static void fb_hHaikuInitGlobalState(void)
{
    memset(&fb_haiku, 0, sizeof(fb_haiku));

    fb_haiku.gui_thread = -1;
    fb_haiku.gui_ready_sem = -1;
    fb_haiku.gui_exit_sem = -1;

    fb_haiku.mouse_visible = 1;
}

#endif

/* end of fb_gfx_haiku.cpp */
