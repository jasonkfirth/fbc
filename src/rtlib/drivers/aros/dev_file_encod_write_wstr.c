/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/aros/dev_file_encod_write_wstr.c

    Purpose:

        Write encoded wide-character data and maintain logical file size.

    Responsibilities:

        - convert FreeBASIC wide characters to the requested UTF encoding
        - normalize multi-byte values to little-endian file order
        - write the complete converted value
        - record growth for LOF and EOF queries

    This file intentionally does NOT contain: - encoded input
        - generic operating-system behavior
        - architecture policy
*/

#include "../../fb.h"

static int fb_hArosUTFToLEWstr( char *buffer, ssize_t bytes,
				 FB_FILE_ENCOD encoding )
{
	unsigned char *destination;
	ssize_t index;
	ssize_t units;

	if( (bytes < 0) || ((buffer == NULL) && (bytes != 0)) )
		return FALSE;

	switch( encoding )
	{
	case FB_FILE_ENCOD_UTF16:
		destination = (unsigned char *)buffer;
		if( (bytes % (ssize_t)sizeof( UTF_16 )) != 0 )
			return FALSE;
		units = bytes / (ssize_t)sizeof( UTF_16 );
		for( index = 0; index < units; ++index )
		{
			UTF_16 unit;
			memcpy( &unit, destination, sizeof( unit ) );
			fb_UTF16ToLE( destination, unit );
			destination += sizeof( UTF_16 );
		}
		break;

	case FB_FILE_ENCOD_UTF32:
		destination = (unsigned char *)buffer;
		if( (bytes % (ssize_t)sizeof( UTF_32 )) != 0 )
			return FALSE;
		units = bytes / (ssize_t)sizeof( UTF_32 );
		for( index = 0; index < units; ++index )
		{
			UTF_32 unit;
			memcpy( &unit, destination, sizeof( unit ) );
			fb_UTF32ToLE( destination, unit );
			destination += sizeof( UTF_32 );
		}
		break;

	default:
		break;
	}
	return TRUE;
}

int fb_DevFileWriteEncodWstr( FB_FILE *handle, const FB_WCHAR *buffer,
			       size_t characters )
{
	char *encoded_buffer;
	ssize_t bytes;
	FILE *fp;
	int result = FB_RTERROR_OK;

	if( (handle == NULL) || ((buffer == NULL) && (characters != 0)) ||
	    (characters > (size_t)SSIZE_MAX) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	FB_LOCK();

	fp = (FILE *)handle->opaque;
	if( fp == NULL )
	{
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	encoded_buffer = fb_WCharToUTF( handle->encod, buffer,
					(ssize_t)characters, NULL, &bytes );
	if( encoded_buffer != NULL )
	{
		if( !fb_hArosUTFToLEWstr( encoded_buffer, bytes, handle->encod ) ||
		    (fwrite( encoded_buffer, 1, (size_t)bytes, fp ) !=
		     (size_t)bytes) )
			result = FB_RTERROR_FILEIO;

		if( (const void *)encoded_buffer != (const void *)buffer )
			free( encoded_buffer );

		if( result == FB_RTERROR_OK )
			fb_hArosGrowFileSize( handle );
	}

	FB_UNLOCK();
	return fb_ErrorSetNum( result );
}

/* end of drivers/aros/dev_file_encod_write_wstr.c */
