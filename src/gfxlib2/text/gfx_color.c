/*
 * FreeBASIC gfxlib2: gfx_color.c
 * Purpose: Set graphics foreground and background colors.
 * Responsibilities: text layout, character conversion and console hooks.
 * This file intentionally does NOT contain native window creation or event dispatch.
 */

/* color statement */

#include "../fb_gfx.h"

unsigned int fb_GfxColor(unsigned int fg, unsigned int bg, int flags)
{
	FB_GFXCTX *context;
	unsigned int cur;

	FB_GRAPHICS_LOCK( );

	context = fb_hGetContext( );

	if (fb_gfx->depth <= 8) {
		cur = context->fg_color | (context->bg_color << 16);
	} else if (fb_gfx->depth == 16) {
		unsigned c = context->fg_color;
		cur = (((c & 0x001F) << 3) | ((c >> 2) & 0x7) |
			((c & 0x07E0) << 5) | ((c >> 1) & 0x300) |
			((c & 0xF800) << 8) | ((c << 3) & 0x70000) | 0xff000000);
	} else {
		cur = context->fg_color;
	}

	switch (fb_gfx->mode_num) {
		case 1:
			if (!(flags & FB_COLOR_BG_DEFAULT))
				fb_GfxPalette(-(4 - (int)(bg & 0x3u)), 0, 0, 0);
			if (!(flags & FB_COLOR_FG_DEFAULT))
				fb_GfxPalette(0, (int)fg, -1, -1);
			break;
		
		case 7:
		case 8:
		case 9:
			if (!(flags & FB_COLOR_FG_DEFAULT))
				context->fg_color = (fg & 0xF);
			if (!(flags & FB_COLOR_BG_DEFAULT))
				fb_GfxPalette(0, (int)bg, -1, -1);
			break;
		
		default:
			if (!(flags & FB_COLOR_FG_DEFAULT)) {
				if (fb_gfx->depth > 8)
					context->fg_color = fb_hMakeColor(fb_gfx->bpp, fg,
						(int)((fg >> 16) & 0xFFu),
						(int)((fg >> 8) & 0xFFu), (int)(fg & 0xFFu));
				else
					context->fg_color =
						(fg & (unsigned int)BPP_MASK(context->target_bpp));
			}
			if (!(flags & FB_COLOR_BG_DEFAULT)) {
				if (fb_gfx->depth > 8)
					context->bg_color = fb_hMakeColor(fb_gfx->bpp, bg,
						(int)((bg >> 16) & 0xFFu),
						(int)((bg >> 8) & 0xFFu), (int)(bg & 0xFFu));
				else
					context->bg_color =
						(bg & (unsigned int)BPP_MASK(context->target_bpp));
			}
			break;
	}

	FB_GRAPHICS_UNLOCK( );
	return cur;
}

/* end of gfx_color.c */
