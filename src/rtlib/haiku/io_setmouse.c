/*
    FreeBASIC runtime support
    -------------------------

    File: io_setmouse.c

    Purpose:

        Provide the console mouse setter entry point required by the
        generic runtime hook layer on Haiku.

    Responsibilities:

        • satisfy the fb_ConsoleSetMouse() runtime symbol
        • return a stable "unsupported" error for console-mode callers

    This file intentionally does NOT contain:

        • graphics mouse handling
        • cursor rendering
        • window-system integration

    Notes:

        Graphics-mode mouse control is handled through the gfxlib hook
        installed by gfx_screen.c. This file exists only for callers that
        reach the console fallback path before graphics hooks are active.
*/

#ifndef DISABLE_HAIKU

#include "../fb.h"

int fb_ConsoleSetMouse( int x, int y, int cursor, int clip )
{
    (void)x;
    (void)y;
    (void)cursor;
    (void)clip;

    return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

#endif

/* end of io_setmouse.c */
