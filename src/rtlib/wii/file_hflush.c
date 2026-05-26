/*
    FreeBASIC runtime Wii file flushing
    -----------------------------------

    File: file_hflush.c

    Purpose:

        Flush the C library stream used by a FreeBASIC file handle.

    Responsibilities:

        - call fflush() for Wii/newlib FILE streams
        - translate C runtime failures into FreeBASIC runtime errors

    This file intentionally does NOT contain:

        - FreeBASIC file handle lookup
        - device dispatch
        - filesystem mounting
*/

#include "../fb.h"

int fb_hFileFlushEx(FILE *f)
{
	if (fflush(f) != 0)
		return fb_ErrorSetNum(FB_RTERROR_FILEIO);

	return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* end of file_hflush.c */
