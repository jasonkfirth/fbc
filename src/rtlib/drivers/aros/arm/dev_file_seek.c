/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/aros/arm/dev_file_seek.c

    Purpose:

        Seek ARM AROS file devices through their POSIXC streams.

    Responsibilities:

        - validate the runtime file device
        - serialize position changes with the runtime file lock
        - keep buffered stream position and logical file size synchronized

    This file intentionally does NOT contain:

        - non-ARM dos.library positioning
        - generic file-device dispatch
*/

#include "../../../fb.h"

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

	/* Keep ARM's POSIXC stream and its native handle paired internally. */
	result = fseeko( fp, offset, whence );
	if( result == 0 )
	{
		fb_off_t position = ftello( fp );

		if( position > handle->size )
			handle->size = position;
	}
	result = fb_ErrorSetNum( result == 0
				 ? FB_RTERROR_OK
				 : FB_RTERROR_FILEIO );

	FB_UNLOCK();
	return result;
}

/* end of drivers/aros/arm/dev_file_seek.c */
