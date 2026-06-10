/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_capture_start.c

    Purpose:

        Implement the CAPTURE START command.

        This command begins audio capture from the currently
        selected input device (microphone, line-in, or system
        mixer loopback depending on platform capabilities).

    Responsibilities:

        • start the audio capture subsystem
        • invoke the active driver capture implementation
        • update global capture state safely

    This file intentionally does NOT contain:

        • platform capture implementations
        • audio file recording
        • mixer logic

    Architectural overview:

        CAPTURE START
              │
        driver capture start
              │
        platform audio input
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"
#include "fb_sfx_driver.h"

#include <stdio.h>


/* ------------------------------------------------------------------------- */
/* Capture state                                                             */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* CAPTURE START                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxCaptureBegin()

    Start capturing audio input.
*/

int fb_sfxCaptureStart(void)
{
    const FB_SFX_DRIVER *driver;
    int result;

    if (!fb_sfxEnsureInitialized())
        return -1;

    fb_sfxRuntimeLock();

    if (__fb_sfx->capture.enabled == FB_SFX_CAPTURE_RUNNING)
    {
        fb_sfxRuntimeUnlock();
        return 0;
    }

    driver = __fb_sfx->driver;

    /*
        The null driver is used by automated tests and headless package
        validation to exercise the command surface without touching real
        audio hardware.  Treat CAPTURE START as a buffer-only state change
        in that mode; CAPTURE READ/SAVE will simply observe whatever samples
        were injected into the capture buffer.
    */
    if (driver == &__fb_sfxDriverNull)
    {
        __fb_sfx->capture.enabled = FB_SFX_CAPTURE_RUNNING;
        fb_sfxRuntimeUnlock();
        return 0;
    }

    fb_sfxRuntimeUnlock();

    result = fb_sfxPlatformCaptureStart();

    if (result != 0)
    {
        SFX_DEBUG("sfx_capture_start: failed to initialize capture device");
        return -1;
    }

    fb_sfxRuntimeLock();
    if (__fb_sfx)
        __fb_sfx->capture.enabled = FB_SFX_CAPTURE_RUNNING;
    fb_sfxRuntimeUnlock();

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Capture state query                                                       */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxCaptureRunning()

    Return non-zero if capture is currently active.
*/

int fb_sfxCaptureRunning(void)
{
    if (!__fb_sfx)
        return 0;

    return (__fb_sfx->capture.enabled == FB_SFX_CAPTURE_RUNNING);
}


/* end of sfx_capture_start.c */
