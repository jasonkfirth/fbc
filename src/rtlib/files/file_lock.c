/*
 * Project: FreeBASIC runtime library
 * File: files/file_lock.c
 * Purpose: lock and unlock byte ranges and random-file records.
 * Responsibilities: Validate file numbers and convert BASIC's one-based inclusive ranges.
 * This file intentionally does NOT contain: Platform lock system calls or compiler parsing.
 */
#include "fb.h"
#include <limits.h>
#include <stdint.h>

static fb_off_t fb_FileLockMaxOffset( void )
{
	/* Derive the signed maximum without relying on a nonstandard OFF_MAX macro. */
	unsigned int shift = (unsigned int)((sizeof(uintmax_t) - sizeof(fb_off_t)) * CHAR_BIT + 1);
	return (fb_off_t)(UINTMAX_MAX >> shift);
}

static int fb_FileLockArgumentsValid( long long inipos, long long endpos )
{
	fb_off_t max_offset = fb_FileLockMaxOffset();

	if( inipos < 1 || endpos < 0 || (endpos != 0 && endpos < inipos) )
		return FALSE;

	if( (uintmax_t)inipos > (uintmax_t)max_offset ||
	    (uintmax_t)endpos > (uintmax_t)max_offset )
		return FALSE;

	return TRUE;
}

/*
   LOCK/UNLOCK positions are one-based and inclusive. A missing TO clause is
   passed as endpos = 0 and means one byte or one random-file record.
*/
static int fb_FileLockRange( FB_FILE *handle, fb_off_t inipos, fb_off_t endpos,
                             fb_off_t *position, fb_off_t *size )
{
	fb_off_t first_unit, unit_count, unit_size, max_offset;

	if( inipos < 1 || endpos < 0 || (endpos != 0 && endpos < inipos) )
		return FALSE;

	unit_size = 1;
	if( handle->mode == FB_FILE_MODE_RANDOM ) {
		if( handle->len <= 0 )
			return FALSE;
		unit_size = handle->len;
	}

	first_unit = inipos - 1;
	unit_count = (endpos == 0) ? 1 : endpos - inipos + 1;
	max_offset = fb_FileLockMaxOffset();

	if( first_unit > max_offset / unit_size || unit_count > max_offset / unit_size )
		return FALSE;

	*position = first_unit * unit_size;
	*size = unit_count * unit_size;

	/* The native APIs need a representable exclusive end for the range. */
	if( *size > max_offset - *position )
		return FALSE;

	return TRUE;
}

static int fb_FileLockOperation( FB_FILE *handle, fb_off_t inipos, fb_off_t endpos,
                                int islock )
{
	int res;
	fb_off_t position, size;

	if( inipos < 1 || endpos < 0 || (endpos != 0 && endpos < inipos) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	FB_LOCK();

	if( !FB_HANDLE_USED(handle) ||
	    !fb_FileLockRange( handle, inipos, endpos, &position, &size ) ) {
		res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	} else if( islock ) {
		if( handle->hooks->pfnLock != NULL )
			res = handle->hooks->pfnLock( handle, position, size );
		else
			res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	} else {
		if( handle->hooks->pfnUnlock != NULL )
			res = handle->hooks->pfnUnlock( handle, position, size );
		else
			res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	FB_UNLOCK();

	return res;
}

int fb_FileLockEx( FB_FILE *handle, fb_off_t inipos, fb_off_t endpos )
{
	return fb_FileLockOperation( handle, inipos, endpos, TRUE );
}

int fb_FileUnlockEx( FB_FILE *handle, fb_off_t inipos, fb_off_t endpos )
{
	return fb_FileLockOperation( handle, inipos, endpos, FALSE );
}

FBCALL int fb_FileLock( int fnum, unsigned int inipos, unsigned int endpos )
{
	if( !FB_FILE_INDEX_VALID(fnum) ||
	    !fb_FileLockArgumentsValid( (long long)inipos, (long long)endpos ) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	return fb_FileLockEx( FB_FILE_TO_HANDLE(fnum), inipos, endpos );
}

FBCALL int fb_FileLockLarge( int fnum, long long inipos, long long endpos )
{
	if( !FB_FILE_INDEX_VALID(fnum) || !fb_FileLockArgumentsValid( inipos, endpos ) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	return fb_FileLockEx( FB_FILE_TO_HANDLE(fnum), (fb_off_t)inipos,
	                      (fb_off_t)endpos );
}

FBCALL int fb_FileUnlock( int fnum, unsigned int inipos, unsigned int endpos )
{
	if( !FB_FILE_INDEX_VALID(fnum) ||
	    !fb_FileLockArgumentsValid( (long long)inipos, (long long)endpos ) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	return fb_FileUnlockEx( FB_FILE_TO_HANDLE(fnum), inipos, endpos );
}

FBCALL int fb_FileUnlockLarge( int fnum, long long inipos, long long endpos )
{
	if( !FB_FILE_INDEX_VALID(fnum) || !fb_FileLockArgumentsValid( inipos, endpos ) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	return fb_FileUnlockEx( FB_FILE_TO_HANDLE(fnum), (fb_off_t)inipos,
	                      (fb_off_t)endpos );
}

/* end of files/file_lock.c */
