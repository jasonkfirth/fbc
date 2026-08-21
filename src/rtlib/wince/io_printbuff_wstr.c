/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_printbuff_wstr.c

    Purpose:

        Write wide text through the Windows CE logical console.

    Responsibilities:

        - convert UTF-16 runtime text to UTF-8
        - preserve explicit-length strings without requiring a terminator
        - share cursor and output handling with the narrow console path

    This file intentionally does NOT contain:

        - desktop console screen-buffer calls
        - locale-dependent lossy conversion
        - input event processing
*/

#include "../fb.h"
#include "fb_private_console.h"

#include <limits.h>
#include <windows.h>

void fb_ConsolePrintBufferWstrEx( const FB_WCHAR *buffer, size_t characters,
	                              int mask )
{
	char *text;
	int byte_count;

	if( (buffer == NULL) || (characters == 0) )
		return;

	if( characters > INT_MAX )
		return;

	byte_count = WideCharToMultiByte( CP_UTF8, 0, buffer, (int)characters,
	                                 NULL, 0, NULL, NULL );
	if( byte_count <= 0 )
		return;

	text = malloc( (size_t)byte_count );
	if( text == NULL )
		return;

	if( WideCharToMultiByte( CP_UTF8, 0, buffer, (int)characters, text,
	                        byte_count, NULL, NULL ) == byte_count ) {
		fb_ConsolePrintBufferEx( text, (size_t)byte_count, mask );
	}

	free( text );
}

void fb_ConsolePrintBufferWstr( const FB_WCHAR *buffer, int mask )
{
	if( buffer != NULL )
		fb_ConsolePrintBufferWstrEx( buffer, fb_wstr_Len( buffer ), mask );
}

/* end of wince/io_printbuff_wstr.c */
