/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/sys_getexename.c

    Purpose:

        Return the current Windows CE executable name as UTF-8.

    Responsibilities:

        - query the native module filename through its UTF-16 API
        - copy the complete path into the caller's bounded buffer
        - return a pointer to the filename component

    This file intentionally does NOT contain:

        - executable path removal
        - desktop ANSI API fallbacks
        - filesystem probing
*/

#include "../fb.h"

#include <windows.h>

char *fb_hGetExeName( char *destination, ssize_t maximum_length )
{
	wchar_t wide_path[MAX_PATH + 1];
	char *path;
	char *name;
	size_t length;

	if( (destination == NULL) || (maximum_length <= 0) )
		return NULL;

	if( !fb_hWinCEGetExecutablePathWC( wide_path,
	                                  ARRAY_SIZE( wide_path ) ) ) {
		destination[0] = '\0';
		return NULL;
	}

	path = fb_hConvertPathFromWC( wide_path, TRUE );
	if( path == NULL ) {
		destination[0] = '\0';
		return NULL;
	}

	length = strlen( path );
	if( length >= (size_t)maximum_length ) {
		free( path );
		destination[0] = '\0';
		return NULL;
	}

	memcpy( destination, path, length + 1 );
	free( path );

	name = strrchr( destination, '\\' );
	return (name != NULL) ? name + 1 : destination;
}

/* end of wince/sys_getexename.c */
