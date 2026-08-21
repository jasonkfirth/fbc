/*
    FreeBASIC runtime library
    -------------------------

    File: aros/dev_file_write_wstr.c

    Purpose:

        Write a wide string as ASCII and maintain logical file size.

    Responsibilities:

        - validate the file device
        - convert the requested wide characters to bytes
        - write the complete converted value
        - record growth for LOF and EOF queries

    This file intentionally does NOT contain:

        - UTF file encoding
        - generic operating-system behavior
        - architecture policy
*/

#include "../fb.h"

int fb_DevFileWriteWstr( FB_FILE *handle, const FB_WCHAR *source,
			 size_t characters )
{
	char *buffer;
	ssize_t bytes;
	FILE *fp;
	int result;

	FB_LOCK();

	fp = (FILE *)handle->opaque;
	if( fp == NULL )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	if( characters < FB_LOCALBUFF_MAXLEN )
	{
		buffer = alloca( characters + 1 );
	}
	else
	{
		buffer = malloc( characters + 1 );
		if( buffer == NULL )
		{
			FB_UNLOCK();
			return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
		}
	}

	bytes = fb_wstr_ConvToA( buffer, characters, source );
	result = fwrite( buffer, 1, bytes, fp ) == (size_t)bytes;

	if( characters >= FB_LOCALBUFF_MAXLEN )
		free( buffer );

	if( result )
		fb_hArosGrowFileSize( handle );

	FB_UNLOCK();
	return fb_ErrorSetNum( result ? FB_RTERROR_OK : FB_RTERROR_FILEIO );
}

/* end of aros/dev_file_write_wstr.c */
