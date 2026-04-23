/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_capture_cmd.c

    Purpose:

        Provide small command-facing helpers for CAPTURE.

        The backend save routine accepts a duration in seconds because
        it was originally built as a lower-level helper.  The BASIC
        command surface is simpler: CAPTURE SAVE "file".  This file
        converts the currently buffered capture state into a reasonable
        save duration.

    Responsibilities:

        • map CAPTURE SAVE command syntax to the lower-level save helper

    This file intentionally does NOT contain:

        • capture buffering
        • platform capture drivers
        • WAV file writing details
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* CAPTURE command helpers                                                   */
/* ------------------------------------------------------------------------- */

int fb_sfxCaptureSaveCmd(const char *filename)
{
    int frames;
    int seconds;

    if (filename == NULL)
        return -1;

    frames = fb_sfxCaptureAvailable();
    if (frames <= 0)
        frames = 44100;

    seconds = (frames + 44099) / 44100;
    if (seconds <= 0)
        seconds = 1;

    return fb_sfxCaptureSave(filename, seconds);
}

/* end of sfx_capture_cmd.c */
