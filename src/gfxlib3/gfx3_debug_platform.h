/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_debug_platform.h

    Purpose:

        Define the private destination boundary for default gfxlib3 logging.

    Responsibilities:

        - accept one bounded, already-formatted log message
        - preserve the centralized logger's severity and label

    This file intentionally does NOT contain:

        - formatting buffers or variable arguments
        - graphics API error conversion
        - persistent log storage
*/

#ifndef __FB_GFX3_DEBUG_PLATFORM_H__
#define __FB_GFX3_DEBUG_PLATFORM_H__

#include "gfx3_debug.h"

void fb_gfx3_debug_platform_write(int level, const char *label,
	const char *message);

#endif

/* end of gfx3_debug_platform.h */
