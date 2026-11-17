/*
 * Project: FreeBASIC runtime library
 * File: files/file_put.c
 * Purpose: put # function.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <stdint.h>

int fb_FilePutDataEx
	(
		FB_FILE *handle,
		fb_off_t pos,
		const void *data,
		size_t length,
		int adjust_rec_pos,
		int checknewline,
		int is_unicode
	)
{
	int res;

    if( !FB_HANDLE_USED(handle) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( pos < 0 )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

    FB_LOCK();

    res = fb_ErrorSetNum( FB_RTERROR_OK );

    /* clear put back buffer for every modifying non-read operation */
    handle->putback_size = 0;

    /* seek to newpos */
    if( pos > 0 )
        res = fb_FileSeekEx( handle, pos );

    if (res==FB_RTERROR_OK)
    {
        /* do write */
        if( !is_unicode )
        {
        	if( handle->hooks->pfnWrite != NULL )
            	res = handle->hooks->pfnWrite( handle, data, length );
        	else
            	res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
        }
        else
        {
        	if( handle->hooks->pfnWriteWstr != NULL )
                res = handle->hooks->pfnWriteWstr( handle, (const FB_WCHAR *)data, length );
        	else
            	res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
        }

    }

    if( handle->mode == FB_FILE_MODE_RANDOM &&
        res == FB_RTERROR_OK &&
        adjust_rec_pos &&
        handle->hooks->pfnSeek!=NULL )
    {
        /* if in random mode, writes must be of reclen.
         * The device must also support the SEEK method and the length
         * must be non-null */

        if( handle->len < 0 ) {
            res = fb_ErrorSetNum( FB_RTERROR_FILEIO );
        } else if( handle->len > 0 ) {
            size_t record_length = (size_t)handle->len;
            size_t consumed_bytes;

			if( length != record_length )
				res = fb_ErrorSetNum( FB_RTERROR_FILEIO );

            if( is_unicode ) {
                /* Reduce before scaling so large writes cannot overflow. */
                uint64_t consumed_chars = (uint64_t)(length % record_length);
                consumed_bytes = (size_t)((consumed_chars * sizeof(FB_WCHAR)) % record_length);
            } else {
                consumed_bytes = length % record_length;
            }

            size_t skip_size = (record_length - consumed_bytes) % record_length;
            if (skip_size != 0)
            {
                /* devices that don't support seek should simulate it
                 with write or never allow to be opened for random access */
                handle->hooks->pfnSeek( handle, (fb_off_t)skip_size, SEEK_CUR );
            }
        }
    }

#ifndef FB_NATIVE_TAB
    if( checknewline )
    	if ( res == FB_RTERROR_OK )
    	{
    		size_t i = length;
    		if( !is_unicode )
    		{
    			const char *pachText = (const char *) data;

        		/* search for last printed CR or LF */
        		while (i--)
        		{
            		char ch = pachText[i];
            		if (ch=='\n' || ch=='\r')
	                	break;
        		}
        	}
        	else
        	{
    			const FB_WCHAR *pachText = (const FB_WCHAR *) data;

        		/* search for last printed CR or LF */
        		while (i--)
        		{
            		FB_WCHAR ch = pachText[i];
            		if (ch == FB_WCHAR_LITERAL('\n') || ch== FB_WCHAR_LITERAL('\r') )
	                	break;
        		}

        	}

	       	handle = FB_HANDLE_DEREF(handle);
			if( handle != NULL )
			{
				++i;
				if (i==0)
					handle->line_length += (unsigned int)length;
				else
					handle->line_length = (unsigned int)(length - i);

				if( handle->width!=0 ) {
					handle->line_length %= handle->width;
				}
			}
    	}
#endif

	FB_UNLOCK();

	/* set the error code again - handle->hooks->pfnSeek() may have reset it */
	return fb_ErrorSetNum( res );
}

int fb_FilePutData
	(
		int fnum,
		fb_off_t pos,
		const void *data,
		size_t length,
		int adjust_rec_pos,
		int checknewline
	)
{
    return fb_FilePutDataEx( FB_FILE_TO_HANDLE(fnum),
    						 pos, data, length, adjust_rec_pos, checknewline, FALSE );
}

FBCALL int fb_FilePut
	(
		int fnum,
		int pos,
		void* value,
		size_t valuelen
	)
{
	return fb_FilePutData( fnum, pos, value, valuelen, TRUE, FALSE );
}

FBCALL int fb_FilePutLarge
	(
		int fnum,
		long long pos,
		void *value,
		size_t valuelen
	)
{
	fb_off_t offset;

	if( !FB_FILE_INDEX_VALID(fnum) || !fb_hFileOffsetFromLongLong(pos, &offset) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	return fb_FilePutData( fnum, offset, value, valuelen, TRUE, FALSE );
}

/* end of files/file_put.c */
