/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/wince/file_hlock.c

    Purpose:

        Provide the file-lock hooks required by FreeBASIC on Windows CE.

    Responsibilities:

        - report that byte-range locking is unavailable on Windows CE

    This file intentionally does NOT contain: - desktop LockFile calls unavailable in Windows CE
        - inter-process advisory locking
        - CRT file-descriptor to OS-handle conversion
*/

#include "../../fb.h"

int fb_hFileLock( FILE *stream, fb_off_t initial_position, fb_off_t size )
{
	if( (stream == NULL) || (initial_position < 0) || (size < 0) )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	/* Do not report success when the platform cannot enforce the lock. */
	return fb_ErrorSetNum( FB_RTERROR_FILEIO );
}

int fb_hFileUnlock( FILE *stream, fb_off_t initial_position, fb_off_t size )
{
	if( (stream == NULL) || (initial_position < 0) || (size < 0) )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	/* Windows CE exposes no compatible byte-range unlock primitive. */
	return fb_ErrorSetNum( FB_RTERROR_FILEIO );
}

/* end of drivers/wince/file_hlock.c */
