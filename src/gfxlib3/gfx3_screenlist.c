/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_screenlist.c

    Purpose:

        Provide SCREENLIST mode enumeration on targets without a native
        display-mode implementation.

    Responsibilities:

        - validate and clear result outputs
        - report that native mode enumeration is unavailable

    This file intentionally does NOT contain:

        - native display-system calls
        - standard-mode fallback behavior
        - mode sorting or collection helpers
*/

#include "gfx3_screenlist.h"

int fb_gfx3_platform_screenlist_modes(int depth, int **modes,
	size_t *mode_count)
{
	(void)depth;
	if (modes != NULL)
		*modes = NULL;
	if (mode_count != NULL)
		*mode_count = 0;
	return FB_GFX3_UNSUPPORTED;
}

/* end of gfx3_screenlist.c */
