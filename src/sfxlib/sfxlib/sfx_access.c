/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_access.c

    Purpose:

        Provide safe access helpers for the global sound runtime state.

        Many parts of the sound subsystem rely on the global runtime
        context (__fb_sfx).  This file provides centralized helpers
        that validate access to the runtime state before it is used.

    Responsibilities:

        • validate that the sound subsystem is initialized
        • provide shared access checks used throughout sfxlib
        • prevent undefined behavior from accessing an uninitialized
          runtime context

    This file intentionally does NOT contain:

        • mixer logic
        • audio driver implementations
        • command layer code
        • buffer management

    Those components are implemented in other modules.

    Architectural note:

        This file plays a similar role to gfx_access.c in gfxlib2.
        It ensures that code interacting with the runtime state
        does not accidentally operate before initialization.
*/

#include <stdio.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Runtime access validation                                                 */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxEnsureInitialized()

    Verify that the sound subsystem has been initialized before it is used.

    Many runtime functions assume that the global context pointer exists
    and that the subsystem has completed initialization.  This helper
    prevents accidental access when those assumptions are not valid.

    Returns:

        1 if the subsystem is ready
        0 if the subsystem is not initialized
*/

int fb_sfxEnsureInitialized(void)
{
    if (fb_sfxEnsureInit() != 0)
    {
        SFX_DEBUG("sfx_access: sound subsystem initialization failed");
        return 0;
    }

    return 1;
}


/* ------------------------------------------------------------------------- */
/* Runtime context accessor                                                  */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxGetContext()

    Return the global runtime context.

    This helper allows modules to obtain the runtime pointer while
    ensuring that the subsystem has been initialized.
*/

FB_SFXCTX *fb_sfxGetContext(void)
{
    if (!fb_sfxEnsureInitialized())
        return NULL;

    return __fb_sfx;
}


/* ------------------------------------------------------------------------- */
/* Driver polling                                                            */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxPoll()

    Some audio drivers require periodic polling to process internal
    events or refill output buffers.

    This helper calls the driver poll function if the active driver
    supports polling.
*/

void fb_sfxPoll(void)
{
    const SFXDRIVER *driver;

    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxRuntimeLock();
    driver = __fb_sfx->driver;
    if (driver && driver->poll)
        driver->poll();
    fb_sfxRuntimeUnlock();
}


/* end of sfx_access.c */
