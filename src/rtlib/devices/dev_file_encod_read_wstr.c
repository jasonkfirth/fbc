/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_file_encod_read_wstr.c
 * Purpose: UTF-encoded input for file devices.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevFileReadEncodWstr( FB_FILE *handle, FB_WCHAR *dst, size_t *max_chars )
{
    FILE *fp;
    size_t requested;
    ssize_t chars;

    FB_LOCK();

    requested = *max_chars;

    fp = (FILE *)handle->opaque;
    if( fp == stdout || fp == stderr )
        fp = stdin;

    if( fp == NULL )
    {
        FB_UNLOCK();
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
    }

    if( requested > ((size_t)PTRDIFF_MAX / sizeof( FB_WCHAR )) )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}

	/* do read */
	chars = fb_hFileRead_UTFToWchar( fp, handle->encod, dst, (ssize_t)requested );
	if( (chars < 0) || ((size_t)chars > requested) )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	/* fill with nulls if at eof */
	if( (size_t)chars != requested )
        memset( (char *)&dst[(size_t)chars], 0,
		        (requested - (size_t)chars) * sizeof( FB_WCHAR ) );

	*max_chars = (size_t)chars;

	FB_UNLOCK();

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of devices/dev_file_encod_read_wstr.c */
