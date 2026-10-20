/*
 * Project: FreeBASIC runtime library
 * File: console/io_lprintvoid.c
 * Purpose: print functions.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_LPrintVoid ( int fnum, int mask )
{
    fb_LPrintInit();
    fb_PrintVoidEx( FB_FILE_TO_HANDLE(fnum),
                    FB_PRINT_CONVERT_BIN_NEWLINE(mask) );
}

/* end of console/io_lprintvoid.c */
