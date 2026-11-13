/*
 * FreeBASIC gfxlib2: gfx_paletteusing64.c
 * Purpose: Implement the PALETTEUSING64 palette ABI entry point.
 * Responsibilities: palette lookup, conversion and compatibility state.
 * This file intentionally does NOT contain window creation or pixel rasterization.
 */

#include "../fb_gfx.h"

FBCALL void fb_GfxPaletteUsing64(long long *data)
{
	int i;

	FB_GRAPHICS_LOCK( );

	if ((!fb_gfx) || (fb_gfx->depth > 8)) {
		FB_GRAPHICS_UNLOCK( );
		return;
	}

	DRIVER_LOCK();

	for (i = 0; i < (1 << fb_gfx->depth); i++) {
		if (data[i] >= 0)
			fb_hSetPaletteColor(i, (int)data[i]);
	}

	fb_hMemSet(fb_gfx->dirty, TRUE, fb_gfx->h);

	DRIVER_UNLOCK();

	FB_GRAPHICS_UNLOCK( );
}

/* end of gfx_paletteusing64.c */
