/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/file_hflush.c
 * Purpose: flush system buffers.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_hFileFlushEx( FILE *f )
{
	if( fsync( fileno( f ) ) != 0 ) {
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of drivers/js/file_hflush.c */
