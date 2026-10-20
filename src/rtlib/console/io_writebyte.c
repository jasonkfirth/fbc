/*
 * Project: FreeBASIC runtime library
 * File: console/io_writebyte.c
 * Purpose: write [#] functions.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_WriteByte ( int fnum, signed char val, int mask )
{
    FB_WRITENUM( fnum, val, mask, "%d" );
}

/*:::::*/
FBCALL void fb_WriteUByte ( int fnum, unsigned char val, int mask )
{
    FB_WRITENUM( fnum, val, mask, "%u" );
}

/* end of console/io_writebyte.c */
