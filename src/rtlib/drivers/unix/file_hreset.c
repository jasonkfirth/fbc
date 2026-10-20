/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/file_hreset.c
 * Purpose: Provide file hreset runtime support.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_hFileResetEx( int streamno )
{
	FILE *f;

	if( streamno == 0 )
		f = freopen( "/dev/tty", "r", stdin );
	else 
		f = freopen( "/dev/tty", "w", stdout );

	// cppcheck-suppress resourceLeak
	return (f != NULL);
}

/* end of drivers/unix/file_hreset.c */
