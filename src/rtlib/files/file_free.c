/*
 * Project: FreeBASIC runtime library
 * File: files/file_free.c
 * Purpose: freefile function.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL int fb_FileFree( void )
{
	int i;

	FB_LOCK();

    for( i = 1; i <= (FB_MAX_FILES - FB_RESERVED_FILES); i++ ) {
        FB_FILE *handle = FB_FILE_TO_HANDLE(i);
        if (handle->hooks==NULL) {
			FB_UNLOCK();
			return i;
        }
    }

	FB_UNLOCK();

	return 0;
}

/* end of files/file_free.c */
