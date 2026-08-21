/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_pageset.c

    Purpose:

        Select active and visible Windows CE logical console pages.

    Responsibilities:

        - allocate pages on first use
        - preserve omitted active and visible page selections
        - return the previous packed page pair

    This file intentionally does NOT contain:

        - desktop console-screen-buffer creation
        - native page presentation
        - graphics page selection
*/

#include "../fb.h"
#include "fb_private_console.h"

HANDLE fb_hConsoleCreateBuffer( void )
{
	return fb_hWinCEConsoleEnsurePage( __fb_con.active ) ? (HANDLE)1 : NULL;
}

int fb_ConsolePageSet( int active, int visible )
{
	int previous;

	previous = __fb_con.active | (__fb_con.visible << 8);
	if( (active >= 0) && !fb_hWinCEConsoleEnsurePage( active ) )
		return -1;
	if( (visible >= 0) && !fb_hWinCEConsoleEnsurePage( visible ) )
		return -1;

	if( active >= 0 )
		__fb_con.active = active;
	if( visible >= 0 )
		__fb_con.visible = visible;

	return previous;
}

/* end of wince/io_pageset.c */
