/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/file_hseteof.c
 * Purpose: low-level truncate / set end of file.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <unistd.h>

int fb_hFileSetEofEx( FILE *f )
{
	fb_off_t pos;

	pos = ftello( f );
	if( ftruncate( fileno(f), pos ) != 0 ) {
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	return fb_ErrorSetNum( FB_RTERROR_OK );
	
}

/* end of drivers/unix/file_hseteof.c */
