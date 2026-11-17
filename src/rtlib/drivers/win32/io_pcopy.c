/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_pcopy.c
 * Purpose: console pcopy function.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"
#include <stdint.h>

int fb_ConsolePageCopy( int src, int dst )
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD source_size;
	SMALL_RECT destination_window;
	size_t width, height, cell_count, buffer_size;
	PCHAR_INFO buff;
	BOOL result;
	COORD pos = { 0, 0 };

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

	if( !GetConsoleScreenBufferInfo( fb_console_context.pgHandleTb[src], &csbi ) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( (csbi.dwSize.X <= 0) || (csbi.dwSize.Y <= 0) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	source_size = csbi.dwSize;
	width = (size_t)source_size.X;
	height = (size_t)source_size.Y;
	if( (height > SIZE_MAX / width) ||
	    ((cell_count = width * height) > SIZE_MAX / sizeof( CHAR_INFO )) )
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );

	buffer_size = cell_count * sizeof( CHAR_INFO );
	buff = (PCHAR_INFO)malloc( buffer_size );
	if( buff == NULL )
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );

	result = ReadConsoleOutput( fb_console_context.pgHandleTb[src], buff,
	                            source_size, pos, &csbi.srWindow );
	if( result ) {
		result = GetConsoleScreenBufferInfo( fb_console_context.pgHandleTb[dst], &csbi );
		if( result ) {
			destination_window = csbi.srWindow;
			result = WriteConsoleOutput( fb_console_context.pgHandleTb[dst], buff,
			                            source_size, pos, &destination_window );
		}
	}

	free( buff );
	return fb_ErrorSetNum( result ? FB_RTERROR_OK : FB_RTERROR_FILEIO );
}

/* end of drivers/win32/io_pcopy.c */
