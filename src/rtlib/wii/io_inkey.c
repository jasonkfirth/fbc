/*
    FreeBASIC runtime Wii console keyboard input
    --------------------------------------------

    File: io_inkey.c

    Purpose:

        Provide safe console input stubs for Wii targets.

    Responsibilities:

        - keep INKEY$/GETKEY/KEYHIT linkable on a console without a keyboard
        - return no-key state without blocking the program

    This file intentionally does NOT contain:

        - controller input polling
        - USB keyboard support
        - gfxlib event delivery
*/

#include "../fb.h"

FBSTRING *fb_ConsoleInkey(void)
{
	return 0;
}

int fb_ConsoleGetkey(void)
{
	return 0;
}

int fb_ConsoleKeyHit(void)
{
	return 0;
}

/* end of io_inkey.c */
