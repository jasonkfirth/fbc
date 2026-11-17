/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/io_printbuff_wstr.c
 * Purpose: low-level print to console function.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"
#include <stdlib.h>

void fb_ConsolePrintBufferWstrEx( const FB_WCHAR *buffer, size_t len, int mask )
{
    /*
    	DOS console output is byte-oriented.  Convert through the runtime narrow
    	string path before handing the buffer to the existing console writer.
    */

    char local_buffer[256];
    char *temp = local_buffer;
    ssize_t converted;

    if( (len > (size_t)FB_STRSIZEMSK) || (!buffer && len != 0) )
        return;
    if( len + 1 > sizeof(local_buffer) ) {
        temp = malloc( len + 1 );
        if( temp == NULL )
            return;
    }

    if( len > 0 )
        converted = fb_wstr_ConvToA( temp, (ssize_t)len, buffer );
    else {
	    temp[0] = '\0';
        converted = 0;
    }

    if( converted >= 0 && (size_t)converted <= len )
        fb_ConsolePrintBufferEx( temp, (size_t)converted, mask );

    if( temp != local_buffer )
        free( temp );

}

void fb_ConsolePrintBufferWstr( const FB_WCHAR *buffer, int mask )
{
	ssize_t length = fb_wstr_Len( buffer );
	if( length >= 0 )
		fb_ConsolePrintBufferWstrEx( buffer, (size_t)length, mask );
}

/* end of drivers/dos/io_printbuff_wstr.c */
