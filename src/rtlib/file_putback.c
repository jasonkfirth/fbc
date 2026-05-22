/* some kind of ungetc function */

#include "fb.h"

int fb_FilePutBackEx( FB_FILE *handle, const void *src, size_t chars )
{
	int res;
	size_t bytes = 0;
	size_t i;

	if( !FB_HANDLE_USED(handle) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	FB_LOCK();

	res = fb_ErrorSetNum( FB_RTERROR_OK );

	if( handle->putback_size > sizeof( handle->putback_buffer ) )
	{
		res = fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}
	else
	{
		bytes = sizeof( handle->putback_buffer ) - handle->putback_size;

		/* UTF? */
		if( handle->encod != FB_FILE_ENCOD_ASCII )
		{
			if( chars > bytes / sizeof( FB_WCHAR ) )
				res = fb_ErrorSetNum( FB_RTERROR_FILEIO );
			else
				bytes = chars * sizeof( FB_WCHAR );
		}
		else
		{
			if( chars > bytes )
				res = fb_ErrorSetNum( FB_RTERROR_FILEIO );
			else
				bytes = chars;
		}
	}

	if( res == FB_RTERROR_OK )
	{
		/* note: if encoding != ASCII, putback buffer will be in
		   wchar format, not in UTF */
		if( handle->putback_size )
		{
			memmove( handle->putback_buffer + bytes,
			         handle->putback_buffer,
			         handle->putback_size );
		}

		if( handle->encod == FB_FILE_ENCOD_ASCII )
		{
			memcpy( handle->putback_buffer, src, bytes );
		}
		else
		{
			/* char to wchar */
			const unsigned char *patch = (const unsigned char *)src;
			for( i = 0; i < chars; ++i )
			{
				FB_WCHAR wc = patch[i];
				memcpy( handle->putback_buffer + (i * sizeof( FB_WCHAR )),
				        &wc,
				        sizeof( wc ) );
			}
		}

		handle->putback_size += bytes;
	}

	FB_UNLOCK();

	return res;
}

FBCALL int fb_FilePutBack( int fnum, const void *data, size_t length )
{
    return fb_FilePutBackEx( FB_FILE_TO_HANDLE(fnum), data, length );
}
