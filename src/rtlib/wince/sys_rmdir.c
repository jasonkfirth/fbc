/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/sys_rmdir.c

    Purpose:

        Implement RMDIR through the Windows CE path-conversion layer.

    Responsibilities:

        - pass FreeBASIC paths to the Unicode-aware WinCE directory helper
        - reject missing path strings
        - release temporary string arguments

    This file intentionally does NOT contain:

        - desktop CRT directory calls
        - path encoding conversions
        - recursive directory removal
*/

#include "../fb.h"

#include <windows.h>

FBCALL int fb_RmDir( FBSTRING *path )
{
	int result;

	if( (path != NULL) && (path->data != NULL) )
		result = fb_hRemoveDir( path->data );
	else {
		SetLastError( ERROR_INVALID_PARAMETER );
		result = -1;
	}

	/* Delete the descriptor when the caller supplied a temporary string. */
	fb_hStrDelTemp( path );

	return result;
}

/* end of wince/sys_rmdir.c */
