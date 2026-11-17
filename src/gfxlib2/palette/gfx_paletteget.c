/*
 * FreeBASIC gfxlib2: gfx_paletteget.c
 * Purpose: Implement the PALETTEGET palette ABI entry point.
 * Responsibilities: palette lookup, conversion and compatibility state.
 * This file intentionally does NOT contain window creation or pixel rasterization.
 */

/* retrieve certain color from current palette */

#include "../fb_gfx.h"

FBCALL void fb_GfxPaletteGet(int index, int *r, int *g, int *b)
{
	unsigned int color;

	FB_GRAPHICS_LOCK( );

	if (!fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return;
	}

	index &= (fb_gfx->default_palette->colors - 1);
	color = fb_gfx->device_palette[index];
	if (!g) {
		*r = (int)((color & 0x00FCFCFCu) >> 2);
	} else {
		*r = (int)(color & 0xFFu);
		*g = (int)((color & 0xFF00u) >> 8);
		*b = (int)((color & 0xFF0000u) >> 16);
	}

	FB_GRAPHICS_UNLOCK( );
}

/* end of gfx_paletteget.c */
