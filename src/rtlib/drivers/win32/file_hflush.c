/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/file_hflush.c
 * Purpose: low-level flush system buffers.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <io.h>
#include <windows.h>

int fb_hFileFlushEx( FILE *f )
{
	if( FlushFileBuffers( (HANDLE)get_osfhandle( fileno( f ) ) ) == 0 ) {
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of drivers/win32/file_hflush.c */
