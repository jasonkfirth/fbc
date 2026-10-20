/*
 * Project: FreeBASIC runtime library
 * File: console/io_lprint_fix.c
 * Purpose: print [#] functions.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_LPrintFixString ( int fnum, const char *s, int mask )
{
    fb_LPrintInit();
    fb_PrintFixStringEx(FB_FILE_TO_HANDLE(fnum),
                        s,
                        FB_PRINT_CONVERT_BIN_NEWLINE(mask));
}

/* end of console/io_lprint_fix.c */
