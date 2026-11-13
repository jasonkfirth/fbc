/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_pcopy.c
 * Purpose: console pcopy function.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsolePageCopy( int src, int dst )
{
	fb_hConsoleGetHandle( FALSE );

	/* use current? */
	if( src < 0 )
		src = fb_console_context.active;

	/* not allocated yet? */
	if( fb_console_context.pgHandleTb[src] == NULL )
	{
    	HANDLE hnd = fb_hConsoleCreateBuffer( );
        if( hnd == NULL )
           	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		else
			fb_console_context.pgHandleTb[src] = hnd;
	}

	/* use current? */
	if( dst < 0 )
		dst = fb_console_context.visible;

	if( src == dst )
		return fb_ErrorSetNum( FB_RTERROR_OK );

	/* not allocated yet? */
	if( fb_console_context.pgHandleTb[dst] == NULL )
	{
    	HANDLE hnd = fb_hConsoleCreateBuffer( );
        if( hnd == NULL )
           	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		else
			fb_console_context.pgHandleTb[dst] = hnd;
	}

	/* do the copy */

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo( fb_console_context.pgHandleTb[src], &csbi );
	PCHAR_INFO buff = malloc( csbi.dwSize.X * csbi.dwSize.Y * sizeof( CHAR_INFO ) );
	if(buff)
	{
		COORD pos = { 0, 0 };
		ReadConsoleOutput( fb_console_context.pgHandleTb[src], buff, csbi.dwSize, pos, &csbi.srWindow );

		GetConsoleScreenBufferInfo( fb_console_context.pgHandleTb[dst], &csbi );
		WriteConsoleOutput( fb_console_context.pgHandleTb[dst], buff, csbi.dwSize, pos, &csbi.srWindow );
		free( buff );
	}
	return fb_ErrorSetNum( buff ? FB_RTERROR_OK : FB_RTERROR_OUTOFMEM );
}

/* end of drivers/win32/io_pcopy.c */
