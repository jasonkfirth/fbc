/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/file_hseteof.c

    Purpose:

        Truncate a Windows CE CRT stream at its current position.

    Responsibilities:

        - read the current 32-bit stream position
        - flush buffered data before changing the file length
        - truncate through the native handle returned by WinCE _fileno()

    This file intentionally does NOT contain:

        - desktop OS-handle conversion
        - 64-bit file positions unavailable in the WinCE CRT
        - cursor repositioning
*/

#include "../fb.h"

#include <windows.h>

int fb_hFileSetEofEx( FILE *stream )
{
	HANDLE handle;
	long position;
	DWORD seek_result;

	if( stream == NULL )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	position = ftell( stream );
	handle = (HANDLE)_fileno( stream );
	if( (position < 0) || (handle == NULL) ||
	    (handle == INVALID_HANDLE_VALUE) || (fflush( stream ) != 0) ) {
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	SetLastError( NO_ERROR );
	seek_result = SetFilePointer( handle, position, NULL, FILE_BEGIN );
	if( ((seek_result == INVALID_SET_FILE_POINTER) &&
	     (GetLastError( ) != NO_ERROR)) ||
	    (SetEndOfFile( handle ) == FALSE) )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of wince/file_hseteof.c */
