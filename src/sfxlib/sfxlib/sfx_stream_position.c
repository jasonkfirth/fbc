/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_stream_position.c

    Purpose:

        Implement the STREAM POSITION command.

        This command returns the current playback position
        within the active stream.

    Responsibilities:

        • query the current stream position
        • provide a stable interface for BASIC programs

    This file intentionally does NOT contain:

        • audio decoding logic
        • mixer algorithms
        • platform driver interaction
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>


/* ------------------------------------------------------------------------- */
/* External stream state                                                     */
/* ------------------------------------------------------------------------- */

/*
    Stream file and position are maintained by the streaming
    subsystem implemented in sfx_stream_open.c.
*/

extern FILE *g_stream_file;
extern int   g_stream_open;
extern int   g_stream_position;


/* ------------------------------------------------------------------------- */
/* STREAM POSITION                                                           */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxStreamPosition()

    Return the current position within the stream in bytes.

    If no stream is open, this function returns -1.
*/

long fb_sfxStreamPosition(void)
{
    long position;

    if (!fb_sfxEnsureInitialized())
        return -1;

    fb_sfxRuntimeLock();
    if (!g_stream_open)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    position = g_stream_position;
    fb_sfxRuntimeUnlock();

    return position;
}


/* ------------------------------------------------------------------------- */
/* STREAM POSITION RESET                                                     */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxStreamRewind()

    Reset the current stream position to the beginning.
*/

int fb_sfxStreamRewind(void)
{
    if (!fb_sfxEnsureInitialized())
        return -1;

    fb_sfxRuntimeLock();
    if (!g_stream_open)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    g_stream_position = 0;
    fb_sfxRuntimeUnlock();

    return 0;
}


/* end of sfx_stream_position.c */
