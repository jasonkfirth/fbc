/*
    FreeBASIC runtime Wii file locking
    ----------------------------------

    File: file_hlock.c

    Purpose:

        Provide the file lock/unlock hooks required by the shared file
        runtime.

    Responsibilities:

        - report unsupported byte-range locking cleanly on Wii

    This file intentionally does NOT contain:

        - advisory lock emulation
        - network filesystem behavior
*/

#include "../fb.h"

int fb_hFileLock(FILE *f, fb_off_t inipos, fb_off_t size)
{
	return fb_ErrorSetNum(FB_RTERROR_FILEIO);
}

int fb_hFileUnlock(FILE *f, fb_off_t inipos, fb_off_t size)
{
	return fb_ErrorSetNum(FB_RTERROR_FILEIO);
}

/* end of file_hlock.c */
