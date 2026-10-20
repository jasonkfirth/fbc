/*
 * FreeBASIC gfxlib2: gfx_palettegetusing.c
 * Purpose: Implement the PALETTEGETUSING palette ABI entry point.
 * Responsibilities: palette lookup, conversion and compatibility state.
 * This file intentionally does NOT contain window creation or pixel rasterization.
 */

/* retrieve whole palette into an array */

#include "../fb_gfx.h"

FBCALL void fb_GfxPaletteGetUsing(int *data)
{
	int i, imax;

	FB_GRAPHICS_LOCK( );

	if (!__fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return;
	}

	imax = __fb_gfx->default_palette->colors;
	if(imax > (1 << __fb_gfx->depth))
		imax = (1 << __fb_gfx->depth);

	for (i = 0; i < imax; i++)
		data[i] = (__fb_gfx->device_palette[i] & 0xFCFCFC) >> 2;

	FB_GRAPHICS_UNLOCK( );
}

/* end of gfx_palettegetusing.c */
