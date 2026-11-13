/*
 * FreeBASIC gfxlib2: gfx_readstr.c
 * Purpose: Read a graphics console row as text.
 * Responsibilities: text layout, character conversion and console hooks.
 * This file intentionally does NOT contain native window creation or event dispatch.
 */

/* gfx hook for INPUT support */

#include "../fb_gfx.h"

static void move_back(void)
{
	fb_gfx->cursor_x--;
	if (fb_gfx->cursor_x < 0) {
		fb_gfx->cursor_x = fb_gfx->text_w - 1;
		fb_gfx->cursor_y--;
	}
}

char *fb_GfxReadStr(char *buffer, ssize_t maxlen)
{
	int key;
	ssize_t len = 0;
	/* CP437 full-block glyph, kept as a byte regardless of char signedness. */
	char cursor_normal[2] = "\333";
	char cursor_backspace[3] = "\333 ";
	char space[2] = { ' ', '\0' };
	char character[2] = { 0, '\0' };
	char *cursor = cursor_normal;

	FB_GRAPHICS_LOCK( );

	if (!fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return NULL;
	}

	do {
		fb_GfxPrintBuffer(cursor, 0);

		if (cursor == cursor_backspace) {
			move_back();
			cursor = cursor_normal;
		}
		move_back();

		key = fb_Getkey();
		if (key < 0x100) {
			if (key == 8) {
				if (len > 0) {
					cursor = cursor_backspace;
					move_back();
					if (fb_gfx->cursor_y < 0) {
						fb_gfx->cursor_y = fb_gfx->cursor_x = 0;
						cursor = cursor_normal;
					}
					len--;
				}
			}
			else if ((key != 7) && (len < maxlen - 1)) {
				if (key == 13) {
					fb_GfxPrintBuffer(space, 0);
					move_back();
				}
				buffer[len++] = key;
				character[0] = key;
				fb_GfxPrintBuffer(character, 0);
			}
		}
	} while (key != 13);

	buffer[len] = '\0';

	FB_GRAPHICS_UNLOCK( );
	return buffer;
}

/* end of gfx_readstr.c */
