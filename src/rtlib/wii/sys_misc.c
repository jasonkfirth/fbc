/*
    FreeBASIC runtime Wii miscellaneous system hooks
    ------------------------------------------------

    File: sys_misc.c

    Purpose:

        Provide small system hooks that generic rtlib code expects.

    Responsibilities:

        - provide a harmless BEEP fallback
        - return a stable executable-name placeholder

    This file intentionally does NOT contain:

        - filesystem path discovery
        - process management
        - audio mixer control
*/

#include "../fb.h"
#include <string.h>

FBCALL void fb_Beep(void)
{
	fb_WiiVideoInit();
	fputc('\a', stdout);
	fflush(stdout);
}

char *fb_hGetExeName(char *dst, ssize_t maxlen)
{
	const char *name = "boot.dol";
	size_t len;

	if ((dst == NULL) || (maxlen <= 0))
		return dst;

	len = strlen(name);
	if (len >= (size_t)maxlen)
		len = (size_t)maxlen - 1;

	memcpy(dst, name, len);
	dst[len] = '\0';
	return dst;
}

/* end of sys_misc.c */
