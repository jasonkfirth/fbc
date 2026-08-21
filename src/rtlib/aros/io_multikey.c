/*
    FreeBASIC runtime support for AROS
    ----------------------------------

    File: aros/io_multikey.c

    Purpose:

        Provide the console fallback behind the MULTIKEY runtime hook.

    Responsibilities:

        - reject scan-code polling while no graphics input hook is installed
        - leave the runtime error state in a defined condition

    This file intentionally does NOT contain:

        - Intuition input-message handling
        - raw-key to FreeBASIC scan-code translation
        - gfxlib2 keyboard state

    Graphics programs install the native AROS gfxlib2 hook when SCREEN opens.
    The fallback remains necessary because references to MULTIKEY are linked
    before that runtime state is known.
*/

#include "../fb.h"

int fb_ConsoleMultikey(int scancode)
{
    (void)scancode;

    fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    return FB_FALSE;
}

/* end of aros/io_multikey.c */
