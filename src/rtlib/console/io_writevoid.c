/*
 * Project: FreeBASIC runtime library
 * File: console/io_writevoid.c
 * Purpose: write [#] functions.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_WriteVoid ( int fnum, int mask )
{
    const char *buffer;

    if( mask & FB_PRINT_NEWLINE )
    	buffer = FB_NEWLINE;

    else if( mask & FB_PRINT_PAD )
    	buffer = "\t";

    else
    	buffer = NULL;

    if( buffer != NULL )
        FB_PRINT(fnum, buffer, mask);
}

/* end of console/io_writevoid.c */
