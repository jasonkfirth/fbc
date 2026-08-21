/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_width.c

    Purpose:

        Resize the Windows CE logical text grid.

    Responsibilities:

        - return the grid size that was active on entry
        - preserve an omitted dimension
        - resize every allocated logical page atomically

    This file intentionally does NOT contain:

        - desktop console-window resizing
        - physical display-mode changes
        - graphics-mode WIDTH handling
*/

#include "../fb.h"
#include "fb_private_console.h"

int fb_ConsoleWidth( int columns, int rows )
{
	int previous;

	previous = __fb_con.columns | (__fb_con.rows << 16);
	if( (columns <= 0) && (rows <= 0) )
		return previous;

	if( columns <= 0 )
		columns = __fb_con.columns;
	if( rows <= 0 )
		rows = __fb_con.rows;

	if( !fb_hWinCEConsoleResize( columns, rows ) )
		fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );

	return previous;
}

/* end of wince/io_width.c */
