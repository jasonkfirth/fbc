/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/file_hflush.c
 * Purpose: flush system buffers.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <unistd.h>

int fb_hFileFlushEx( FILE *f )
{
	if( fsync( fileno( f ) ) != 0 ) {
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of drivers/dos/file_hflush.c */
