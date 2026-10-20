/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/file_hlock.c
 * Purpose: low-level lock and unlock functions.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <io.h>

int fb_hFileLock( FILE *f, fb_off_t inipos, fb_off_t size )
{
	return fb_ErrorSetNum( _dos_lock( fileno(f), inipos, size ) == 0 ?
	                       FB_RTERROR_OK : FB_RTERROR_FILEIO );
}

int fb_hFileUnlock( FILE *f, fb_off_t inipos, fb_off_t size )
{
	return fb_ErrorSetNum( _dos_unlock( fileno(f), inipos, size) == 0 ?
	                       FB_RTERROR_OK : FB_RTERROR_FILEIO );
}

/* end of drivers/dos/file_hlock.c */
