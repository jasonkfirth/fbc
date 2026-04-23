/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_stream_pause.c

    Purpose:

        Implement the STREAM PAUSE command.

        This command pauses playback of the currently active
        audio stream started by STREAM PLAY.

        Unlike STREAM STOP, pausing preserves the current
        playback position so playback may later resume.

    Responsibilities:

        • pause the currently playing stream
        • preserve the playback position
        • update stream playback state

    This file intentionally does NOT contain:

        • audio decoding logic
        • mixer algorithms
        • platform driver interaction
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>


/* ------------------------------------------------------------------------- */
/* External stream playback state                                            */
/* ------------------------------------------------------------------------- */

/*
    Stream playback state is maintained by sfx_stream_play.c
*/

extern int g_stream_playing;
extern int g_stream_paused;


/* ------------------------------------------------------------------------- */
/* STREAM PAUSE                                                              */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxStreamPause()

    Pause playback of the current stream.
*/

void fb_sfxStreamPause(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxRuntimeLock();
    if (!g_stream_playing)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    if (g_stream_paused)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    g_stream_paused = 1;
    fb_sfxRuntimeUnlock();
}


/* ------------------------------------------------------------------------- */
/* Status helpers                                                            */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxStreamIsPaused()

    Return non-zero if the stream is currently paused.
*/

int fb_sfxStreamIsPaused(void)
{
    int paused;

    fb_sfxRuntimeLock();
    paused = g_stream_paused;
    fb_sfxRuntimeUnlock();

    return paused;
}


/* end of sfx_stream_pause.c */
