/*
 * Project: FreeBASIC runtime library
 * File: console/io_write_wstr.c
 * Purpose: write [#] wstring functions.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL void fb_WriteWstr( int fnum, FB_WCHAR *s, int mask )
{
    const FB_WCHAR *buff;
	ssize_t len, bufflen;
    FB_FILE *handle = FB_FILE_TO_HANDLE( fnum );

	if( s == NULL )
	{
		if( mask & FB_PRINT_BIN_NEWLINE )
			fb_hFilePrintBufferWstrEx( handle, FB_WCHAR_LITERAL("\"\"" FB_BINARY_NEWLINE), 1+1+sizeof(FB_BINARY_NEWLINE)-1 );
		else if( mask & FB_PRINT_NEWLINE )
			fb_hFilePrintBufferWstrEx( handle, FB_WCHAR_LITERAL("\"\"" FB_NEWLINE), 1+1+sizeof(FB_NEWLINE)-1 );
		else
			fb_hFilePrintBufferWstrEx( handle, FB_WCHAR_LITERAL("\"\","), 1+1+1 );
		return;
	}

    /* close quote + new-line or comma */
    if( mask & FB_PRINT_BIN_NEWLINE )
    {
		buff = FB_WCHAR_LITERAL("\"" FB_BINARY_NEWLINE);
		bufflen = fb_wstr_Len( FB_WCHAR_LITERAL("\"" FB_BINARY_NEWLINE) );
	}
    else if( mask & FB_PRINT_NEWLINE )
    {
		buff = FB_WCHAR_LITERAL("\"" FB_NEWLINE);
		bufflen = fb_wstr_Len( FB_WCHAR_LITERAL("\"" FB_NEWLINE) );
	}
    else
    {
		buff = FB_WCHAR_LITERAL("\",");
		bufflen = 2;
	}

    FB_LOCK( );

    /* open quote */
    fb_hFilePrintBufferWstrEx( handle, FB_WCHAR_LITERAL("\""), 1 );

    len = fb_wstr_Len( s );
    if( len != 0 )
        FB_PRINTWSTR_EX( handle, s, len, 0 );

    fb_hFilePrintBufferWstrEx( handle, buff, bufflen );

    FB_UNLOCK( );
}

/* end of console/io_write_wstr.c */
