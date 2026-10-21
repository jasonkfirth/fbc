/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/file_hlock.c
 * Purpose: acquire and release native Unix byte-range locks.
 * Responsibilities: Translate runtime requests to POSIX fcntl record locks.
 * This file intentionally does NOT contain: BASIC range conversion or file-number validation.
 */
#include "../../fb.h"
#include <fcntl.h>
#include <string.h>

static int do_lock( FILE *f, int lock, fb_off_t inipos, fb_off_t size )
{
	struct flock lck;
	int fd, flags, result;

	if( f == NULL || inipos < 0 || size <= 0 )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	memset( &lck, 0, sizeof(lck) );

	fd = fileno( f );
	if( fd < 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	flags = fcntl( fd, F_GETFL );
	if( flags < 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	if( lock ) {
		/* O_RDONLY is zero, so inspect the O_ACCMODE field explicitly. */
		lck.l_type = ((flags & O_ACCMODE) == O_RDONLY) ? F_RDLCK : F_WRLCK;
	} else {
		lck.l_type = F_UNLCK;
	}

	lck.l_whence = SEEK_SET;
	lck.l_start = inipos;
	lck.l_len = size;

	/* Runtime file operations hold FB_LOCK, so contention must not block here. */
	result = fcntl( fd, F_SETLK, &lck );

	return fb_ErrorSetNum( result < 0 ? FB_RTERROR_FILEIO : FB_RTERROR_OK );
}

int fb_hFileLock( FILE *f, fb_off_t inipos, fb_off_t size )
{
	return do_lock( f, TRUE, inipos, size );
}

int fb_hFileUnlock( FILE *f, fb_off_t inipos, fb_off_t size )
{
	return do_lock( f, FALSE, inipos, size );
}

/* end of drivers/unix/file_hlock.c */
