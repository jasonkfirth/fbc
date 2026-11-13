/*
 * FreeBASIC gfxlib2: gfx_getmouse.c
 * Purpose: Query mouse state through the active graphics driver.
 * Responsibilities: input state, event delivery and runtime input hooks.
 * This file intentionally does NOT contain pixel rasterization or mode allocation.
 */

/* GETMOUSE function support. */

#include "../fb_gfx.h"

static void poll_events(void)
{
	if ((fb_gfx) && (fb_gfx->driver->poll_events))
		fb_gfx->driver->poll_events();
}

int fb_GfxGetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
	int failure = TRUE;
	int fast_mouse_state = FALSE;

	FB_GRAPHICS_LOCK( );

	poll_events( );

	/* Use a synchronized cached pointer without waiting for display updates. */
	if ((fb_gfx) && (fb_gfx->get_mouse_fast))
		fast_mouse_state = fb_gfx->get_mouse_fast(x, y, z, buttons, clip);
	if (fast_mouse_state > 0) {
		failure = FALSE;
	} else if ((fast_mouse_state == 0) && (fb_gfx) && (fb_gfx->driver->get_mouse)) {
		DRIVER_LOCK();
		failure = fb_gfx->driver->get_mouse(x, y, z, buttons, clip);
		DRIVER_UNLOCK();
	}
	if (y && !failure && (fb_gfx->scanline_size != 1))
		*y /= fb_gfx->scanline_size;

	FB_GRAPHICS_UNLOCK( );

	if (failure) {
		if (x) *x = -1;
		if (y) *y = -1;
		if (z) *z = -1;
		if (buttons) *buttons = -1;
		if (clip) *clip = -1;
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of gfx_getmouse.c */
