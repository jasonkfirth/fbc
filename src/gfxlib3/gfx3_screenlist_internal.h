/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_screenlist_internal.h

    Purpose:

        Share checked packed-mode collection helpers with native SCREENLIST
        adapters.

    Responsibilities:

        - declare depth-equivalence matching
        - declare checked mode append and finalization helpers

    This file intentionally does NOT contain:

        - native display-system declarations
        - public SCREENLIST iterator state
        - storage owned by a platform adapter
*/

#ifndef __FB_GFX3_SCREENLIST_INTERNAL_H__
#define __FB_GFX3_SCREENLIST_INTERNAL_H__

#include "gfx3_screenlist.h"

int fb_gfx3_screenlist_depth_matches(uint32_t native_depth, int depth);
int fb_gfx3_screenlist_append(int **modes, size_t *count, size_t *capacity,
	int mode);
int fb_gfx3_screenlist_finish(int *modes, size_t count,
	int **result_modes, size_t *result_count);

#endif

/* end of gfx3_screenlist_internal.h */
