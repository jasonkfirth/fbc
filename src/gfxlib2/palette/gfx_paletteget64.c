/*
 * FreeBASIC gfxlib2: gfx_paletteget64.c
 * Purpose: Implement the PALETTEGET64 palette ABI entry point.
 * Responsibilities: palette lookup, conversion and compatibility state.
 * This file intentionally does NOT contain window creation or pixel rasterization.
 */

/* retrieve certain color from current palette */

#include "../fb_gfx.h"

FBCALL void fb_GfxPaletteGet64(int index, long long *r, long long *g, long long *b)
{
	unsigned int color;

	FB_GRAPHICS_LOCK( );

	if (!__fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return;
	}

	index &= (__fb_gfx->default_palette->colors - 1);
	color = __fb_gfx->device_palette[index];
	if (!g) {
		*r = (color & 0xFCFCFC) >> 2;
	} else {
		*r = color & 0xFF;
		*g = (color & 0xFF00) >> 8;
		*b = (color & 0xFF0000) >> 16;
	}

	FB_GRAPHICS_UNLOCK( );
}

/* end of gfx_paletteget64.c */
