/*
 * Project: FreeBASIC runtime library
 * File: console/io_print_fp.c
 * Purpose: print [#] function (floating point).
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL void fb_PrintSingle( int fnum, float val, int mask )
{
	char buffer[8+1+9+1];
	fb_PrintFixString( fnum, fb_hFloat2Str( (double)val, buffer, 7, FB_F2A_ADDBLANK ), mask );
}

FBCALL void fb_PrintDouble( int fnum, double val, int mask )
{
	char buffer[16+1+9+1];
	fb_PrintFixString( fnum, fb_hFloat2Str( val, buffer, 16, FB_F2A_ADDBLANK ), mask );
}

/* end of console/io_print_fp.c */
