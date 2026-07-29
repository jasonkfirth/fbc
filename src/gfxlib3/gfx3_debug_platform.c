/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_debug_platform.c

    Purpose:

        Write default gfxlib3 diagnostics to the hosted standard-error stream.

    Responsibilities:

        - emit one labeled line for each centralized logger message
        - avoid ownership or buffering beyond the current call

    This file intentionally does NOT contain:

        - message formatting policy
        - Android logcat integration
        - persistent log-file management
*/

#include "gfx3_debug_platform.h"

void fb_gfx3_debug_platform_write(int level, const char *label,
	const char *message)
{
	(void)level;
	if ((label != NULL) && (message != NULL))
		fprintf(stderr, "gfxlib3 %s: %s\n", label, message);
}

/* end of gfx3_debug_platform.c */
