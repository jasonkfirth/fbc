/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/file_hseteof.c
 * Purpose: low-level truncate / set end of file.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <windows.h>
#include "pdclib/_PDCLIB_int.h"

int fb_hFileSetEofEx( FILE *f )
{
	if( f == NULL )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	if( fflush( f ) != 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	if( SetEndOfFile( (HANDLE)f->handle ) == 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of drivers/xbox/file_hseteof.c */
