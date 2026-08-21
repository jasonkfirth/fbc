/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_getsize.c

    Purpose:

        Report the Windows CE logical text-grid dimensions.

    Responsibilities:

        - return the current logical column and row counts
        - support callers interested in only one dimension

    This file intentionally does NOT contain:

        - physical display measurement
        - desktop console queries
        - resizing policy
*/

#include "../fb.h"
#include "fb_private_console.h"

FBCALL void fb_ConsoleGetSize( int *columns, int *rows )
{
	if( columns != NULL )
		*columns = __fb_con.columns;
	if( rows != NULL )
		*rows = __fb_con.rows;
}

/* end of wince/io_getsize.c */
