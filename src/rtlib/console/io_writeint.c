/*
 * Project: FreeBASIC runtime library
 * File: console/io_writeint.c
 * Purpose: write [#] functions.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_WriteInt ( int fnum, int val, int mask )
{
    FB_WRITENUM( fnum, val, mask, "%d" );
}

/*:::::*/
FBCALL void fb_WriteUInt ( int fnum, unsigned int val, int mask )
{
    FB_WRITENUM( fnum, val, mask, "%u" );
}

/* end of console/io_writeint.c */
