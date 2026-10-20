/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/file_hlock.c
 * Purpose: low-level lock and unlock functions.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <io.h>
#include <windows.h>

int fb_hFileLock( FILE *f, fb_off_t inipos, fb_off_t size )
{
	return fb_ErrorSetNum( LockFile( (HANDLE)get_osfhandle( fileno( f ) ), inipos, 0, size, 0 ) == TRUE ?
	                       FB_RTERROR_OK : FB_RTERROR_FILEIO );
}

int fb_hFileUnlock( FILE *f, fb_off_t inipos, fb_off_t size )
{
	return fb_ErrorSetNum( UnlockFile( (HANDLE)get_osfhandle( fileno( f ) ), inipos, 0, size, 0 ) == TRUE ?
	                       FB_RTERROR_OK : FB_RTERROR_FILEIO );
}

/* end of drivers/win32/file_hlock.c */
