/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/file_hreset.c
 * Purpose: Provide file hreset runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_hFileResetEx( int streamno )
{
	FILE *f;

	if( streamno == 0 )
		f = freopen( "CON", "r", stdin );
	else 
		f = freopen( "CON", "w", stdout );

	return (f != NULL);
}

/* end of drivers/dos/file_hreset.c */
