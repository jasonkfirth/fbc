/*
    FreeBASIC runtime JavaScript console output
    -------------------------------------------

    File: js/io_printbuff_wstr.c

    Purpose:

        Write a FreeBASIC wide-character buffer to the JavaScript target's
        standard console stream.

    Responsibilities:

        - preserve the shared console-print entry points
        - pass each character to the C wide-output interface using wint_t

    This file intentionally does NOT contain:

        - console buffering policy
        - terminal cursor or colour handling
        - string encoding conversion
*/

#include "../fb.h"

void fb_ConsolePrintBufferWstrEx
	(
		const FB_WCHAR *buffer,
		size_t chars,
		int mask
	)
{
	while( chars-- > 0 )
		wprintf( L"%lc", (wint_t)*buffer++ );
}

void fb_ConsolePrintBufferWstr
	(
		const FB_WCHAR *buffer,
		int mask
	)
{
	return fb_ConsolePrintBufferWstrEx( buffer, fb_wstr_Len( buffer ), mask );
}

/* end of js/io_printbuff_wstr.c */
