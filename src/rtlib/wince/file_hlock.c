/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/file_hlock.c

    Purpose:

        Provide the file-lock hooks required by FreeBASIC on Windows CE.

    Responsibilities:

        - validate lock and unlock requests
        - preserve successful single-process file I/O behavior

    This file intentionally does NOT contain:

        - desktop LockFile calls unavailable in Windows CE
        - inter-process advisory locking
        - CRT file-descriptor to OS-handle conversion
*/

#include "../fb.h"

int fb_hFileLock( FILE *stream, fb_off_t initial_position, fb_off_t size )
{
	if( (stream == NULL) || (initial_position < 0) || (size < 0) )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	/* Windows CE exposes no compatible byte-range file-lock primitive. */
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

int fb_hFileUnlock( FILE *stream, fb_off_t initial_position, fb_off_t size )
{
	if( (stream == NULL) || (initial_position < 0) || (size < 0) )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	/* Match the successful process-local lock operation above. */
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of wince/file_hlock.c */
