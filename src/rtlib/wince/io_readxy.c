/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_readxy.c

    Purpose:

        Implement text-mode SCREEN character and color queries on Windows CE.

    Responsibilities:

        - translate one-based BASIC coordinates to logical-page cells
        - return either the stored UTF-16 code unit or packed color attribute
        - reject coordinates outside the active text grid

    This file intentionally does NOT contain:

        - desktop ReadConsoleOutput calls
        - graphics pixel reads
        - character encoding conversion
*/

#include "../fb.h"
#include "fb_private_console.h"

FBCALL unsigned int fb_ConsoleReadXY( int column, int row, int color_flag )
{
	FB_WINCE_CONSOLE_CELL *cell;

	cell = fb_hWinCEConsoleCell( __fb_con.active, column - 1, row - 1 );
	if( cell == NULL )
		return 0;

	return color_flag ? (unsigned int)cell->attribute
	                  : (unsigned int)cell->character;
}

/* end of wince/io_readxy.c */
