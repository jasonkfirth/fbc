/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: fb_sfx_win32.h

    Purpose:

        Define the Windows platform backend interface for sfxlib.

        This header provides the shared state and function
        declarations used by Windows audio drivers such as
        WinMM, DirectSound, WASAPI, or other Windows audio APIs.

    Responsibilities:

        • define Win32 backend state
        • provide driver lifecycle declarations
        • provide shared debugging utilities
        • define platform interaction hooks used by Windows drivers

    This file intentionally does NOT contain:

        • mixer logic
        • sound synthesis
        • driver implementations
*/

#ifndef __FB_SFX_WIN32_H__
#define __FB_SFX_WIN32_H__

#ifndef DISABLE_WIN32

#include "../fb_sfx_driver.h"

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif


/* ------------------------------------------------------------------------- */
/* Backend state                                                             */
/* ------------------------------------------------------------------------- */

/*
    FB_SFX_WIN32_STATE

    Shared state used by Windows audio backends.

    This structure tracks the runtime configuration of the
    currently active Windows audio device.
*/

typedef struct FB_SFX_WIN32_STATE
{
    int initialized;

    int sample_rate;
    int channels;
    int buffer_frames;

    int running;

    /* Windows audio handles may be stored here by drivers */
    void *device_handle;

} FB_SFX_WIN32_STATE;


/* ------------------------------------------------------------------------- */
/* Global backend state                                                      */
/* ------------------------------------------------------------------------- */

extern FB_SFX_WIN32_STATE fb_sfx_win32;


/* ------------------------------------------------------------------------- */
/* Driver lifecycle                                                          */
/* ------------------------------------------------------------------------- */

/*
    Initialize the Windows audio subsystem.
*/

int  fb_sfxWin32Init(void);

/*
    Shut down the Windows audio subsystem.
*/

void fb_sfxWin32Exit(void);


/* ------------------------------------------------------------------------- */
/* Audio output                                                              */
/* ------------------------------------------------------------------------- */

/*
    Write audio frames to the active Windows audio device.

    buffer  - floating point audio samples
    frames  - number of frames to write
*/

int fb_sfxWin32Write(float *buffer, int frames);


/* ------------------------------------------------------------------------- */
/* Driver status                                                             */
/* ------------------------------------------------------------------------- */

/*
    Return non-zero if the Windows audio subsystem is active.
*/

int fb_sfxWin32Running(void);


/* ------------------------------------------------------------------------- */
/* Debug helpers                                                             */
/* ------------------------------------------------------------------------- */

/*
    Initialize the Windows debug system.
*/

void fb_sfxWin32InitDebug(void);

/*
    Return non-zero if debugging is enabled.
*/

int fb_sfxWin32DebugEnabled(void);


#ifdef __cplusplus
}
#endif

#endif
#endif

/* end of fb_sfx_win32.h */
