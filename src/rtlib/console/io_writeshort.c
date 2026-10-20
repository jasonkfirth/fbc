/*
 * Project: FreeBASIC runtime library
 * File: console/io_writeshort.c
 * Purpose: write [#] functions.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_WriteShort ( int fnum, short val, int mask )
{
    FB_WRITENUM( fnum, val, mask, "%hd" );
}

/*:::::*/
FBCALL void fb_WriteUShort ( int fnum, unsigned short val, int mask )
{
    FB_WRITENUM( fnum, val, mask, "%hu" );
}

/* end of console/io_writeshort.c */
