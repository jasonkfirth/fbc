/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_unix.c

    Purpose:

        Provide shared Unix backend support for sfxlib.

        This module implements the platform state and helper
        functions used by Unix audio drivers such as ALSA,
        PulseAudio, JACK, and PipeWire.

    Responsibilities:

        • maintain shared Unix backend state
        • provide initialization helpers
        • provide debug infrastructure
        • provide safe write helpers

    This file intentionally does NOT contain:

        • mixer logic
        • audio synthesis
        • driver implementations
*/

#ifndef DISABLE_UNIX

#include "fb_sfx_unix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* Global backend state                                                      */
/* ------------------------------------------------------------------------- */

FB_SFX_UNIX_STATE fb_sfx_unix =
{
    0,      /* initialized */

    44100,  /* sample_rate */
    2,      /* channels */
    512,    /* buffer_frames */

    0       /* running */
};


/* ------------------------------------------------------------------------- */
/* Debug support                                                             */
/* ------------------------------------------------------------------------- */

static int g_unix_debug_initialized = 0;
static int g_unix_debug_enabled = 0;


/*
    Initialize debug subsystem.
*/

void fb_sfxUnixInitDebug(void)
{
    const char *e;

    if (g_unix_debug_initialized)
        return;

    g_unix_debug_initialized = 1;

    e = getenv("SFXLIB_UNIX_DEBUG");

    g_unix_debug_enabled = (e && *e && *e != '0');
}


/*
    Check if debug output is enabled.
*/

int fb_sfxUnixDebugEnabled(void)
{
    fb_sfxUnixInitDebug();
    return g_unix_debug_enabled;
}


/* ------------------------------------------------------------------------- */
/* Backend lifecycle                                                         */
/* ------------------------------------------------------------------------- */

/*
    Initialize shared Unix backend state.

    This function does not open audio devices.
    Individual drivers perform that task.
*/

int fb_sfxUnixInit(void)
{
    if (fb_sfx_unix.initialized)
        return 0;

    fb_sfxUnixInitDebug();

    fb_sfx_unix.initialized = 1;
    fb_sfx_unix.running = 0;

    if (fb_sfxUnixDebugEnabled())
    {
        fprintf(stderr,
            "SFX_UNIX: initialized (rate=%d channels=%d buffer=%d)\n",
            fb_sfx_unix.sample_rate,
            fb_sfx_unix.channels,
            fb_sfx_unix.buffer_frames
        );
    }

    return 0;
}


/*
    Shutdown shared Unix backend state.
*/

void fb_sfxUnixExit(void)
{
    if (!fb_sfx_unix.initialized)
        return;

    fb_sfx_unix.running = 0;
    fb_sfx_unix.initialized = 0;

    if (fb_sfxUnixDebugEnabled())
        fprintf(stderr, "SFX_UNIX: shutdown\n");
}


/* ------------------------------------------------------------------------- */
/* Audio output helper                                                       */
/* ------------------------------------------------------------------------- */

/*
    Write audio frames.

    This helper exists so drivers can route audio
    through a shared interface if desired.
*/

int fb_sfxUnixWrite(float *buffer, int frames)
{
    (void)buffer;
    (void)frames;

    if (!fb_sfx_unix.initialized)
        return -1;

    if (!fb_sfx_unix.running)
        return -1;

    /*
        Default implementation does nothing.

        Actual drivers override this behavior.
    */

    return frames;
}


/* ------------------------------------------------------------------------- */
/* Status helper                                                             */
/* ------------------------------------------------------------------------- */

int fb_sfxUnixRunning(void)
{
    return fb_sfx_unix.running;
}

#endif

/* end of sfx_unix.c */
