/*
 * Project: FreeBASIC runtime library
 * File: console/io_view.c
 * Purpose: view print (console, no gfx).
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <limits.h>
#include <stdint.h>

/*:::::*/
int fb_ConsoleViewEx( int toprow, int botrow, int set_cursor )
{
    int do_update = FALSE;
    int maxrow, minrow;


    minrow = 1;
    fb_GetSize( NULL, &maxrow );
    if( maxrow==0 )
        maxrow = FB_SCRN_DEFAULT_HEIGHT;

    if( toprow > 0 ) {
        do_update = TRUE;
    } else if ( toprow == 0 ) {
        do_update = TRUE;
        toprow = minrow;
    } else {
        int current_row = fb_ConsoleGetTopRow();
		int64_t next_row = (int64_t)current_row + 1;
		toprow = next_row > INT_MAX ? 0 : (int)next_row;
    }

    if( botrow > 0 ) {
        do_update = TRUE;
    } else if ( botrow == 0 ) {
        do_update = TRUE;
        botrow = maxrow;
    } else {
        int current_row = fb_ConsoleGetBotRow();
		int64_t next_row = (int64_t)current_row + 1;
		botrow = next_row > INT_MAX ? 0 : (int)next_row;
    }

    if( toprow > botrow
        || toprow < 1
        || botrow < 1
        || toprow > maxrow
        || botrow > maxrow )
    {
        /* This is an error ... */
        do_update = FALSE;
        botrow = toprow = 0;
    }

    if( do_update && toprow > 0 && botrow > 0 ) {
        fb_ConsoleSetTopBotRows( toprow - 1, botrow - 1 );
        fb_ViewUpdate( );
        if( set_cursor ) {
            /* set cursor to top row */
            fb_Locate( toprow, 1, -1, 0, 0 );
        }
    }

    /* Preserve the legacy packed row result without shifting signed values. */
    return (int)((uint32_t)toprow | ((uint32_t)botrow << 16));
}

/*:::::*/
FBCALL int fb_ConsoleView( int toprow, int botrow )
{
    return fb_ConsoleViewEx( toprow, botrow, TRUE );
}

/* end of console/io_view.c */
