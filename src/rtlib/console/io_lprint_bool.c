/*
 * Project: FreeBASIC runtime library
 * File: console/io_lprint_bool.c
 * Purpose: print [#] function (boolean).
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_LPrintBool ( int fnum, char val, int mask )
{
    fb_LPrintInit();
    mask = FB_PRINT_CONVERT_BIN_NEWLINE(mask);
    FB_PRINTNUM( fnum, fb_hBoolToStr( val ), mask, "%", "s" );

}

/* end of console/io_lprint_bool.c */
