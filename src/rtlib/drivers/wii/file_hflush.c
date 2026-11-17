/*
    Project: FreeBASIC runtime Wii file flushing
    -----------------------------------

    File: drivers/wii/file_hflush.c

    Purpose:

        Flush the C library stream used by a FreeBASIC file handle.

    Responsibilities:

        - flush Wii/newlib streams and synchronize their backing descriptors
        - translate C runtime failures into FreeBASIC runtime errors

    This file intentionally does NOT contain: - FreeBASIC file handle lookup
        - device dispatch
        - filesystem mounting
*/

#include "../../fb.h"
#include <unistd.h>

int fb_hFileFlushEx(FILE *f)
{
	if (f == NULL)
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

	if (fflush(f) != 0)
		return fb_ErrorSetNum(FB_RTERROR_FILEIO);
	/* libfat retains directory size and cluster metadata behind stdio. A
	   second fflush alone does not implement FILEFLUSH's system-buffer flag. */
	if (fsync(fileno(f)) != 0)
		return fb_ErrorSetNum(FB_RTERROR_FILEIO);

	return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* end of drivers/wii/file_hflush.c */
