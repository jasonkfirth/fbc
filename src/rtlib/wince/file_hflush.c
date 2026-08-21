/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/file_hflush.c

    Purpose:

        Flush FreeBASIC file streams through the Windows CE CRT.

    Responsibilities:

        - reject missing stream handles
        - flush buffered bytes to the CRT's backing file
        - translate failure to a FreeBASIC runtime error

    This file intentionally does NOT contain:

        - desktop CRT-to-Win32 handle conversion
        - volume-wide cache flushing
        - file closing
*/

#include "../fb.h"

int fb_hFileFlushEx( FILE *stream )
{
	if( (stream == NULL) || (fflush( stream ) != 0) )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of wince/file_hflush.c */
