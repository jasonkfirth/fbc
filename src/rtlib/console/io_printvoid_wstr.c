/*
 * Project: FreeBASIC runtime library
 * File: console/io_printvoid_wstr.c
 * Purpose: print functions.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
void fb_PrintVoidWstrEx
	(
		FB_FILE *handle,
		int mask
	)
{
    if( mask & FB_PRINT_BIN_NEWLINE )
    {
        FB_PRINTWSTR_EX( handle,
        				 FB_BINARY_NEWLINE_WSTR,
        				 sizeof( FB_BINARY_NEWLINE_WSTR ) / sizeof( FB_WCHAR ) - 1,
        				 mask );
    }
    else if( mask & FB_PRINT_NEWLINE )
    {
        FB_PRINTWSTR_EX( handle,
        				 FB_NEWLINE_WSTR,
        				 sizeof( FB_NEWLINE_WSTR ) / sizeof( FB_WCHAR ) - 1,
        				 mask );
    }
    else if( mask & FB_PRINT_PAD )
    {
        fb_PrintPadWstrEx( handle, mask & ~FB_PRINT_HLMASK );
    }
}

/*:::::*/
FBCALL void fb_PrintVoidWstr
	(
		int fnum, int mask
	)
{
    fb_PrintVoidWstrEx( FB_FILE_TO_HANDLE(fnum), mask );
}

/* end of console/io_printvoid_wstr.c */
