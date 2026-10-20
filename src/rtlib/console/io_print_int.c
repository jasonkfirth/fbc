/*
 * Project: FreeBASIC runtime library
 * File: console/io_print_int.c
 * Purpose: print [#] function (int).
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_PrintInt ( int fnum, int val, int mask )
{
    FB_PRINTNUM( fnum, val, mask, "% ", "d" );
}

/*:::::*/
FBCALL void fb_PrintUInt ( int fnum, unsigned int val, int mask )
{
    FB_PRINTNUM( fnum, val, mask, "%", "u" );
}

/* end of console/io_print_int.c */
