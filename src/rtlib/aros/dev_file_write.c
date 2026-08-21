/*
    FreeBASIC runtime library
    -------------------------

    File: aros/dev_file_write.c

    Purpose:

        Write byte data and maintain the AROS file-device logical size.

    Responsibilities:

        - validate the file device
        - write the complete byte range
        - record growth for LOF and EOF queries

    This file intentionally does NOT contain:

        - text encoding
        - generic operating-system behavior
        - architecture policy
*/

#include "../fb.h"

int fb_DevFileWrite( FB_FILE *handle, const void *value, size_t value_length )
{
	FILE *fp;

	FB_LOCK();

	fp = (FILE *)handle->opaque;
	if( fp == NULL )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	if( FB_FWRITE_LARGE( value, value_length, fp ) != value_length )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	fb_hArosGrowFileSize( handle );

	FB_UNLOCK();
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of aros/dev_file_write.c */
