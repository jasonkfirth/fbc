/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_stream_resume.c

    Purpose:

        Implement the STREAM RESUME command.

        This command resumes playback of a stream that was
        previously paused with STREAM PAUSE.

    Responsibilities:

        • resume playback of a paused stream
        • update stream playback state safely

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
/* STREAM RESUME                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxStreamResume()

    Resume playback of the currently paused stream.
*/

void fb_sfxStreamResume(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxRuntimeLock();
    if (!g_stream_playing)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    if (!g_stream_paused)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    g_stream_paused = 0;
    fb_sfxRuntimeUnlock();
}


/* ------------------------------------------------------------------------- */
/* Status helpers                                                            */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxStreamIsResumed()

    Return non-zero if the stream is playing and not paused.
*/

int fb_sfxStreamIsResumed(void)
{
    int resumed = 1;

    fb_sfxRuntimeLock();
    if (!g_stream_playing)
    {
        fb_sfxRuntimeUnlock();
        return 0;
    }

    if (g_stream_paused)
    {
        fb_sfxRuntimeUnlock();
        return 0;
    }

    fb_sfxRuntimeUnlock();
    return resumed;
}


/* end of sfx_stream_resume.c */
