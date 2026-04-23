/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_init.c

    Purpose:

        Implement the initialization logic for the FreeBASIC sound
        subsystem.

        This file prepares the runtime environment required by the
        audio engine before any sound commands are executed.

    Responsibilities:

        • allocate the runtime context
        • initialize debug infrastructure
        • establish default runtime configuration
        • invoke core subsystem initialization

    This file intentionally does NOT contain:

        • mixer implementation
        • driver implementations
        • BASIC command handlers
        • audio synthesis code

    Architectural overview:

        program startup
             │
             ▼
        sfx_init.c
             │
             ▼
        runtime context allocation
             │
             ▼
        sfx_core initialization
*/

#include <stdio.h>

#include "../rtlib/fb.h"
#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Sound subsystem initialization                                            */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxInit()

    Initialize the sound subsystem.

    Initialization order:

        1. allocate runtime context
        2. initialize debugging
        3. invoke core initialization

    Returns:

        0 on success
        non-zero on failure
*/

int fb_sfxInit(void)
{
    int result;

    fb_sfxRuntimeLockInit();
    fb_sfxRuntimeLock();

    if (__fb_sfx && __fb_sfx->initialized)
    {
        fb_sfxRuntimeUnlock();
        return 0;
    }

    /* allocate runtime context if necessary */
    result = fb_sfxAllocContext();

    if (result != 0)
    {
        SFX_DEBUG("sfx_init: failed to allocate runtime context");
        fb_sfxRuntimeUnlock();
        return -1;
    }

    /* initialize debugging system */
    fb_sfxDebugInit();

    if (__fb_ctx.exit_sfxlib == NULL)
        __fb_ctx.exit_sfxlib = fb_sfxExit;

    SFX_DEBUG("sfx_init: starting sound subsystem");

    /* perform core subsystem initialization */
    result = fb_sfxInitCore();

    if (result != 0)
    {
        SFX_DEBUG("sfx_init: core initialization failed");
        fb_sfxFreeContext();
        fb_sfxRuntimeUnlock();
        return -1;
    }

    SFX_DEBUG("sfx_init: sound subsystem ready");

    fb_sfxRuntimeUnlock();
    return 0;
}


/* ------------------------------------------------------------------------- */
/* Lazy initialization helper                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxEnsureInit()

    Ensure that the sound subsystem has been initialized.

    This helper allows modules to safely request initialization
    without requiring the caller to know whether the subsystem
    has already started.
*/

int fb_sfxEnsureInit(void)
{
    if (__fb_sfx && __fb_sfx->initialized)
        return 0;

    return fb_sfxInit();
}


/* end of sfx_init.c */
