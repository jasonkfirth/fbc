/*
    Project: FreeBASIC runtime Wii file copy support
    ---------------------------------------

    File: drivers/wii/file_copy.c

    Purpose:

        Connect FILECOPY to the portable C runtime implementation.

    Responsibilities:

        - expose the target-specific fb_FileCopy entry point
        - reuse the checked stdio copy implementation shared by other targets

    This file intentionally does NOT contain: - Wii filesystem mounting
        - path translation
        - directory copying
*/

#include "../../fb.h"

FBCALL int fb_FileCopy(const char *source, const char *destination)
{
	return fb_CrtFileCopy(source, destination);
}

/* end of drivers/wii/file_copy.c */
