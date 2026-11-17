/*
 * Project: FreeBASIC runtime library
 * File: files/file_putarray.c
 * Purpose: put # function for arrays.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_FilePutArray( int fnum, int pos, FBARRAY_ACCESS *src )
{
	return fb_FilePutData( fnum, pos, src->ptr, src->size, TRUE, FALSE );
}

FBCALL int fb_FilePutArrayLarge( int fnum, long long pos, FBARRAY_ACCESS *src )
{
	fb_off_t offset;

	if( !FB_FILE_INDEX_VALID(fnum) || !fb_hFileOffsetFromLongLong(pos, &offset) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	return fb_FilePutData( fnum, offset, src->ptr, src->size, TRUE, FALSE );
}

/* end of files/file_putarray.c */
