/* ungetwc-like function */

#include "fb.h"

int fb_FilePutBackWstrEx( FB_FILE *handle, const FB_WCHAR *src, size_t chars )
{
	int res;
	size_t bytes = 0;
	size_t i;
	char *dst;

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
		if( handle->putback_size )
			memmove( handle->putback_buffer + bytes,
			         handle->putback_buffer,
			         handle->putback_size );

		handle->putback_size += bytes;

		/* note: if encoding != ASCII, putback buffer will be in
		   wchar format, not in UTF */
		if( handle->encod != FB_FILE_ENCOD_ASCII )
			memcpy( handle->putback_buffer, src, bytes );
		else
		{
			/* wchar to char */
			dst = handle->putback_buffer;
			for( i = 0; i < bytes; ++i )
				dst[i] = (char)src[i];
		}
	}

	FB_UNLOCK();

	return res;
}

FBCALL int fb_FilePutBackWstr( int fnum, const FB_WCHAR *src, size_t chars )
{
    return fb_FilePutBackWstrEx( FB_FILE_TO_HANDLE(fnum), src, chars );
}
