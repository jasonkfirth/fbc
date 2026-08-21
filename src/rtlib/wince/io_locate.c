/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_locate.c

    Purpose:

        Position the cursor in the Windows CE logical text grid.

    Responsibilities:

        - apply one-based LOCATE coordinates
        - preserve omitted row, column, and visibility values
        - provide zero-based raw helpers used by console internals

    This file intentionally does NOT contain:

        - desktop cursor-information calls
        - native caret creation
        - graphics cursor handling
*/

#include "../fb.h"
#include "fb_private_console.h"

static int hClamp( int value, int minimum, int maximum )
{
	if( value < minimum )
		return minimum;
	if( value > maximum )
		return maximum;
	return value;
}

int fb_ConsoleLocate( int row, int column, int cursor )
{
	int result;

	if( column < 1 )
		column = __fb_con.cursor_x + 1;
	if( row < 1 )
		row = __fb_con.cursor_y + 1;

	column = hClamp( column, 1, __fb_con.columns );
	row = hClamp( row, 1, __fb_con.rows );
	result = (column & 0xFF) | ((row & 0xFF) << 8) |
	         (__fb_con.cursor_visible ? 0x10000 : 0);

	fb_ConsoleLocateRawEx( NULL, row - 1, column - 1, cursor );
	return result;
}

void fb_ConsoleLocateRawEx( HANDLE console, int row, int column, int cursor )
{
	(void)console;
	if( column >= 0 )
		__fb_con.cursor_x = hClamp( column, 0, __fb_con.columns - 1 );
	if( row >= 0 )
		__fb_con.cursor_y = hClamp( row, 0, __fb_con.rows - 1 );
	if( cursor >= 0 )
		__fb_con.cursor_visible = (cursor != 0);
}

FBCALL void fb_ConsoleLocateRaw( int row, int column, int cursor )
{
	fb_ConsoleLocateRawEx( NULL, row, column, cursor );
}

/* end of wince/io_locate.c */
