/*
 * Project: FreeBASIC runtime library
 * File: console/io_print_short.c
 * Purpose: print [#] function (short).
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_PrintShort ( int fnum, short val, int mask )
{
    FB_PRINTNUM( fnum, val, mask, "% ", "hd" );
}

/*:::::*/
FBCALL void fb_PrintUShort ( int fnum, unsigned short val, int mask )
{
    FB_PRINTNUM( fnum, val, mask, "%", "hu" );
}

/* end of console/io_print_short.c */
