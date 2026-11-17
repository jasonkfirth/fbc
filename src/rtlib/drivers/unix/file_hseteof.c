/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/file_hseteof.c
 * Purpose: low-level truncate / set end of file.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <limits.h>
#include <unistd.h>

int fb_hFileSetEofEx( FILE *f )
{
	fb_off_t pos;

	pos = ftello( f );
	if( pos < 0 ) {
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}
#if defined(HOST_RISCOS)
	if( pos > LONG_MAX )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
#endif
#if defined(HOST_RISCOS) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
	if( ftruncate( fileno(f), (off_t)pos ) != 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
#if defined(HOST_RISCOS) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

	return fb_ErrorSetNum( FB_RTERROR_OK );
	
}

/* end of drivers/unix/file_hseteof.c */
