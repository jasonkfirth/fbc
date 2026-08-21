/*
    FreeBASIC runtime library
    -------------------------

    File: aros/file_hseteof.c

    Purpose:

        Set an AROS file's length to its current logical position.

    Responsibilities:

        - resolve the DOS handle behind a POSIXC stream
        - read the position through the AROS-local stable seek path
        - resize the file with dos.library and preserve its position
        - translate failures to the FreeBASIC file error contract

    This file intentionally does NOT contain:

        - generic Unix truncation behavior
        - AROS architecture policy
        - file-device locking
*/

#include "../fb.h"

#include <dos/dos.h>
#include <proto/dos.h>

int fb_hFileSetEofEx( FILE *fp )
{
	BPTR file_handle;
	fb_off_t position;
	LONG native_position;

	position = fb_hArosGetFilePosition( fp );
	if( position < 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	native_position = (LONG)position;
	if( (fb_off_t)native_position != position ||
	    fb_hArosGetFileHandle( fp, &file_handle ) != 0 )
	{
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	if( SetFileSize( file_handle, native_position,
			 OFFSET_BEGINNING ) == -1 )
	{
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}
	Flush( file_handle );

	if( Seek( file_handle, native_position, OFFSET_BEGINNING ) == -1 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	clearerr( fp );
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of aros/file_hseteof.c */
