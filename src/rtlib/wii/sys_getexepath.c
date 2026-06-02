/*
    FreeBASIC runtime Wii executable path
    -------------------------------------

    File: sys_getexepath.c

    Purpose:

        Provide the EXEPATH runtime hook for Wii homebrew.

    Responsibilities:

        - return a stable filesystem directory for asset lookup
        - prefer the current directory selected during Wii runtime startup

    This file intentionally does NOT contain:

        - executable name discovery
        - file opening
        - directory enumeration
*/

#include "../fb.h"

char *fb_hGetExePath(char *dst, ssize_t maxlen)
{
	size_t len;

	if ((dst == NULL) || (maxlen <= 0))
		return NULL;

	if (getcwd(dst, (size_t)maxlen) == NULL) {
		if (maxlen < 5)
			return NULL;

		memcpy(dst, "sd:/", 5);
	}

	fb_hConvertPath(dst);

	len = strlen(dst);
	if ((len > 0) && (dst[len - 1] == '/'))
		dst[len - 1] = '\0';

	return dst;
}

/* end of sys_getexepath.c */
