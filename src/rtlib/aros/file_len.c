/*
    FreeBASIC runtime library
    -------------------------

    File: aros/file_len.c

    Purpose:

        Report a named file's length without AROS's faulty 64-bit seek path.

    Responsibilities:

        - open the requested file as a binary stream
        - reuse the AROS file-device metadata query
        - preserve FreeBASIC's FileLen error contract

    This file intentionally does NOT contain:

        - generic Unix FileLen behavior
        - file positioning
        - architecture policy
*/

#include "../fb.h"

fb_off_t fb_FileLenEx( const char *filename )
{
	FILE *fp;
	fb_off_t length;

	fp = fb_hOpenFile( filename, "rb" );
	if( fp == NULL )
	{
		fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		return 0;
	}

	length = fb_DevFileGetSize( fp, FB_FILE_MODE_BINARY,
				    FB_FILE_ENCOD_ASCII, FALSE );
	fclose( fp );

	if( length < 0 )
	{
		fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		return 0;
	}

	fb_ErrorSetNum( FB_RTERROR_OK );
	return length;
}

FBCALL long long fb_FileLen( const char *filename )
{
	return fb_FileLenEx( filename );
}

/* end of aros/file_len.c */
