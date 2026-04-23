/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_shutdown.c

    Purpose:

        Implement the shutdown logic for the FreeBASIC sound subsystem.

        This file performs the orderly teardown of the runtime sound
        environment and releases all resources allocated during
        subsystem initialization.

    Responsibilities:

        • shut down the active audio driver
        • stop the capture subsystem
        • shut down mixer and buffer systems
        • release the runtime context

    This file intentionally does NOT contain:

        • mixer implementation
        • audio driver implementations
        • BASIC command handlers
        • audio synthesis logic

    Architectural overview:

        runtime shutdown
              │
              ▼
        stop audio driver
              │
              ▼
        shutdown capture system
              │
              ▼
        shutdown buffers and mixer
              │
              ▼
        release runtime context

    Design note:

        Shutdown occurs in the reverse order of initialization
        to avoid dependency violations between subsystems.
*/

#include <stdio.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"
#include "fb_sfx_driver.h"
#include "fb_sfx_mixer.h"
#include "fb_sfx_buffer.h"
#include "fb_sfx_capture.h"


/* ------------------------------------------------------------------------- */
/* Sound subsystem shutdown                                                  */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxExit()

    Shut down the sound subsystem and release runtime resources.

    Shutdown order:

        1. stop active audio driver
        2. stop capture subsystem
        3. shut down mixer
        4. release buffers
        5. free runtime context
*/

void fb_sfxExit(void)
{
    fb_sfxRuntimeLock();

    if (__fb_sfx == NULL)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    if (__fb_sfx->initialized)
    {
        SFX_DEBUG("sfx_shutdown: shutting down sound subsystem");
        fb_sfxExitCore();
    }

    fb_sfxFreeContext();

    SFX_DEBUG("sfx_shutdown: sound subsystem shutdown complete");
    fb_sfxRuntimeUnlock();
}


/* ------------------------------------------------------------------------- */
/* Emergency shutdown                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxAbort()

    Emergency shutdown used in situations where the subsystem
    must be forcibly stopped.

    This routine avoids complex teardown logic and simply
    disables the active driver before freeing runtime memory.
*/

void fb_sfxAbort(void)
{
    fb_sfxRuntimeLock();

    if (!__fb_sfx)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    SFX_DEBUG("sfx_shutdown: emergency shutdown");
    fb_sfxRuntimeUnlock();

    fb_sfxMidiStop();
    fb_sfxDriverShutdown();

    fb_sfxRuntimeLock();
    if (__fb_sfx)
        __fb_sfx->initialized = 0;

    fb_sfxFreeContext();
    fb_sfxRuntimeUnlock();
}


/* end of sfx_shutdown.c */
