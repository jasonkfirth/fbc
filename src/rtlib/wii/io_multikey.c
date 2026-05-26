/*
    FreeBASIC runtime Wii console MULTIKEY
    --------------------------------------

    File: io_multikey.c

    Purpose:

        Provide a safe console MULTIKEY fallback for Wii targets.

    Responsibilities:

        - keep MULTIKEY linkable outside graphics mode
        - report unsupported console keyboard state explicitly

    This file intentionally does NOT contain:

        - Wii Remote or GameCube controller polling
        - gfxlib keyboard state
*/

#include "../fb.h"

int fb_ConsoleMultikey(int scancode)
{
	(void)scancode;
	fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	return FB_FALSE;
}

/* end of io_multikey.c */
