/*
 * Project: FreeBASIC runtime library
 * File: console/con_locate.c
 * Purpose: locate function entry point, returns current position.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL int fb_Locate( int row, int col, int cursor, int start, int stop )
{
    int new_pos;
    int res = fb_LocateEx( row, col, cursor, &new_pos );
    if( res!=FB_RTERROR_OK )
        fb_LocateEx( 0, 0, cursor, &new_pos );
    return new_pos;
}

/* end of console/con_locate.c */
