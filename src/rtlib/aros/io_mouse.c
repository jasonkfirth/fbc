/*
    FreeBASIC runtime support for AROS
    ----------------------------------

    File: aros/io_mouse.c

    Purpose:

        Provide console fallbacks behind the mouse runtime hooks.

    Responsibilities:

        - return defined values before a graphics input hook is installed
        - report the standard illegal-function error for console requests

    This file intentionally does NOT contain:

        - Intuition mouse-message handling
        - pointer visibility or clipping policy
        - gfxlib2 mouse state

    Graphics programs install the native AROS gfxlib2 hooks when SCREEN opens.
    These fallbacks satisfy programs that reference GETMOUSE or SETMOUSE while
    preserving a predictable result before graphics initialization.
*/

#include "../fb.h"

int fb_ConsoleGetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
    if (x != NULL)
        *x = -1;
    if (y != NULL)
        *y = -1;
    if (z != NULL)
        *z = -1;
    if (buttons != NULL)
        *buttons = -1;
    if (clip != NULL)
        *clip = -1;

    return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

int fb_ConsoleSetMouse(int x, int y, int cursor, int clip)
{
    (void)x;
    (void)y;
    (void)cursor;
    (void)clip;

    return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

/* end of aros/io_mouse.c */
