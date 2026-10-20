/*
 * Project: FreeBASIC runtime library
 * File: console/io_print_longint.c
 * Purpose: print [#] function (longint).
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_PrintLongint ( int fnum, long long val, int mask )
{
	FB_PRINTNUM( fnum, val, mask, "% ", FB_LL_FMTMOD "d" );
}

/*:::::*/
FBCALL void fb_PrintULongint ( int fnum, unsigned long long val, int mask )
{
    FB_PRINTNUM( fnum, val, mask, "%", FB_LL_FMTMOD "u" );
}

/* end of console/io_print_longint.c */
