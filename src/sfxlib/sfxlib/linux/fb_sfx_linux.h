/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: fb_sfx_linux.h

    Purpose:

        Define the Linux platform backend interface for sfxlib.

        This header provides the shared state and function
        declarations used by Linux audio drivers such as
        ALSA, PulseAudio, and PipeWire.

    Responsibilities:

        • define Linux backend state
        • declare driver lifecycle helpers
        • provide shared debugging interface
        • provide Linux-specific audio helpers

    This file intentionally does NOT contain:

        • mixer logic
        • audio synthesis
        • driver implementations
*/

#ifndef __FB_SFX_LINUX_H__
#define __FB_SFX_LINUX_H__

#ifndef DISABLE_LINUX

#include "../fb_sfx_driver.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ------------------------------------------------------------------------- */
/* Backend state                                                             */
/* ------------------------------------------------------------------------- */

/*
    FB_SFX_LINUX_STATE

    Shared state used by Linux audio backends.
*/

typedef struct FB_SFX_LINUX_STATE
{
    int initialized;

    int sample_rate;
    int channels;
    int buffer_frames;

    int running;

} FB_SFX_LINUX_STATE;


/* ------------------------------------------------------------------------- */
/* Global backend state                                                      */
/* ------------------------------------------------------------------------- */

extern FB_SFX_LINUX_STATE fb_sfx_linux;


/* ------------------------------------------------------------------------- */
/* Driver lifecycle                                                          */
/* ------------------------------------------------------------------------- */

/*
    Initialize Linux backend state.

    This does not open audio devices. Individual drivers
    perform device initialization.
*/

int  fb_sfxLinuxInit(void);

/*
    Activate Linux audio pumping for the current backend.
*/

int  fb_sfxLinuxActivate(int rate, int channels, int buffer_frames);

/*
    Mark the current Linux backend as inactive without tearing
    down the shared worker infrastructure.
*/

void fb_sfxLinuxDeactivate(void);

/*
    Shutdown Linux backend state.
*/

void fb_sfxLinuxExit(void);


/* ------------------------------------------------------------------------- */
/* Audio output                                                              */
/* ------------------------------------------------------------------------- */

/*
    Write audio frames to the active Linux audio backend.

    Drivers may override this helper depending on
    the audio system used.
*/

int fb_sfxLinuxWrite(float *buffer, int frames);


/* ------------------------------------------------------------------------- */
/* Driver status                                                             */
/* ------------------------------------------------------------------------- */

/*
    Returns non-zero if the backend is currently running.
*/

int fb_sfxLinuxRunning(void);


/* ------------------------------------------------------------------------- */
/* Debug helpers                                                             */
/* ------------------------------------------------------------------------- */

/*
    Initialize debugging subsystem.
*/

void fb_sfxLinuxInitDebug(void);

/*
    Returns non-zero if debug logging is enabled.
*/

int fb_sfxLinuxDebugEnabled(void);


#ifdef __cplusplus
}
#endif

#endif
#endif

/* end of fb_sfx_linux.h */
