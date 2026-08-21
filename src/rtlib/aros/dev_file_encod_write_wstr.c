/*
    FreeBASIC runtime library
    -------------------------

    File: aros/dev_file_encod_write_wstr.c

    Purpose:

        Write encoded wide-character data and maintain logical file size.

    Responsibilities:

        - convert FreeBASIC wide characters to the requested UTF encoding
        - normalize multi-byte values to little-endian file order
        - write the complete converted value
        - record growth for LOF and EOF queries

    This file intentionally does NOT contain:

        - encoded input
        - generic operating-system behavior
        - architecture policy
*/

#include "../fb.h"

static void fb_hArosUTFToLEWstr( char *buffer, ssize_t bytes,
				 FB_FILE_ENCOD encoding )
{
	unsigned char *destination;
	ssize_t index;
	ssize_t units;

	switch( encoding )
	{
	case FB_FILE_ENCOD_UTF16:
		destination = (unsigned char *)buffer;
		units = bytes / sizeof( UTF_16 );
		for( index = 0; index < units; ++index )
		{
			fb_UTF16ToLE( destination, ((UTF_16 *)buffer)[index] );
			destination += sizeof( UTF_16 );
		}
		break;

	case FB_FILE_ENCOD_UTF32:
		destination = (unsigned char *)buffer;
		units = bytes / sizeof( UTF_32 );
		for( index = 0; index < units; ++index )
		{
			fb_UTF32ToLE( destination, ((UTF_32 *)buffer)[index] );
			destination += sizeof( UTF_32 );
		}
		break;

	default:
		break;
	}
}

int fb_DevFileWriteEncodWstr( FB_FILE *handle, const FB_WCHAR *buffer,
			       size_t characters )
{
	char *encoded_buffer;
	ssize_t bytes;
	FILE *fp;
	int result = FB_RTERROR_OK;

	FB_LOCK();

	fp = (FILE *)handle->opaque;
	if( fp == NULL )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	encoded_buffer = fb_WCharToUTF( handle->encod, buffer,
					characters, NULL, &bytes );
	if( encoded_buffer != NULL )
	{
		fb_hArosUTFToLEWstr( encoded_buffer, bytes, handle->encod );
		if( fwrite( encoded_buffer, 1, bytes, fp ) != (size_t)bytes )
			result = FB_RTERROR_FILEIO;

		if( encoded_buffer != (char *)buffer )
			free( encoded_buffer );

		if( result == FB_RTERROR_OK )
			fb_hArosGrowFileSize( handle );
	}

	FB_UNLOCK();
	return fb_ErrorSetNum( result );
}

/* end of aros/dev_file_encod_write_wstr.c */
