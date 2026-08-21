/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/sys_chdir.c

    Purpose:

        Implement CHDIR through the Windows CE path-conversion layer.

    Responsibilities:

        - pass FreeBASIC paths to the Unicode-aware WinCE directory helper
        - release temporary string arguments

    This file intentionally does NOT contain:

        - desktop CRT directory calls
        - path encoding conversions
        - current-directory emulation
*/

#include "../fb.h"

#include <windows.h>

FBCALL int fb_ChDir( FBSTRING *path )
{
	int result;

	if( (path != NULL) && (path->data != NULL) )
		result = fb_hChangeDir( path->data );
	else {
		SetLastError( ERROR_INVALID_PARAMETER );
		result = -1;
	}

	/* Delete the descriptor when the caller supplied a temporary string. */
	fb_hStrDelTemp( path );

	return result;
}

/* end of wince/sys_chdir.c */
