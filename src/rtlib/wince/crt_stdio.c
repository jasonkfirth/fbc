/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/crt_stdio.c

    Purpose:

        Supply narrow C file-management entry points omitted by Coredll.

    Responsibilities:

        - convert UTF-8 paths through the Windows CE runtime path layer
        - implement remove() with DeleteFileW()
        - implement rename() with MoveFileW()

    This file intentionally does NOT contain:

        - stream I/O wrappers already exported by Coredll
        - FreeBASIC file-number error translation
        - desktop MSVCRT compatibility
*/

#include "../fb.h"

#include <windows.h>

int remove( const char *path )
{
	return fb_hRemoveFile( path );
}

int rename( const char *old_path, const char *new_path )
{
	wchar_t *old_wide;
	wchar_t *new_wide;
	int result = -1;

	if( old_path == NULL || new_path == NULL )
		return -1;

	old_wide = fb_hConvertPathToWC( old_path, NULL );
	if( old_wide == NULL )
		return -1;

	new_wide = fb_hConvertPathToWC( new_path, NULL );
	if( new_wide != NULL ) {
		result = MoveFileW( old_wide, new_wide ) ? 0 : -1;
		free( new_wide );
	}

	free( old_wide );
	return result;
}

/* end of wince/crt_stdio.c */
