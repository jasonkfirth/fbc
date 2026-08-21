/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/file_kill.c

    Purpose:

        Implement KILL with Windows CE error reporting.

    Responsibilities:

        - remove a file through the WinCE Unicode path layer
        - translate stable Win32 errors to FreeBASIC runtime errors
        - release temporary string descriptors on every path

    This file intentionally does NOT contain:

        - wildcard deletion
        - directory removal
        - desktop errno translation
*/

#include "../fb.h"

#include <windows.h>

FBCALL int fb_FileKill( FBSTRING *str )
{
	DWORD error = ERROR_SUCCESS;
	int result = -1;
	int runtime_error;

	if( str != NULL && str->data != NULL ) {
		SetLastError( ERROR_SUCCESS );
		result = fb_hRemoveFile( str->data );
		if( result != 0 )
			error = GetLastError();
	}

	if( str != NULL )
		fb_hStrDelTemp( str );

	if( result == 0 ) {
		runtime_error = FB_RTERROR_OK;
	} else {
		switch( error ) {
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			runtime_error = FB_RTERROR_FILENOTFOUND;
			break;
		case ERROR_ACCESS_DENIED:
			runtime_error = FB_RTERROR_NOPRIVILEGES;
			break;
		case ERROR_SHARING_VIOLATION:
			runtime_error = FB_RTERROR_FILEIO;
			break;
		default:
			runtime_error = FB_RTERROR_ILLEGALFUNCTIONCALL;
			break;
		}
	}

	return fb_ErrorSetNum( runtime_error );
}

/* end of wince/file_kill.c */
