/*
 * Project: FreeBASIC runtime library
 * File: console/io_print_bool.c
 * Purpose: print [#] function (boolean).
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_PrintBool ( int fnum, char val, int mask )
{
    FB_PRINTNUM( fnum, fb_hBoolToStr( val ), mask, "%", "s" );
}

/* end of console/io_print_bool.c */
