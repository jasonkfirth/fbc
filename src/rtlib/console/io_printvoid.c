/*
 * Project: FreeBASIC runtime library
 * File: console/io_printvoid.c
 * Purpose: print functions.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
void fb_PrintVoidEx ( FB_FILE *handle, int mask )
{
    if( mask & FB_PRINT_BIN_NEWLINE ) {

        FB_PRINT_EX(handle,
                    FB_BINARY_NEWLINE,
                    sizeof(FB_BINARY_NEWLINE)-1,
                    mask);

    } else if( mask & FB_PRINT_NEWLINE ) {

        FB_PRINT_EX(handle,
                    FB_NEWLINE,
                    sizeof(FB_NEWLINE)-1,
                    mask);

    } else if( mask & FB_PRINT_PAD ) {

        fb_PrintPadEx( handle, mask & ~FB_PRINT_HLMASK );

    }
}

/*:::::*/
FBCALL void fb_PrintVoid ( int fnum, int mask )
{
    fb_PrintVoidEx( FB_FILE_TO_HANDLE(fnum), mask );
}

/* end of console/io_printvoid.c */
