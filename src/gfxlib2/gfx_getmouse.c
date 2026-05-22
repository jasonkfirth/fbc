/* GETMOUSE function support. */

#include "fb_gfx.h"

static void poll_events(void)
{
	if ((__fb_gfx) && (__fb_gfx->driver->poll_events))
		__fb_gfx->driver->poll_events();
}

int fb_GfxGetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
	int failure = TRUE;

	FB_GRAPHICS_LOCK( );

	poll_events( );

	if ((__fb_gfx) && (__fb_gfx->driver->get_mouse)) {
		DRIVER_LOCK();
		failure = __fb_gfx->driver->get_mouse(x, y, z, buttons, clip);
		if( y && !failure && (__fb_gfx->scanline_size != 1) ) {
			*y /= __fb_gfx->scanline_size;
		}
		DRIVER_UNLOCK();
	}

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
