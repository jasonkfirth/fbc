/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_capture_stop.c

    Purpose:

        Implement the CAPTURE STOP command.

        This command stops audio capture previously started
        with CAPTURE START.

    Responsibilities:

        • stop the audio capture subsystem
        • invoke the active driver capture shutdown routine
        • update global capture state safely

    This file intentionally does NOT contain:

        • platform capture implementations
        • audio recording or file writing
        • mixer logic

    Architectural overview:

        CAPTURE STOP
              │
        driver capture stop
              │
        platform audio input shutdown
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"
#include "fb_sfx_driver.h"

#include <stdio.h>


/* ------------------------------------------------------------------------- */
/* CAPTURE STOP                                                              */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxCaptureEnd()

    Stop audio capture.
*/

void fb_sfxCaptureStop(void)
{
    const FB_SFX_DRIVER *driver;

    if (!fb_sfx_context)
        return;

    fb_sfxRuntimeLock();

    if (fb_sfx_context->capture.enabled == FB_SFX_CAPTURE_STOPPED)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    driver = fb_sfx_context->driver;
    fb_sfx_context->capture.enabled = FB_SFX_CAPTURE_STOPPED;

    fb_sfxRuntimeUnlock();

    if (!fb_sfxDriverIsNull(driver))
        fb_sfxPlatformCaptureStop();
}


/* end of sfx_capture_stop.c */
