/*
 * Project: FreeBASIC runtime library
 * File: files/file_get.c
 * Purpose: get # function.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <limits.h>
#include <stdint.h>

#if defined(_MSC_VER)
#define FB_FILE_GET_ALIGNOF(type) __alignof(type)
#else
#define FB_FILE_GET_ALIGNOF(type) __alignof__(type)
#endif

int fb_FileGetDataEx
	(
		FB_FILE *handle,
		fb_off_t pos,
		void *dst,
		size_t length,
		size_t *bytesread,
		int adjust_rec_pos,
		int is_unicode
	)
{
    int res;
    size_t chars, read_chars;
    char *pachData = (char *)dst;

	if( bytesread )
		*bytesread = 0;

    if( !FB_HANDLE_USED(handle) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	if( pos < 0 )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

    if( (length != 0) && (dst == NULL) )
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

    if( is_unicode && (dst != NULL) &&
        ((uintptr_t)dst % FB_FILE_GET_ALIGNOF(FB_WCHAR) != 0) )
        return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

    FB_LOCK();

    res = fb_ErrorSetNum( FB_RTERROR_OK );

    chars = length;

    /* seek to newpos */
    if( pos > 0 )
        res = fb_FileSeekEx( handle, pos );

    /* any data in the put-back buffer? */
    if( handle->putback_size != 0 )
    {
        size_t buffer_chars, bytes;

        if( handle->putback_size > sizeof( handle->putback_buffer ) )
        {
            FB_UNLOCK();
            return fb_ErrorSetNum( FB_RTERROR_FILEIO );
        }

		buffer_chars = handle->putback_size;
		if( handle->encod != FB_FILE_ENCOD_ASCII )
			buffer_chars /= sizeof( FB_WCHAR );

        if( buffer_chars > chars )
            buffer_chars = chars;

		bytes = buffer_chars;
		if( handle->encod != FB_FILE_ENCOD_ASCII )
			bytes *= sizeof( FB_WCHAR );

        if( buffer_chars != 0 )
        {
            if( !is_unicode )
            {
                if( handle->encod == FB_FILE_ENCOD_ASCII )
                    memcpy( pachData, handle->putback_buffer, bytes );

                else
                {
                    size_t i;
                    char *cp = pachData;

                    for( i = 0; i < buffer_chars; ++i )
                    {
                        FB_WCHAR wc;
                        memcpy( &wc,
                                handle->putback_buffer + (i * sizeof( FB_WCHAR )),
                                sizeof( wc ) );
                        *cp++ = (char)wc;
                    }
                }
            }
            else
            {
                if( handle->encod != FB_FILE_ENCOD_ASCII )
                {
                    memcpy( pachData, handle->putback_buffer, bytes );
                }
                else
                {
                    size_t len = buffer_chars;
                    char *wcp = pachData;
                    unsigned char *cp = (unsigned char *)handle->putback_buffer;

                    while( len-- > 0 ) {
                        FB_WCHAR wc = (FB_WCHAR)*cp++;
                        memcpy(wcp, &wc, sizeof(wc));
                        wcp += sizeof(wc);
                    }
                }
            }

            handle->putback_size -= bytes;
            if( handle->putback_size != 0 )
            {
                memmove( handle->putback_buffer,
                         handle->putback_buffer + bytes,
                         handle->putback_size );
            }

			pachData += buffer_chars;
			if( is_unicode && sizeof( FB_WCHAR ) > 1 )
				pachData += buffer_chars * (sizeof( FB_WCHAR ) - 1);

            read_chars = buffer_chars;
            chars -= buffer_chars;
        }
        else
        {
            read_chars = 0;
        }
    }
    else
    	read_chars = 0;

    if ( (res == FB_RTERROR_OK) && (chars != 0) )
    {
        /* do read */
        if( !is_unicode )
        {
        	if( handle->hooks->pfnRead == NULL )
            	res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
        	else
        	{
        		res = handle->hooks->pfnRead( handle, pachData, &chars );
        		read_chars += chars;
        	}
        }
        else
        {
        	if( handle->hooks->pfnReadWstr == NULL )
            	res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		else if( pachData == NULL ||
		         ((uintptr_t)pachData % FB_FILE_GET_ALIGNOF(FB_WCHAR) != 0) )
			res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
        	else
        	{
                void *aligned_data = pachData;
                res = handle->hooks->pfnReadWstr( handle,
                    aligned_data, &chars );
        		read_chars += chars;
        	}
        }
    }

    if( handle->mode == FB_FILE_MODE_RANDOM &&
        res == FB_RTERROR_OK &&
        adjust_rec_pos &&
        handle->hooks->pfnSeek != NULL )
    {
        /* if in random mode, reads must be of reclen.
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
                /* Reduce before scaling so large reads cannot overflow. */
                uint64_t consumed_chars = (uint64_t)(read_chars % record_length);
                consumed_bytes = (size_t)((consumed_chars * sizeof(FB_WCHAR)) % record_length);
            } else {
                consumed_bytes = read_chars % record_length;
            }

            size_t skip_size = (record_length - consumed_bytes) % record_length;

            if( skip_size != 0 )
            {
                /* don't forget the put back buffer */
                if( skip_size > handle->putback_size )
                {
                    skip_size -= handle->putback_size;
                    handle->putback_size = 0;
                }
                else
                {
                    handle->putback_size -= skip_size;
                    skip_size = 0;
                }
            }

            if (skip_size!=0)
            {
                /* devices that don't support seek should simulate it
                 with read or never allow to be opened for random access */
                handle->hooks->pfnSeek( handle, (fb_off_t)skip_size, SEEK_CUR );
            }
        }
    }

	if( bytesread )
		*bytesread = read_chars;

	FB_UNLOCK();

	/* set the error code again - handle->hooks->pfnSeek() may have reset it */
	return fb_ErrorSetNum( res );
}

/*:::::*/
/* Can fb_FileGetData() be removed? it's not used by the rtlib
 * nor is it referenced by fbc?  Compatibility with old libs? [jeffm]
 */
int fb_FileGetData( int fnum, fb_off_t pos, void *dst, size_t chars, int adjust_rec_pos )
{
    return fb_FileGetDataEx( FB_FILE_TO_HANDLE(fnum),
    						 pos,
    						 dst,
    						 chars,
							 NULL,
    						 adjust_rec_pos,
    						 FALSE );
}

FBCALL int fb_FileGet( int fnum, int pos, void *dst, size_t chars )
{
	return fb_FileGetDataEx( FB_FILE_TO_HANDLE(fnum),
							 pos,
							 dst,
							 chars,
							 NULL,
							 TRUE,
							 FALSE );
}

FBCALL int fb_FileGetLarge( int fnum, long long pos, void *dst, size_t chars )
{
	fb_off_t offset;

	if( !FB_FILE_INDEX_VALID(fnum) || !fb_hFileOffsetFromLongLong(pos, &offset) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	return fb_FileGetDataEx( FB_FILE_TO_HANDLE(fnum),
							 offset,
							 dst,
							 chars,
							 NULL,
							 TRUE,
							 FALSE );
}

FBCALL int fb_FileGetIOB( int fnum, int pos, void *dst, size_t chars, size_t *bytesread )
{
	return fb_FileGetDataEx( FB_FILE_TO_HANDLE(fnum),
							 pos,
							 dst,
							 chars,
							 bytesread,
							 TRUE,
							 FALSE );
}

FBCALL int fb_FileGetLargeIOB( int fnum, long long pos, void *dst, size_t chars, size_t *bytesread )
{
	fb_off_t offset;

	if( !FB_FILE_INDEX_VALID(fnum) || !fb_hFileOffsetFromLongLong(pos, &offset) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	return fb_FileGetDataEx( FB_FILE_TO_HANDLE(fnum),
							 offset,
							 dst,
							 chars,
							 bytesread,
							 TRUE,
							 FALSE );
}

/* end of files/file_get.c */
