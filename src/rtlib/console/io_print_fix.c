/*
 * Project: FreeBASIC runtime library
 * File: console/io_print_fix.c
 * Purpose: print [#] functions.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
static void fb_hPrintStrEx( FB_FILE *handle, const char *s, size_t len, int mask )
{
    if( len != 0 ) {
        FB_PRINT_EX(handle, s, len, 0);
    }

    fb_PrintVoidEx( handle, mask );
}

/*:::::*/
void fb_PrintFixStringEx ( FB_FILE *handle, const char *s, int mask )
{
    if( s == NULL )
    	fb_PrintVoidEx( handle, mask );
    else
    	fb_hPrintStrEx( handle, s, strlen( s ), mask );
}

/*:::::*/
FBCALL void fb_PrintFixString ( int fnum, const char *s, int mask )
{
    fb_PrintFixStringEx(FB_FILE_TO_HANDLE(fnum), s, mask);
}

/* end of console/io_print_fix.c */
