/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_cls.c

    Purpose:

        Clear all or part of the Windows CE logical text page.

    Responsibilities:

        - implement text-mode CLS against the active logical page
        - honor the current VIEW PRINT region
        - return the cursor to the cleared region's upper-left cell

    This file intentionally does NOT contain:

        - graphics-mode clearing
        - desktop console fill calls
        - native window repainting
*/

#include "../fb.h"
#include "fb_private_console.h"

void fb_ConsoleClearViewRawEx( HANDLE console, int left, int top, int right,
	                           int bottom )
{
	(void)console;
	fb_hWinCEConsoleClearRegion( __fb_con.active, left, top, right, bottom );
	fb_ConsoleLocateRawEx( NULL, top, left, -1 );
}

void fb_ConsoleClear( int mode )
{
	int top;
	int bottom;

	/* Mode 1 is reserved for the graphics viewport. */
	if( mode == 1 )
		return;

	if( (mode == 2) || (mode == (int)0xFFFF0000) ) {
		top = fb_ConsoleGetTopRow();
		bottom = fb_ConsoleGetBotRow();
	} else {
		top = 0;
		bottom = __fb_con.rows - 1;
	}

	fb_ConsoleClearViewRawEx( NULL, 0, top, __fb_con.columns - 1, bottom );
}

/* end of wince/io_cls.c */
