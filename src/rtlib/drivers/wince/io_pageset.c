/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/wince/io_pageset.c

    Purpose:

        Select active and visible Windows CE logical console pages.

    Responsibilities:

        - allocate pages on first use
        - preserve omitted active and visible page selections
        - return the previous packed page pair

    This file intentionally does NOT contain: - desktop console-screen-buffer creation
        - native page presentation
        - graphics page selection
*/

#include "../../fb.h"
#include "fb_private_console.h"

HANDLE fb_hConsoleCreateBuffer( void )
{
	return fb_hWinCEConsoleEnsurePage( fb_console_context.active ) ? (HANDLE)1 : NULL;
}

int fb_ConsolePageSet( int active, int visible )
{
	int previous;

	previous = fb_console_context.active | (fb_console_context.visible << 8);
	if( (active >= 0) && !fb_hWinCEConsoleEnsurePage( active ) )
		return -1;
	if( (visible >= 0) && !fb_hWinCEConsoleEnsurePage( visible ) )
		return -1;

	if( active >= 0 )
		fb_console_context.active = active;
	if( visible >= 0 )
		fb_console_context.visible = visible;

	return previous;
}

/* end of drivers/wince/io_pageset.c */
