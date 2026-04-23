/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: fb_sfx_unix.h

    Purpose:

        Define the Unix platform backend interface for sfxlib.

        This header provides the shared state and function
        declarations used by Unix audio drivers such as ALSA,
        PulseAudio, JACK, or PipeWire.

    Responsibilities:

        • define Unix backend state
        • provide driver lifecycle declarations
        • provide shared debugging utilities

    This file intentionally does NOT contain:

        • mixer logic
        • audio synthesis
        • platform driver implementations
*/

#ifndef __FB_SFX_UNIX_H__
#define __FB_SFX_UNIX_H__

#ifndef DISABLE_UNIX

#include "../fb_sfx_driver.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ------------------------------------------------------------------------- */
/* Backend state                                                             */
/* ------------------------------------------------------------------------- */

/*
    FB_SFX_UNIX_STATE

    Shared state used by Unix audio backends.
*/

typedef struct FB_SFX_UNIX_STATE
{
    int initialized;

    int sample_rate;
    int channels;
    int buffer_frames;

    int running;

} FB_SFX_UNIX_STATE;


/* ------------------------------------------------------------------------- */
/* Global backend state                                                      */
/* ------------------------------------------------------------------------- */

extern FB_SFX_UNIX_STATE fb_sfx_unix;


/* ------------------------------------------------------------------------- */
/* Driver lifecycle                                                          */
/* ------------------------------------------------------------------------- */

int  fb_sfxUnixInit(void);
void fb_sfxUnixExit(void);


/* ------------------------------------------------------------------------- */
/* Audio output                                                              */
/* ------------------------------------------------------------------------- */

int fb_sfxUnixWrite(float *buffer, int frames);


/* ------------------------------------------------------------------------- */
/* Driver status                                                             */
/* ------------------------------------------------------------------------- */

int fb_sfxUnixRunning(void);


/* ------------------------------------------------------------------------- */
/* Debug helpers                                                             */
/* ------------------------------------------------------------------------- */

void fb_sfxUnixInitDebug(void);
int  fb_sfxUnixDebugEnabled(void);


#ifdef __cplusplus
}
#endif

#endif
#endif
