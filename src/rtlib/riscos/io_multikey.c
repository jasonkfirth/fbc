/*
    FreeBASIC runtime support for RISC OS
    -------------------------------------

    File: io_multikey.c

    Purpose:

        Provide the MULTIKEY entry point required by the console runtime.

    Responsibilities:

        - reject keyboard-state polling until a native input backend exists
        - leave the runtime error state in a defined condition

    This file intentionally does NOT contain:

        - Wimp key-state polling
        - PC scan-code translation
        - gfxlib2 keyboard event handling

    Platform note:

        MULTIKEY expects persistent scan-code state.  UnixLib's character
        input functions cannot provide that state without consuming input, so
        pretending they can would make key handling unreliable.
*/

#include "../fb.h"

int fb_ConsoleMultikey( int scancode )
{
	(void)scancode;

	fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	return FB_FALSE;
}

/* end of io_multikey.c */
