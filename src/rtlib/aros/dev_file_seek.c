/*
    FreeBASIC runtime library
    -------------------------

    File: aros/dev_file_seek.c

    Purpose:

        Seek an AROS file device through its buffered POSIXC stream.

    Responsibilities:

        - validate the runtime file device
        - serialize position changes with the runtime file lock
        - translate stream-positioning failures to FreeBASIC file errors
        - track file growth caused by a seek beyond end-of-file

    This file intentionally does NOT contain:

        - generic operating-system behavior
        - file-size discovery
        - AROS architecture policy
*/

#include "../fb.h"

int fb_DevFileSeek( FB_FILE *handle, fb_off_t offset, int whence )
{
	FILE *fp;
	int result;

	FB_LOCK();

	fp = (FILE *)handle->opaque;
	if( fp == NULL )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	result = fb_hArosSetFilePosition( fp, offset, whence );
	if( result == 0 )
		fb_hArosGrowFileSize( handle );
	result = fb_ErrorSetNum( result == 0
				 ? FB_RTERROR_OK
				 : FB_RTERROR_FILEIO );

	FB_UNLOCK();
	return result;
}

/* end of aros/dev_file_seek.c */
