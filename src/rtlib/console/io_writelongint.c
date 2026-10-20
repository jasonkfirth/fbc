/*
 * Project: FreeBASIC runtime library
 * File: console/io_writelongint.c
 * Purpose: write [#] functions.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_WriteLongint ( int fnum, long long val, int mask )
{
    FB_WRITENUM( fnum, val, mask, "%" FB_LL_FMTMOD "d" );
}

/*:::::*/
FBCALL void fb_WriteULongint ( int fnum, unsigned long long val, int mask )
{
    FB_WRITENUM( fnum, val, mask, "%" FB_LL_FMTMOD "u" );
}

/* end of console/io_writelongint.c */
