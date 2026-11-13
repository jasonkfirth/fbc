/*
 * FreeBASIC gfxlib2: gfx_vgaemu.c
 * Purpose: Emulate the supported VGA palette port operations.
 * Responsibilities: palette lookup, conversion and compatibility state.
 * This file intentionally does NOT contain window creation or pixel rasterization.
 */

/* VGA in/out emulation */

#include "../fb_gfx.h"

static int idx = 0, shift = 2, color = 0;

int fb_GfxIn(unsigned short port)
{
	int value = -1;

	FB_GRAPHICS_LOCK( );

	if (!fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return -1;
	}

	switch (port) {
		case 0x3C9:
			if (fb_gfx->depth > 8)
				break;
			value = (fb_gfx->device_palette[idx] >> shift) & 0x3F;
			shift += 8;
			if (shift > 18) {
				shift = 2;
				idx++;
				idx &= (fb_gfx->default_palette->colors - 1);
			}
			break;

		case 0x3DA:
			if (fb_gfx->driver->wait_vsync)
				fb_gfx->driver->wait_vsync();
			value = 8;
			break;
	}

	FB_GRAPHICS_UNLOCK( );
	return value;
}

int fb_GfxOut(unsigned short port, unsigned char value)
{
	int i, r, g, b;

	FB_GRAPHICS_LOCK( );

	if ((!fb_gfx) || (fb_gfx->depth > 8)) {
		FB_GRAPHICS_UNLOCK( );
		return -1;
	}

	switch (port) {
		case 0x3C7:
		case 0x3C8:
			idx = value & (fb_gfx->default_palette->colors - 1);
			shift = 2;
			color = 0;
			break;

		case 0x3C9:
			color |= ((value & 0x3F) << shift);
			shift += 8;
			if (shift > 18) {
				if (fb_gfx->default_palette == &fb_palette[FB_PALETTE_256])
					fb_GfxPalette(idx, (color >> 2) & 0x3F3F3F, -1, -1);
				else {
					DRIVER_LOCK();
					r = color & 0xFF;
					g = (color >> 8) & 0xFF;
					b = (color >> 16) & 0xFF;
					fb_gfx->palette[idx] = color;
					for (i = 0; i < (1 << fb_gfx->depth); i++) {
						if (fb_gfx->color_association[i] == idx) {
							fb_gfx->device_palette[i] = color;
							if (fb_gfx->driver->set_palette)
								fb_gfx->driver->set_palette(i, r, g, b);
						}
					}
					fb_hMemSet(fb_gfx->dirty, TRUE, fb_gfx->h);
					DRIVER_UNLOCK();
				}
				shift = 2;
				color = 0;
				idx++;
				idx &= (fb_gfx->default_palette->colors - 1);
			}
			break;
		
		default:
			FB_GRAPHICS_UNLOCK( );
			return -1;
	}

	FB_GRAPHICS_UNLOCK( );
	return 0;
}

/* end of gfx_vgaemu.c */
