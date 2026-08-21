/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/sys_getexepath.c

    Purpose:

        Return the current Windows CE executable directory as UTF-8.

    Responsibilities:

        - query and convert the native UTF-16 module filename
        - remove the final filename component
        - reject results that do not fit the caller's bounded buffer

    This file intentionally does NOT contain:

        - current-directory fallback
        - desktop ANSI API calls
        - executable search-path policy
*/

#include "../fb.h"

#include <windows.h>

char *fb_hGetExePath( char *destination, ssize_t maximum_length )
{
	wchar_t wide_path[MAX_PATH + 1];
	char *path;
	char *separator;
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

	separator = strrchr( destination, '\\' );
	if( separator == NULL ) {
		destination[0] = '\0';
		return NULL;
	}

	*separator = '\0';
	return separator;
}

/* end of wince/sys_getexepath.c */
