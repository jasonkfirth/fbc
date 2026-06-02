/*
    FreeBASIC runtime Wii current directory
    ---------------------------------------

    File: sys_getcwd.c

    Purpose:

        Provide the CURDIR runtime hook for Wii homebrew.

    Responsibilities:

        - query newlib/libogc for the process current directory
        - return an empty result when no current directory is available
        - normalize path separators for FreeBASIC callers

    This file intentionally does NOT contain:

        - executable path discovery
        - directory enumeration
        - file opening
*/

#include "../fb.h"

ssize_t fb_hGetCurrentDir(char *dst, ssize_t maxlen)
{
	size_t len;

	if ((dst == NULL) || (maxlen <= 0))
		return 0;

	if (getcwd(dst, (size_t)maxlen) == NULL) {
		dst[0] = '\0';
		return 0;
	}

	fb_hConvertPath(dst);

	len = strlen(dst);
	return (ssize_t)len;
}

/* end of sys_getcwd.c */
