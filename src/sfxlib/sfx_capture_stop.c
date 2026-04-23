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
    if (!__fb_sfx)
        return;

    if (__fb_sfx->capture.enabled == FB_SFX_CAPTURE_STOPPED)
        return;

    fb_sfxPlatformCaptureStop();
    __fb_sfx->capture.enabled = FB_SFX_CAPTURE_STOPPED;
}


/* end of sfx_capture_stop.c */
