/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/wince/io_window.c

    Purpose:

        Map FreeBASIC console coordinates onto the WinCE logical console.

    Responsibilities:

        - report the process-local text grid dimensions
        - translate between one-based public and zero-based internal positions
        - preserve the desktop console-window hook contract as no-ops

    This file intentionally does NOT contain: - desktop screen-buffer APIs
        - scrollback-window tracking
        - native window creation
*/

#include "../../fb.h"
#include "fb_private_console.h"

FBCALL void fb_hUpdateConsoleWindow( void )
{
}

void fb_InitConsoleWindow( void )
{
}

FBCALL void fb_hRestoreConsoleWindow( void )
{
}

void fb_ConsoleGetMaxWindowSize( int *columns, int *rows )
{
	if( columns != NULL )
		*columns = fb_console_context.columns;
	if( rows != NULL )
		*rows = fb_console_context.rows;
}

FBCALL void fb_hConvertToConsole( int *left, int *top, int *right,
	                              int *bottom )
{
	if( left != NULL )
		--*left;
	if( top != NULL )
		--*top;
	if( right != NULL )
		--*right;
	if( bottom != NULL )
		--*bottom;
}

FBCALL void fb_hConvertFromConsole( int *left, int *top, int *right,
	                                int *bottom )
{
	if( left != NULL )
		++*left;
	if( top != NULL )
		++*top;
	if( right != NULL )
		++*right;
	if( bottom != NULL )
		++*bottom;
}

void fb_hConsoleGetWindow( int *left, int *top, int *columns, int *rows )
{
	if( left != NULL )
		*left = 0;
	if( top != NULL )
		*top = 0;
	if( columns != NULL )
		*columns = fb_console_context.columns;
	if( rows != NULL )
		*rows = fb_console_context.rows;
}

/* end of drivers/wince/io_window.c */
