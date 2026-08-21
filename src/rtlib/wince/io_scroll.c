/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_scroll.c

    Purpose:

        Scroll the current Windows CE VIEW PRINT region.

    Responsibilities:

        - move active-page cells upward by complete text rows
        - clear newly exposed rows with the current color attribute

    This file intentionally does NOT contain:

        - desktop ScrollConsoleScreenBuffer calls
        - pixel scrolling
        - view-region selection
*/

#include "../fb.h"
#include "fb_private_console.h"

void fb_ConsoleScroll( int rows )
{
	int top;
	int bottom;

	if( rows <= 0 )
		return;

	top = fb_ConsoleGetTopRow();
	bottom = fb_ConsoleGetBotRow();
	fb_hWinCEConsoleScrollRegion( __fb_con.active, 0, top,
	                            __fb_con.columns - 1, bottom, rows );
}

/* end of wince/io_scroll.c */
