/*
    FreeBASIC graphics library
    -------------------------
    File: gfx_multikey.c
    Purpose: Query the current pressed state of a graphics scan code.
    Responsibilities: Poll foreground input drivers and read protected key state.
    This file intentionally does NOT contain native event translation.
*/

#include "fb_gfx.h"

int fb_GfxMultikey(int scancode)
{
	int result = FB_FALSE;

	FB_GRAPHICS_LOCK( );

	/* Some drivers receive events on the calling thread. Match GETMOUSE and
	   INKEY so MULTIKEY-only loops observe queued presses and releases, even
	   when SLEEP uses its delay-only form. Poll before taking the driver lock. */
	if ((__fb_gfx) && (__fb_gfx->driver->poll_events))
		__fb_gfx->driver->poll_events();

	if ((__fb_gfx) && (scancode >= 0) && (scancode < 128)) {
		DRIVER_LOCK();
		result = (__fb_gfx->key[scancode]? FB_TRUE : FB_FALSE);
		DRIVER_UNLOCK();
	}

	FB_GRAPHICS_UNLOCK( );

	return result;
}

/* end of gfx_multikey.c */
