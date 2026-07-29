/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_vga_api.c

    Purpose:

        Emulate the VGA palette ports used by historical BASIC graphics
        programs while an indexed gfxlib3 mode is active.

    Responsibilities:

        - emulate VGA DAC read/write address ports 0x3C7 and 0x3C8
        - emulate sequential six-bit RGB access through port 0x3C9
        - map status-port 0x3DA reads to synchronized presentation
        - install the FreeBASIC runtime INP and OUT hooks

    This file intentionally does NOT contain:

        - direct processor I/O instructions or arbitrary hardware access
        - non-VGA register emulation
        - palette drawing or presentation shader code
*/

#include "gfx3_api_internal.h"
#include "gfx3_vga_api.h"

FBCALL int fb_GfxWaitVSync(void);

enum FB_GFX3_VGA_PORT {
	FB_GFX3_VGA_DAC_READ_INDEX = 0x03C7,
	FB_GFX3_VGA_DAC_WRITE_INDEX = 0x03C8,
	FB_GFX3_VGA_DAC_DATA = 0x03C9,
	FB_GFX3_VGA_STATUS = 0x03DA
};

/* ------------------------------------------------------------------------- */
/* VGA palette register emulation                                            */
/* ------------------------------------------------------------------------- */

static uint32_t vga_palette_entries(const FB_GFX3_MODE *mode)
{
	return 1u << mode->depth;
}

static uint32_t vga_expand_component(uint32_t component)
{
	return (component & 0x3Fu) * 255u / 63u;
}

int fb_GfxIn(unsigned short port)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_MODE *mode;
	uint32_t entries;
	uint32_t color;
	int value = -1;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state == NULL) {
		FB_GRAPHICS_UNLOCK();
		return -1;
	}
	mode = state->mode;
	if ((port == FB_GFX3_VGA_DAC_DATA) && (mode->depth <= 8)) {
		entries = vga_palette_entries(mode);
		color = mode->palette[mode->vga_palette_index & (entries - 1u)];
		value = (int)((color >> mode->vga_palette_shift) & 0x3Fu);
		mode->vga_palette_shift += 8u;
		if (mode->vga_palette_shift > 18u) {
			mode->vga_palette_shift = 2u;
			mode->vga_palette_index =
				(mode->vga_palette_index + 1u) & (entries - 1u);
		}
	} else if (port == FB_GFX3_VGA_STATUS) {
		(void)fb_GfxWaitVSync();
		value = 8;
	}
	FB_GRAPHICS_UNLOCK();
	return value;
}

int fb_GfxOut(unsigned short port, unsigned char value)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_MODE *mode;
	uint32_t entries;
	uint32_t red;
	uint32_t green;
	uint32_t blue;
	int result = -1;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state == NULL) || (state->mode->depth > 8)) {
		FB_GRAPHICS_UNLOCK();
		return -1;
	}
	mode = state->mode;
	entries = vga_palette_entries(mode);
	switch (port) {
	case FB_GFX3_VGA_DAC_READ_INDEX:
	case FB_GFX3_VGA_DAC_WRITE_INDEX:
		mode->vga_palette_index = (uint32_t)value & (entries - 1u);
		mode->vga_palette_shift = 2u;
		mode->vga_palette_color = 0;
		result = 0;
		break;
	case FB_GFX3_VGA_DAC_DATA:
		mode->vga_palette_color |=
			((uint32_t)value & 0x3Fu) << mode->vga_palette_shift;
		mode->vga_palette_shift += 8u;
		if (mode->vga_palette_shift > 18u) {
			red = vga_expand_component(mode->vga_palette_color >> 2);
			green = vga_expand_component(mode->vga_palette_color >> 10);
			blue = vga_expand_component(mode->vga_palette_color >> 18);
			mode->palette[mode->vga_palette_index] = red |
				(green << 8) | (blue << 16);
			(void)fb_gfx3_context_set_palette(&mode->context,
				mode->palette);
			mode->vga_palette_index =
				(mode->vga_palette_index + 1u) & (entries - 1u);
			mode->vga_palette_shift = 2u;
			mode->vga_palette_color = 0;
		}
		result = 0;
		break;
	default:
		break;
	}
	FB_GRAPHICS_UNLOCK();
	return result;
}

/* ------------------------------------------------------------------------- */
/* Hook ownership                                                            */
/* ------------------------------------------------------------------------- */

void fb_gfx3_vga_install_hooks_locked(void)
{
	__fb_ctx.hooks.inproc = fb_GfxIn;
	__fb_ctx.hooks.outproc = fb_GfxOut;
}

/* end of gfx3_vga_api.c */
