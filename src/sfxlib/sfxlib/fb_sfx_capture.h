/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: fb_sfx_capture.h

    Purpose:

        Define the audio capture subsystem interface used by the
        FreeBASIC sound runtime.

        The capture subsystem provides a platform-independent interface
        for recording audio from input devices such as microphones or
        line inputs.

    Responsibilities:

        • capture device control
        • runtime capture buffer management
        • safe transfer of recorded audio into the runtime system

    This file intentionally does NOT contain:

        • platform capture driver implementations
        • mixer logic
        • audio synthesis logic
        • BASIC command parsing

    Architectural overview:

        OS capture device
                │
                ▼
        platform capture driver
                │
                ▼
        runtime capture buffer
                │
                ▼
        BASIC capture commands
*/

#ifndef __FB_SFX_CAPTURE_H__
#define __FB_SFX_CAPTURE_H__

#include "fb_sfx.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ------------------------------------------------------------------------- */
/* Capture device types                                                      */
/* ------------------------------------------------------------------------- */

/*
    Capture sources supported by the runtime.

    Not all platforms support every source type.
*/

#define FB_SFX_CAPTURE_MIC      0
#define FB_SFX_CAPTURE_LINEIN   1
#define FB_SFX_CAPTURE_LOOPBACK 2



/* ------------------------------------------------------------------------- */
/* Capture state flags                                                       */
/* ------------------------------------------------------------------------- */

#define FB_SFX_CAPTURE_STOPPED  0
#define FB_SFX_CAPTURE_RUNNING  1
#define FB_SFX_CAPTURE_PAUSED   2



/* ------------------------------------------------------------------------- */
/* Capture lifecycle                                                         */
/* ------------------------------------------------------------------------- */

/*
    Initialize the capture subsystem.

    This prepares internal buffers and resets capture state.
*/

void fb_sfxCaptureInit(void);


/*
    Shutdown the capture subsystem.

    Releases resources owned by the capture system.
*/

void fb_sfxCaptureShutdown(void);



/* ------------------------------------------------------------------------- */
/* Capture control                                                           */
/* ------------------------------------------------------------------------- */

/*
    Start audio capture.

    Parameters:

        source      capture device type
        rate        requested sample rate
        channels    number of channels

    Returns:

        0 on success
        non-zero on failure
*/

int fb_sfxCaptureStart(void);


/*
    Stop audio capture.
*/

void fb_sfxCaptureStop(void);


/*
    Pause capture.

    Samples will no longer be written to the runtime buffer
    until capture is resumed.
*/

void fb_sfxCapturePause(void);


/*
    Resume capture after a pause.
*/

void fb_sfxCaptureResume(void);



/* ------------------------------------------------------------------------- */
/* Capture status                                                            */
/* ------------------------------------------------------------------------- */

/*
    Return current capture state.

    Possible values:

        FB_SFX_CAPTURE_STOPPED
        FB_SFX_CAPTURE_RUNNING
        FB_SFX_CAPTURE_PAUSED
*/

int fb_sfxCaptureStatus(void);



/* ------------------------------------------------------------------------- */
/* Capture buffer interface                                                  */
/* ------------------------------------------------------------------------- */

/*
    Write captured samples into the runtime buffer.

    This function is typically called by the platform driver
    when new samples arrive from the audio device.
*/

int fb_sfxCaptureWrite(
    const short *samples,
    int frames
);


/*
    Read samples from the runtime capture buffer.

    BASIC programs and test systems use this function to retrieve
    recorded audio data.
*/

int fb_sfxCaptureRead(
    float *samples,
    int frames
);



/* ------------------------------------------------------------------------- */
/* Capture buffer management                                                 */
/* ------------------------------------------------------------------------- */

/*
    Clear the capture buffer.

    This removes all stored audio samples.
*/

void fb_sfxCaptureBufferClear(void);


/*
    Return number of frames currently stored in the capture buffer.
*/

int fb_sfxCaptureAvailable(void);



/* ------------------------------------------------------------------------- */
/* Capture utilities                                                         */
/* ------------------------------------------------------------------------- */

/*
    Save captured samples to a file.

    Used by capture commands that allow recorded audio to be written
    to disk.
*/

int fb_sfxCaptureSave(
    const char *filename,
    int seconds
);



#ifdef __cplusplus
}
#endif

#endif

/* end of fb_sfx_capture.h */
