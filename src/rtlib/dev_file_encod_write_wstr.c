/* UTF-encoded wstring file writing */

#include "fb.h"

static void hUTFToLE( char *buffer, ssize_t bytes, FB_FILE_ENCOD encod )
{
	unsigned char *dst;
	ssize_t i, units;

	switch( encod )
	{
	case FB_FILE_ENCOD_UTF16:
		dst = (unsigned char *)buffer;
		units = bytes / sizeof( UTF_16 );
		for( i = 0; i < units; ++i )
		{
			fb_UTF16ToLE( dst, ((UTF_16 *)buffer)[i] );
			dst += sizeof( UTF_16 );
		}
		break;

	case FB_FILE_ENCOD_UTF32:
		dst = (unsigned char *)buffer;
		units = bytes / sizeof( UTF_32 );
		for( i = 0; i < units; ++i )
		{
			fb_UTF32ToLE( dst, ((UTF_32 *)buffer)[i] );
			dst += sizeof( UTF_32 );
		}
		break;

	default:
		break;
	}
}

int fb_DevFileWriteEncodWstr( FB_FILE *handle, const FB_WCHAR* buffer, size_t chars )
{
	    FILE *fp;
    char *encod_buffer;
	ssize_t bytes;

    FB_LOCK();

    fp = (FILE*) handle->opaque;
	if( fp == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	/* convert (note: only wstrings will be written using this function,
				so there's no binary data to care) */
	encod_buffer = fb_WCharToUTF( handle->encod,
								  buffer,
								  chars,
								  NULL,
								  &bytes );

	if( encod_buffer != NULL )
	{
		hUTFToLE( encod_buffer, bytes, handle->encod );

		/* do write */
		if( fwrite( encod_buffer, 1, bytes, fp ) != (size_t)bytes )
		{
			FB_UNLOCK();
			return fb_ErrorSetNum( FB_RTERROR_FILEIO );
		}

		if( encod_buffer != (char *)buffer )
			free( encod_buffer );
	}

	FB_UNLOCK();

	return fb_ErrorSetNum( FB_RTERROR_OK );
}
