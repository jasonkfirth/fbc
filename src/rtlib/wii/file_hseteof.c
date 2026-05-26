/*
    FreeBASIC runtime Wii file truncation
    -------------------------------------

    File: file_hseteof.c

    Purpose:

        Implement SETEOF for Wii/newlib streams.

    Responsibilities:

        - truncate the underlying file descriptor at the current stream
          position when newlib exposes ftruncate()
        - translate failures into FreeBASIC runtime errors

    This file intentionally does NOT contain:

        - file handle lookup
        - device dispatch
*/

#include "../fb.h"
#include <unistd.h>

int fb_hFileSetEofEx(FILE *f)
{
	fb_off_t pos;

	if (f == NULL)
		return fb_ErrorSetNum(FB_RTERROR_FILEIO);

	pos = ftello(f);
	if (pos < 0)
		return fb_ErrorSetNum(FB_RTERROR_FILEIO);

	if (ftruncate(fileno(f), pos) != 0)
		return fb_ErrorSetNum(FB_RTERROR_FILEIO);

	return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* end of file_hseteof.c */
