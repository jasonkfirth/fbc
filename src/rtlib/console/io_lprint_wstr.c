/*
 * Project: FreeBASIC runtime library
 * File: console/io_lprint_wstr.c
 * Purpose: print [#] functions.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_LPrintWstr
	(
		int fnum,
		const FB_WCHAR *s,
		int mask
	)
{
    fb_LPrintInit();

    fb_PrintWstrEx( FB_FILE_TO_HANDLE(fnum),
                    s,
                    FB_PRINT_CONVERT_BIN_NEWLINE(mask) );
}

/* end of console/io_lprint_wstr.c */
