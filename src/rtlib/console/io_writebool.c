/*
 * Project: FreeBASIC runtime library
 * File: console/io_writebool.c
 * Purpose: write [#] function (boolean).
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_WriteBool ( int fnum, char val, int mask )
{
	FB_WRITENUM( fnum, fb_hBoolToStr( val ), mask, "%s" );
}

/* end of console/io_writebool.c */
