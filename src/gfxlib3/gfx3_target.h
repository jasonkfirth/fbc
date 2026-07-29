/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_target.h

    Purpose:

        Define the small target-policy boundary used by shared gfxlib3 code.

    Responsibilities:

        - validate platform-specific SCREEN flag restrictions
        - provide the target's ordered renderer preference
        - select the renderer named by the legacy OpenGL SCREEN flag
        - provide a bounded CPU image snapshot budget

    This file intentionally does NOT contain:

        - native window, input, or graphics API declarations
        - backend initialization
        - public FreeBASIC declarations
*/

#ifndef __FB_GFX3_TARGET_H__
#define __FB_GFX3_TARGET_H__

#include "gfx3_backend.h"

int fb_gfx3_target_screen_flags_valid(uint32_t flags);
size_t fb_gfx3_target_backend_default_list(
	const FB_GFX3_BACKEND_VTABLE **backends, size_t capacity);
const FB_GFX3_BACKEND_VTABLE *fb_gfx3_target_opengl_backend(void);
size_t fb_gfx3_target_image_cache_snapshot_budget(void);

#endif

/* end of gfx3_target.h */
