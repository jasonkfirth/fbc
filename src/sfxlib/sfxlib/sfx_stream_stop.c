/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_stream_stop.c

    Purpose:

        Expose a public STREAM STOP entry point.

        The stream playback layer already has an internal stop helper.
        The BASIC command surface needs a stable exported routine that
        can be called directly from compiler-generated code.

    Responsibilities:

        • provide STREAM STOP as a public command entry point

    This file intentionally does NOT contain:

        • stream playback state ownership
        • stream decoding
        • stream file management
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* STREAM STOP                                                               */
/* ------------------------------------------------------------------------- */

void fb_sfxStreamStop(void)
{
    fb_sfxStreamStopInternal();
}

/* end of sfx_stream_stop.c */
