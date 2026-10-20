/*
 * Project: FreeBASIC runtime library
 * File: drivers/wince/file_hreset.c
 * Purpose: Provide file hreset runtime support.
 * Responsibilities: Target-specific runtime support for wince.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_hFileResetEx( int streamno )
{
	FILE *f;

	fb_hConsoleResetHandles();

	if( streamno == 0 )
		f = freopen( "CONIN$", "r", stdin );
	else 
		f = freopen( "CONOUT$", "w", stdout );

	/* force handles to be reinitialized now */
	fb_hConsoleGetHandle( TRUE );

	// cppcheck-suppress resourceLeak
	return (f != NULL);
}

/* end of drivers/wince/file_hreset.c */
