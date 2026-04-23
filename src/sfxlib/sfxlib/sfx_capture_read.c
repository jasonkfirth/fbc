/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_capture_read.c

    Purpose:

        Implement the capture read interface.

        This module allows sfxlib to retrieve audio samples from
        the active capture device (microphone, line-in, or mixer
        loopback depending on platform capabilities).

        The capture subsystem is primarily used for:

            • recording
            • visualization
            • testing audio output
            • diagnostic tools

    Responsibilities:

        • read captured audio samples from the platform driver
        • provide a stable interface for the runtime
        • validate input parameters

    This file intentionally does NOT contain:

        • platform-specific capture implementations
        • audio file writing
        • mixer logic

    Architectural overview:

        capture device
              │
        platform driver
              │
        sfx_capture_read
              │
        runtime / BASIC program
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>


/* ------------------------------------------------------------------------- */
/* External capture implementation                                           */
/* ------------------------------------------------------------------------- */

/*
    Platform drivers provide the actual capture implementation.
*/

extern int fb_sfxCaptureRead(float *buffer, int frames);


/* ------------------------------------------------------------------------- */
/* Capture read                                                              */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxCaptureReadSamples()

    Read audio samples from the capture device.

    buffer
        Destination buffer for captured samples.

    frames
        Number of frames to read.

    Returns
        Number of frames read or negative on error.
*/

int fb_sfxCaptureReadSamples(float *buffer, int frames)
{
    int result;

    if (!buffer)
        return -1;

    if (frames <= 0)
        return -1;

    result = fb_sfxPlatformCaptureRead(buffer, frames);
    if (result != 0)
        return result;

    return fb_sfxCaptureRead(buffer, frames);
}


/* ------------------------------------------------------------------------- */
/* Capture probe                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxCaptureProbe()

    Attempt to read a small number of samples to verify
    that capture is functioning.

    This function is primarily used for testing.
*/

int fb_sfxCaptureProbe(void)
{
    float test_buffer[64];

    return fb_sfxCaptureReadSamples(test_buffer, 32);
}


/* end of sfx_capture_read.c */
