/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_file_encod_write_wstr.c
 * Purpose: UTF-encoded wstring file writing.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

static void hUTFToLE( unsigned char *buffer, ssize_t bytes, FB_FILE_ENCOD encod )
{
	UTF_16 unit16;
	UTF_32 unit32;
	unsigned char *unit_buffer;
	size_t i, units;

	if( bytes <= 0 )
		return;

	if( encod == FB_FILE_ENCOD_UTF16 ) {
		units = (size_t)bytes / sizeof( UTF_16 );
		for( i = 0; i < units; ++i ) {
			unit_buffer = buffer + (i * sizeof( unit16 ));
			memcpy( &unit16, unit_buffer, sizeof( unit16 ) );
			fb_UTF16ToLE( unit_buffer, unit16 );
		}
	} else if( encod == FB_FILE_ENCOD_UTF32 ) {
		units = (size_t)bytes / sizeof( UTF_32 );
		for( i = 0; i < units; ++i ) {
			unit_buffer = buffer + (i * sizeof( unit32 ));
			memcpy( &unit32, unit_buffer, sizeof( unit32 ) );
			fb_UTF32ToLE( unit_buffer, unit32 );
		}
	}
}

int fb_DevFileWriteEncodWstr( FB_FILE *handle, const FB_WCHAR* buffer, size_t chars )
{
	    FILE *fp;
    char *encod_buffer;
	ssize_t bytes;
	size_t write_bytes;

    FB_LOCK();

    fp = (FILE*) handle->opaque;
	if( fp == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}
	if( chars > (size_t)FB_STRSIZEMSK ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	/* convert (note: only wstrings will be written using this function,
				so there's no binary data to care) */
	encod_buffer = fb_WCharToUTF( handle->encod,
								  buffer,
								  (ssize_t)chars,
								  NULL,
								  &bytes );

	if( encod_buffer != NULL )
	{
		if( bytes < 0 ) {
			if( encod_buffer != (const char *)buffer )
				free( encod_buffer );
			FB_UNLOCK();
			return fb_ErrorSetNum( FB_RTERROR_FILEIO );
		}
		write_bytes = (size_t)bytes;
		hUTFToLE( (unsigned char *)encod_buffer, bytes, handle->encod );

		/* do write */
		if( fwrite( encod_buffer, 1, write_bytes, fp ) != write_bytes ) {
			if( encod_buffer != (const char *)buffer )
				free( encod_buffer );
			FB_UNLOCK();
			return fb_ErrorSetNum( FB_RTERROR_FILEIO );
		}

		if( encod_buffer != (const char *)buffer )
			free( encod_buffer );
	}

	FB_UNLOCK();

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of devices/dev_file_encod_write_wstr.c */
