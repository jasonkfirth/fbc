/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_screenlist.h

    Purpose:

        Define the native display-mode enumeration boundary for SCREENLIST.

    Responsibilities:

        - return packed width/height modes allocated for one query
        - keep native display headers out of public graphics entry points
        - allow callers to retain deterministic fallbacks when unsupported

    This file intentionally does NOT contain:

        - SCREENLIST iterator state
        - standard SCREEN mode tables
        - fullscreen or display-mode changes
*/

#ifndef __FB_GFX3_SCREENLIST_H__
#define __FB_GFX3_SCREENLIST_H__

#include "fb_gfx3.h"

int fb_gfx3_platform_screenlist_modes(int depth, int **modes,
	size_t *mode_count);

#endif

/* end of gfx3_screenlist.h */
