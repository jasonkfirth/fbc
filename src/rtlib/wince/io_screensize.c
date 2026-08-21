/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_screensize.c

    Purpose:

        Provide screen-size compatibility helpers for the logical console.

    Responsibilities:

        - report the same text-grid size for handle and public entry points
        - ignore desktop handle values that have no Windows CE equivalent

    This file intentionally does NOT contain:

        - screen-buffer handles
        - physical pixel dimensions
        - display orientation handling
*/

#include "../fb.h"
#include "fb_private_console.h"

void fb_ConsoleGetScreenSizeEx( HANDLE console, int *columns, int *rows )
{
	(void)console;
	fb_ConsoleGetSize( columns, rows );
}

FBCALL void fb_ConsoleGetScreenSize( int *columns, int *rows )
{
	fb_ConsoleGetSize( columns, rows );
}

/* end of wince/io_screensize.c */
