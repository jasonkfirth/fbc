/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/wince/io_pcopy.c

    Purpose:

        Copy one Windows CE logical console page to another.

    Responsibilities:

        - resolve omitted source and destination page numbers
        - allocate pages on first use
        - copy characters and color attributes together

    This file intentionally does NOT contain: - desktop ReadConsoleOutput or WriteConsoleOutput calls
        - graphics framebuffer copies
        - page presentation
*/

#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsolePageCopy( int source, int destination )
{
	size_t bytes;

	if( source < 0 )
		source = fb_console_context.active;
	if( destination < 0 )
		destination = fb_console_context.visible;
	if( source == destination )
		return fb_ErrorSetNum( FB_RTERROR_OK );

	if( !fb_hWinCEConsoleEnsurePage( source ) ||
	    !fb_hWinCEConsoleEnsurePage( destination ) )
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );

	bytes = (size_t)fb_console_context.columns * fb_console_context.rows *
	        sizeof(FB_WINCE_CONSOLE_CELL);
	memcpy( fb_console_context.pages[destination].cells,
	        fb_console_context.pages[source].cells, bytes );

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of drivers/wince/io_pcopy.c */
