/*
    FreeBASIC runtime library
    -------------------------

    File: aros/dev_file_tell.c

    Purpose:

        Report an AROS file-device position through its buffered stream.

    Responsibilities:

        - validate the runtime file device and destination pointer
        - serialize position queries with the runtime file lock
        - return a FreeBASIC file error when AROS cannot report the position

    This file intentionally does NOT contain:

        - generic operating-system behavior
        - file-size discovery
        - AROS architecture policy
*/

#include "../fb.h"

int fb_DevFileTell( FB_FILE *handle, fb_off_t *position )
{
	FILE *fp;
	fb_off_t result;

	FB_LOCK();

	fp = (FILE *)handle->opaque;
	if( fp == NULL || position == NULL )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	result = fb_hArosGetFilePosition( fp );
	if( result < 0 )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	*position = result;
	FB_UNLOCK();
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of aros/dev_file_tell.c */
