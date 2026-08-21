/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/sys_getcwd.c

    Purpose:

        Return the Windows CE current directory as UTF-8.

    Responsibilities:

        - query the native UTF-16 current directory
        - convert it to the runtime's narrow-string representation
        - reject results that do not fit the caller's buffer

    This file intentionally does NOT contain:

        - drive-current-directory emulation
        - desktop ANSI API fallbacks
        - current-directory mutation
*/

#include "../fb.h"

#include <windows.h>

ssize_t fb_hGetCurrentDir( char *destination, ssize_t maximum_length )
{
	wchar_t wide_path[MAX_PATH];
	char *path;
	size_t length;
	int succeeded;

	if( (destination == NULL) || (maximum_length <= 0) )
		return 0;

	succeeded = fb_hWinCEGetCurrentDirectoryWC( wide_path,
	                                           ARRAY_SIZE( wide_path ) );
	if( !succeeded )
		return 0;

	path = fb_hConvertPathFromWC( wide_path, TRUE );
	if( path == NULL )
		return 0;

	length = strlen( path );
	if( length >= (size_t)maximum_length ) {
		free( path );
		return 0;
	}

	memcpy( destination, path, length + 1 );
	free( path );

	return (ssize_t)length;
}

/* end of wince/sys_getcwd.c */
