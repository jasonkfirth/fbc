/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_getxy.c

    Purpose:

        Return the process-local Windows CE console cursor position.
*/

#include "../fb.h"
#include "fb_private_console.h"

void fb_ConsoleGetRawXYEx( HANDLE console, int *column, int *row )
{
	(void)console;
	if( column != NULL )
		*column = __fb_con.cursor_x;
	if( row != NULL )
		*row = __fb_con.cursor_y;
}

void fb_ConsoleGetRawXY( int *column, int *row )
{
	fb_ConsoleGetRawXYEx( NULL, column, row );
}

FBCALL void fb_ConsoleGetXY( int *column, int *row )
{
	if( column != NULL )
		*column = __fb_con.cursor_x + 1;
	if( row != NULL )
		*row = __fb_con.cursor_y + 1;
}

/* end of wince/io_getxy.c */
