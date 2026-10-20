/*
 * Project: FreeBASIC runtime library
 * File: files/file_loc.c
 * Purpose: seek function and stmt.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
fb_off_t fb_FileLocationEx( FB_FILE *handle )
{
    fb_off_t pos;

    if( !FB_HANDLE_USED(handle) )
		return 0;

    FB_LOCK();

    pos = fb_FileTellEx( handle );

    if (pos != 0) {
        --pos;
        switch( handle->mode )
        {
        case FB_FILE_MODE_INPUT:
        case FB_FILE_MODE_OUTPUT:
            /* if in seq mode, divide by 128 (QB quirk) */
            pos /= 128;
            break;
        }
    }

	FB_UNLOCK();

	return pos;
}

/*:::::*/
FBCALL long long fb_FileLocation( int fnum )
{
    return fb_FileLocationEx( FB_FILE_TO_HANDLE(fnum) );
}

/* end of files/file_loc.c */
