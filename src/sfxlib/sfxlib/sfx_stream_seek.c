/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_stream_seek.c

    Purpose:

        Implement the STREAM SEEK command.

        This command moves the playback position within the
        currently opened stream. The new position is measured
        in bytes relative to the start of the stream source.

    Responsibilities:

        • reposition the active stream
        • validate the seek position
        • update internal stream position state

    This file intentionally does NOT contain:

        • audio decoding logic
        • mixer algorithms
        • platform driver interaction
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>
#include <stdlib.h>


/* ------------------------------------------------------------------------- */
/* External stream state                                                     */
/* ------------------------------------------------------------------------- */

/*
    The stream file and position are maintained by sfx_stream_open.c.
*/

extern FILE *g_stream_file;
extern int   g_stream_frames;
extern int   g_stream_open;
extern int   g_stream_position;


/* ------------------------------------------------------------------------- */
/* STREAM SEEK                                                               */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxStreamSeek()

    Move the playback position within the currently open stream.

    position is expressed as a byte offset relative to the
    beginning of the stream source.
*/

int fb_sfxStreamSeek(long position)
{
    if (!fb_sfxEnsureInitialized())
        return -1;

    fb_sfxRuntimeLock();
    if (!g_stream_open)
    {
        fb_sfxRuntimeUnlock();
        SFX_DEBUG("sfx_stream_seek: no stream open");
        return -1;
    }

    if (position < 0)
    {
        fb_sfxRuntimeUnlock();
        SFX_DEBUG("sfx_stream_seek: invalid position");
        return -1;
    }

    if (position > g_stream_frames)
    {
        fb_sfxRuntimeUnlock();
        SFX_DEBUG("sfx_stream_seek: seek failed");
        return -1;
    }

    g_stream_position = (int)position;
    fb_sfxRuntimeUnlock();

    return 0;
}


/* end of sfx_stream_seek.c */
