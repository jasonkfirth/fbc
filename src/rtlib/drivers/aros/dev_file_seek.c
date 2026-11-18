/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/aros/dev_file_seek.c

    Purpose:

        Seek AROS file devices through dos.library on non-ARM targets.

    Responsibilities:

        - validate the runtime file device
        - serialize position changes with the runtime file lock
        - update logical file size after a successful seek

    This file intentionally does NOT contain:

        - ARM POSIXC stream handling, which is in arm/dev_file_seek.c
        - generic file-device dispatch
*/

#include "../../fb.h"

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

/* end of drivers/aros/dev_file_seek.c */
