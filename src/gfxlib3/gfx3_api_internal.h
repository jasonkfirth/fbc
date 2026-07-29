/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_api_internal.h

    Purpose:

        Share the active FreeBASIC compatibility state between focused public
        ABI implementation files.

    Responsibilities:

        - expose the caller-local draw state while FB_GRAPHICS_LOCK is held
        - expose the common gfxlib3 to rtlib error translation

    This file intentionally does NOT contain:

        - public FreeBASIC declarations
        - mode ownership or TLS allocation
        - renderer or image algorithms
*/

#ifndef __FB_GFX3_API_INTERNAL_H__
#define __FB_GFX3_API_INTERNAL_H__

#include "gfx3_compat.h"

FB_GFX3_DRAW_STATE *fb_gfx3_api_get_draw_state_locked(void);
int fb_gfx3_api_apply_pending_resize_locked(FB_GFX3_DRAW_STATE *state);
int fb_gfx3_api_runtime_error(int result);
const char *fb_gfx3_api_get_window_title_locked(void);
void fb_gfx3_api_set_window_title_locked(const char *title, size_t length);
/* Capture immutable GL/GLES context facts after a successful mode open. */
void fb_gfx3_api_gl_control_mode_opened_locked(const FB_GFX3_MODE *mode);

#endif

/* end of gfx3_api_internal.h */
