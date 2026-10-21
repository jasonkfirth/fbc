/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/file_hlock.c
 * Purpose: acquire and release native Windows byte-range locks.
 * Responsibilities: Translate runtime requests to LockFile and UnlockFile calls.
 * This file intentionally does NOT contain: BASIC range conversion or file-number validation.
 */
#include "../../fb.h"
#include <io.h>
#include <stdint.h>
#include <windows.h>

static int do_lock( FILE *f, int lock, fb_off_t inipos, fb_off_t size )
{
	uint64_t offset, length;
	intptr_t os_handle;
	int fd, result;

	if( f == NULL || inipos < 0 || size <= 0 )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	fd = fileno( f );
	if( fd < 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	os_handle = get_osfhandle( fd );
	if( os_handle == (intptr_t)INVALID_HANDLE_VALUE )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	/* LockFile takes separate low/high words for both 64-bit values. */
	offset = (uint64_t)inipos;
	length = (uint64_t)size;

	if( lock ) {
		result = LockFile( (HANDLE)os_handle,
		                   (DWORD)offset, (DWORD)(offset >> 32),
		                   (DWORD)length, (DWORD)(length >> 32) );
	} else {
		result = UnlockFile( (HANDLE)os_handle,
		                     (DWORD)offset, (DWORD)(offset >> 32),
		                     (DWORD)length, (DWORD)(length >> 32) );
	}

	return fb_ErrorSetNum( result ? FB_RTERROR_OK : FB_RTERROR_FILEIO );
}

int fb_hFileLock( FILE *f, fb_off_t inipos, fb_off_t size )
{
	return do_lock( f, TRUE, inipos, size );
}

int fb_hFileUnlock( FILE *f, fb_off_t inipos, fb_off_t size )
{
	return do_lock( f, FALSE, inipos, size );
}

/* end of drivers/win32/file_hlock.c */
