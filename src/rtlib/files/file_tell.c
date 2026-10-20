/*
 * Project: FreeBASIC runtime library
 * File: files/file_tell.c
 * Purpose: TELL() and TELL.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
fb_off_t fb_FileTellEx( FB_FILE *handle )
{
	fb_off_t pos;

    if( !FB_HANDLE_USED(handle) )
		return 0;

	FB_LOCK();

    if (handle->hooks->pfnTell != NULL) {
        if (handle->hooks->pfnTell( handle, &pos )!=0) {
            pos = -1;
        }
    } else {
        pos = -1;
    }

    if (pos != -1) {
        /* Adjust real position by number of characters in put back buffer */
        pos -= handle->putback_size;

        /* if in random mode, divide by reclen */
        if( handle->mode == FB_FILE_MODE_RANDOM )
            pos /= handle->len;

    }

	FB_UNLOCK();

	return pos + 1;
}

/*:::::*/
FBCALL long long fb_FileTell( int fnum )
{
    return fb_FileTellEx( FB_FILE_TO_HANDLE(fnum) );
}

/* end of files/file_tell.c */
