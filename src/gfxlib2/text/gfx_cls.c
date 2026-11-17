/*
 * FreeBASIC gfxlib2: gfx_cls.c
 * Purpose: Clear graphics and text regions using BASIC CLS rules.
 * Responsibilities: text layout, character conversion and console hooks.
 * This file intentionally does NOT contain native window creation or event dispatch.
 */

/* screen clearing routine */

#include "../fb_gfx.h"
#include <limits.h>
#include <stdint.h>

void fb_GfxClear(int mode)
{
	FB_GFXCTX *context;
    int i, dirty, dirty_len;
    int reset_gfx_pos;
    int reset_console_start = 0, reset_console_end = 0;
    int new_x = -1, new_y = -1;
    size_t pixel_count, row_bytes, pixel_offset;
    int64_t wide_y_start, wide_y_end, wide_center_x, wide_center_y;

	FB_GRAPHICS_LOCK( );

	context = fb_hGetContext();
	fb_hPrepareTarget(context, NULL);
	fb_hSetPixelTransfer(context, context->bg_color);

    DRIVER_LOCK();

    if( mode == (int)0xFFFF0000 ) {
        if( context->flags & CTX_VIEWPORT_SET )
            mode = 1;
        else {
            int con_y_start = fb_ConsoleGetTopRow();
            int con_y_end = fb_ConsoleGetBotRow();
            if( con_y_start==0 && (con_y_end==fb_gfx->text_h-1) )
                /* No VIEW PRINT range set */
                mode = 0;
            else
                mode = 2;
        }
    }

	switch (mode) {
		case 0:
			/* Clear entire screen */
            {
                int cursor_y = fb_ConsoleGetTopRow();

                if( (fb_gfx->w <= 0) || (fb_gfx->h <= 0) ||
                    (context->line == NULL) || (context->line[0] == NULL) ||
                    ((size_t)fb_gfx->w > (SIZE_MAX / (size_t)fb_gfx->h)) )
                    goto cleanup;
                pixel_count = (size_t)fb_gfx->w * (size_t)fb_gfx->h;
                context->pixel_set(context->line[0], (int)context->bg_color, pixel_count);
                dirty = 0;
                dirty_len = fb_gfx->h;

                new_x = 0;
                new_y = cursor_y;

                reset_console_end = fb_gfx->text_h;

                reset_gfx_pos = TRUE;
            }
			break;

		case 2:
            /* Clear text viewport */
            {
                unsigned char *clear_line;
                int con_y_start = fb_ConsoleGetTopRow();
                int con_y_end = fb_ConsoleGetBotRow();
                int y_start, y_end, view_height;

                if( (fb_gfx->font == NULL) || (fb_gfx->font->h <= 0) ||
                    (fb_gfx->w <= 0) || (fb_gfx->h <= 0) ||
                    (fb_gfx->text_h <= 0) || (context->line == NULL) ||
                    (con_y_start < 0) || (con_y_end < con_y_start) ||
                    (con_y_end >= fb_gfx->text_h) )
                    goto cleanup;
                wide_y_start = (int64_t)con_y_start * (int64_t)fb_gfx->font->h;
                wide_y_end = ((int64_t)con_y_end + 1) * (int64_t)fb_gfx->font->h;
                if( (wide_y_start < 0) || (wide_y_end <= wide_y_start) ||
                    (wide_y_end > (int64_t)fb_gfx->h) ||
                    (wide_y_end > (int64_t)INT_MAX) )
                    goto cleanup;
                y_start = (int)wide_y_start;
                y_end = (int)wide_y_end;
                view_height = y_end - y_start;
                clear_line = context->line[y_start];
                if( (clear_line == NULL) ||
                    ((size_t)fb_gfx->w > (SIZE_MAX / (size_t)view_height)) )
                    goto cleanup;
                pixel_count = (size_t)fb_gfx->w * (size_t)view_height;

                context->pixel_set(clear_line, (int)context->bg_color, pixel_count);
                dirty = y_start;
                dirty_len = view_height;

                new_x = 0;
                new_y = con_y_start;

                reset_console_start = con_y_start;
                reset_console_end = con_y_end + 1;

                reset_gfx_pos = FALSE;
            }
            break;

		case 1:
		default:
            /* Clear graphics viewport if set */
            {
                unsigned char *dest;

                if( (fb_gfx->w <= 0) || (fb_gfx->h <= 0) ||
                    (fb_gfx->bpp <= 0) || (fb_gfx->pitch <= 0) ||
                    (context->line == NULL) || (context->view_x < 0) ||
                    (context->view_y < 0) || (context->view_w <= 0) ||
                    (context->view_h <= 0) ||
                    (context->view_x > fb_gfx->w) ||
                    (context->view_w > (fb_gfx->w - context->view_x)) ||
                    (context->view_y > fb_gfx->h) ||
                    (context->view_h > (fb_gfx->h - context->view_y)) ||
                    ((size_t)fb_gfx->w > (SIZE_MAX / (size_t)fb_gfx->bpp)) )
                    goto cleanup;
                dest = context->line[context->view_y];
                row_bytes = (size_t)fb_gfx->w * (size_t)fb_gfx->bpp;
                if( (row_bytes > (size_t)fb_gfx->pitch) ||
                    ((size_t)context->view_x > (SIZE_MAX / (size_t)fb_gfx->bpp)) ||
                    (dest == NULL) )
                    goto cleanup;
                pixel_offset = (size_t)context->view_x * (size_t)fb_gfx->bpp;
                if( pixel_offset > row_bytes )
                    goto cleanup;
                dest += pixel_offset;
                for (i = 0; i < context->view_h; i++) {
                    context->pixel_set(dest, (int)context->bg_color, (size_t)context->view_w);
                    dest += fb_gfx->pitch;
                }
                dirty = context->view_y;
                dirty_len = context->view_h;

                reset_gfx_pos = TRUE;
            }
			break;
	}
	if( (fb_gfx->dirty != NULL) && (dirty >= 0) && (dirty_len >= 0) &&
	    (dirty <= fb_gfx->h) && (dirty_len <= (fb_gfx->h - dirty)) )
		SET_DIRTY(context, dirty, (size_t)dirty_len);

    if( reset_gfx_pos ) {
        wide_center_x = (int64_t)context->view_x + (int64_t)(context->view_w >> 1);
        wide_center_y = (int64_t)context->view_y + (int64_t)(context->view_h >> 1);
        context->last_x = (float)wide_center_x;
        context->last_y = (float)wide_center_y;
    }

    if( (fb_gfx->text_w > 0) && (fb_gfx->text_h > 0) &&
        (reset_console_start >= 0) && (reset_console_end >= reset_console_start) &&
        (reset_console_end <= fb_gfx->text_h) )
		fb_hClearCharCells( 0, reset_console_start,
		                    fb_gfx->text_w, reset_console_end,
		                    context->work_page,
		                    32,
		                    context->fg_color, context->bg_color );

    if( new_x!=-1 || new_y!=-1 ) {
        fb_GfxLocate( new_y + 1, new_x + 1, -1 );
    }

cleanup:
    DRIVER_UNLOCK();
	FB_GRAPHICS_UNLOCK( );
}

/* end of gfx_cls.c */
