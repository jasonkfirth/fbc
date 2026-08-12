/*
    FreeBASIC runtime support for RISC OS
    -------------------------------------

    File: io_mouse.c

    Purpose:

        Provide the console mouse entry points required by the runtime.

    Responsibilities:

        - return defined output values when console mouse input is unavailable
        - report the standard illegal-function error for get and set requests

    This file intentionally does NOT contain:

        - Wimp pointer polling
        - gfxlib2 mouse handling
        - application event-loop ownership

    Platform note:

        UnixLib does not turn RISC OS pointer events into a POSIX console mouse
        interface.  A future Wimp backend should replace these stubs rather
        than adding desktop event handling to the console runtime.
*/

#include "../fb.h"

int fb_ConsoleGetMouse( int *x, int *y, int *z, int *buttons, int *clip )
{
	if( x != NULL )
		*x = -1;
	if( y != NULL )
		*y = -1;
	if( z != NULL )
		*z = -1;
	if( buttons != NULL )
		*buttons = -1;
	if( clip != NULL )
		*clip = -1;

	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_ConsoleSetMouse( int x, int y, int cursor, int clip )
{
	(void)x;
	(void)y;
	(void)cursor;
	(void)clip;

	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

/* end of io_mouse.c */
