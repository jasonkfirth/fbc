/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_interop_api.c

    Purpose:

        Expose the explicit render-thread interop boundary used by programs
        that need a native OpenGL or OpenGL ES extension beyond gfxlib3's
        portable GPU-surface API.

    Responsibilities:

        - validate and synchronously queue application interop callbacks
        - preserve renderer-thread ownership of the live graphics context
        - map gfxlib3 failures onto the established FreeBASIC error state

    This file intentionally does NOT contain:

        - OpenGL declarations, symbol resolution, or backend state mutation
        - a raw context handle or a cross-thread GL escape hatch
        - ordinary gfxlib2 compatibility entry points
*/

#include "gfx3_api_internal.h"

/* ------------------------------------------------------------------------- */
/* Public render-thread interop                                              */
/* ------------------------------------------------------------------------- */

FBCALL int fb_Gfx3RunOnRenderThread(FB_GFX3_INTEROP_CALLBACK callback,
	void *user_data)
{
	FB_GFX3_DRAW_STATE *state;
	int result = FB_GFX3_INVALID;

	if (callback == NULL)
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state != NULL)
		result = fb_gfx3_context_run_interop_callback(&state->mode->context,
			callback, user_data);
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

/* end of gfx3_interop_api.c */
