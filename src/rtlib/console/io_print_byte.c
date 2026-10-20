/*
 * Project: FreeBASIC runtime library
 * File: console/io_print_byte.c
 * Purpose: print [#] function (byte).
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_PrintByte ( int fnum, signed char val, int mask )
{
    FB_PRINTNUM( fnum, ((int) val), mask, "% ", "d" );
}

/*:::::*/
FBCALL void fb_PrintUByte ( int fnum, unsigned char val, int mask )
{
    FB_PRINTNUM( fnum, ((unsigned) val), mask, "%", "u" );
}

/* end of console/io_print_byte.c */
