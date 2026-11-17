/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/aros/dev_file_write_wstr.c

    Purpose:

        Write a wide string as ASCII and maintain logical file size.

    Responsibilities:

        - validate the file device
        - convert the requested wide characters to bytes
        - write the complete converted value
        - record growth for LOF and EOF queries

    This file intentionally does NOT contain: - UTF file encoding
        - generic operating-system behavior
        - architecture policy
*/

#include "../../fb.h"
#include <stdint.h>
#include <limits.h>

int fb_DevFileWriteWstr( FB_FILE *handle, const FB_WCHAR *source,
			 size_t characters )
{
	char *buffer;
	char local_buffer[FB_LOCALBUFF_MAXLEN + 1];
	ssize_t bytes;
	FILE *fp;
	int result;
	int allocated = FALSE;

	FB_LOCK();

	fp = (FILE *)handle->opaque;
	if( (fp == NULL) || ((source == NULL) && (characters != 0)) )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}
	if( (characters > (size_t)SSIZE_MAX) || (characters == SIZE_MAX) )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	if( characters < FB_LOCALBUFF_MAXLEN )
	{
		buffer = local_buffer;
	}
	else
	{
		buffer = malloc( characters + 1 );
		if( buffer == NULL )
		{
			FB_UNLOCK();
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		}
		allocated = TRUE;
	}

	bytes = fb_wstr_ConvToA( buffer, (ssize_t)characters, source );
	result = (bytes >= 0) &&
		(fwrite( buffer, 1, (size_t)bytes, fp ) == (size_t)bytes);

	if( allocated )
		free( buffer );

	if( result )
		fb_hArosGrowFileSize( handle );

	FB_UNLOCK();
	return fb_ErrorSetNum( result ? FB_RTERROR_OK : FB_RTERROR_FILEIO );
}

/* end of drivers/aros/dev_file_write_wstr.c */
