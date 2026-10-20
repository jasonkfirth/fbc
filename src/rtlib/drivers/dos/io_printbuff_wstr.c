/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/io_printbuff_wstr.c
 * Purpose: low-level print to console function.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

void fb_ConsolePrintBufferWstrEx( const FB_WCHAR *buffer, size_t len, int mask )
{
    /*
    	DOS console output is byte-oriented.  Convert through the runtime narrow
    	string path before handing the buffer to the existing console writer.
    */

    char *temp = alloca( len + 1 );

    if( len > 0 )
        fb_wstr_ConvToA( temp, len, buffer );
    else
    	*temp = '\0';

    fb_ConsolePrintBufferEx( temp, len, mask );
}

void fb_ConsolePrintBufferWstr( const FB_WCHAR *buffer, int mask )
{
	fb_ConsolePrintBufferWstrEx( buffer, fb_wstr_Len( buffer ), mask );
}

/* end of drivers/dos/io_printbuff_wstr.c */
