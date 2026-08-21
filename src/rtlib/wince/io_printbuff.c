/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_printbuff.c

    Purpose:

        Write narrow text through the Windows CE logical console.

    Responsibilities:

        - preserve output through the CeGCC standard stream
        - track cursor movement in the process-local text grid
        - apply wrapping and bottom-row scrolling semantics

    This file intentionally does NOT contain:

        - desktop console screen-buffer calls
        - graphics text rendering
        - input event processing
*/

#include "../fb.h"
#include "fb_private_console.h"

void fb_ConsolePrintBufferEx( const void *buffer, size_t length, int mask )
{
	const char *text = (const char *)buffer;

	(void)mask;
	if( (text == NULL) || (length == 0) )
		return;

	FB_LOCK();
	(void)fwrite( text, 1, length, stdout );
	(void)fflush( stdout );
	fb_hWinCEConsoleWrite( text, length );
	FB_UNLOCK();
}

void fb_ConsolePrintBuffer( const char *buffer, int mask )
{
	if( buffer != NULL )
		fb_ConsolePrintBufferEx( buffer, strlen( buffer ), mask );
}

/* end of wince/io_printbuff.c */
