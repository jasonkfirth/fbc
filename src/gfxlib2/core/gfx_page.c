/*
 * FreeBASIC gfxlib2: gfx_page.c
 * Purpose: Select and copy logical screen pages.
 * Responsibilities: driver selection, pages and mode queries.
 * This file intentionally does NOT contain drawing algorithms or native presentation.
 */

/* screen multiple pages handling */

#include "../fb_gfx.h"
#include "../fb_gfx_gl.h"
#include <stdint.h>

static void poll_events(void)
{
	if ((fb_gfx) && (fb_gfx->driver->poll_events))
		fb_gfx->driver->poll_events();
}

static void update_driver(void)
{
	if ((fb_gfx) && (fb_gfx->driver->update))
		fb_gfx->driver->update();
}

static void yield_host(void)
{
#ifdef HOST_JS
	/*
		Browser-hosted graphics programs often use SCREENCOPY or SCREENSET as
		their frame boundary.  Yield after presentation so pending browser
		timers, input, and canvas callbacks can run even if the program's main
		loop is otherwise CPU-bound.
	*/
	fb_Delay( 0 );
#endif
}

FBCALL int fb_GfxFlip(int from_page, int to_page)
{
	FB_GFXCTX *context;
	unsigned char *dest, *src;
	size_t size, text_size, text_cells, row_offset, column_offset;
	size_t start_offset, text_width, text_height;
	int i, lock = FALSE;
	int src_page, dest_page;

	FB_GRAPHICS_LOCK( );

	if (!fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

#ifndef DISABLE_OPENGL
	if (fb_gfx->driver->flip && (fb_gl_params.mode_2d != DRIVER_OGL_2D_AUTO_SYNC)) {
#else
	if (fb_gfx->driver->flip) {
#endif
		fb_gfx->driver->flip();
		poll_events();
		FB_GRAPHICS_UNLOCK( );
		yield_host();
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	poll_events();

	context = fb_hGetContext( );
	fb_hPrepareTarget(context, NULL);
	fb_hSetPixelTransfer(context, MASK_A_32);
	if ((context->view_x < 0) || (context->view_y < 0) ||
	    (context->view_w < 0) || (context->view_h < 0) ||
	    (fb_gfx->w < 0) || (fb_gfx->h < 0) ||
	    (context->view_x > fb_gfx->w) ||
	    (context->view_y > fb_gfx->h) ||
	    (context->view_w > fb_gfx->w - context->view_x) ||
	    (context->view_h > fb_gfx->h - context->view_y) ||
	    (fb_gfx->pitch <= 0) || (fb_gfx->bpp <= 0) ||
	    (fb_gfx->text_w < 0) || (fb_gfx->text_h < 0)) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	if (from_page < 0) {
		src = context->line[0];
		src_page = context->work_page;
	} else if (from_page >= fb_gfx->num_pages) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	} else {
		src = fb_gfx->page[from_page];
		src_page = from_page;
	}

	if (to_page < 0) {
		dest = fb_gfx->framebuffer;
		dest_page = fb_gfx->visible_page;
	} else if (to_page >= fb_gfx->num_pages) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	} else {
		dest = fb_gfx->page[to_page];
		dest_page = to_page;
	}

	if (!src || !dest) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	if (src == dest) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_OK);
	}
	if ((dest == fb_gfx->framebuffer) && (fb_gfx->lock_count==0))
		lock = TRUE;

	text_width = (size_t)fb_gfx->text_w;
	text_height = (size_t)fb_gfx->text_h;
	if ((text_width != 0) && (text_height > SIZE_MAX / text_width)) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	text_cells = text_width * text_height;
	if (text_cells > SIZE_MAX / sizeof(GFX_CHAR_CELL)) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	text_size = text_cells * sizeof(GFX_CHAR_CELL);
	if ((text_size > 0) &&
	    (!fb_gfx->con_pages || !fb_gfx->con_pages[dest_page] ||
	     !fb_gfx->con_pages[src_page])) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	if ((size_t)context->view_w > SIZE_MAX / (size_t)fb_gfx->bpp) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	size = (size_t)context->view_w * (size_t)fb_gfx->bpp;
	if ((size_t)context->view_x > SIZE_MAX / (size_t)fb_gfx->bpp) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	column_offset = (size_t)context->view_x * (size_t)fb_gfx->bpp;
	if ((column_offset > (size_t)fb_gfx->pitch) ||
	    (size > (size_t)fb_gfx->pitch - column_offset)) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	if ((size > 0) && (context->view_h > 0)) {
		if ((size_t)context->view_y > SIZE_MAX / (size_t)fb_gfx->pitch) {
			FB_GRAPHICS_UNLOCK( );
			return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
		}
		row_offset = (size_t)context->view_y * (size_t)fb_gfx->pitch;
		if (column_offset > SIZE_MAX - row_offset) {
			FB_GRAPHICS_UNLOCK( );
			return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
		}
		start_offset = row_offset + column_offset;
		src += start_offset;
		dest += start_offset;
	}

	if (lock)
		DRIVER_LOCK();
	if ((size > 0) && (context->view_h > 0)) {
		for (i = context->view_h; i; i--) {
			fb_hMemCpy(dest, src, size);
			dest += fb_gfx->pitch;
			src += fb_gfx->pitch;
		}
	}
	/* Copy the character cell pages too */
	if (text_size > 0)
		fb_hMemCpy(fb_gfx->con_pages[dest_page], fb_gfx->con_pages[src_page], text_size);
	if (lock) {
		if (context->view_h > 0)
			fb_hMemSet(fb_gfx->dirty + context->view_y, TRUE,
			           (size_t)context->view_h);
		DRIVER_UNLOCK();
		update_driver();
	}

	FB_GRAPHICS_UNLOCK( );
	yield_host();
	return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_GfxPageCopy(int from_page, int to_page)
{
	return fb_GfxFlip( from_page, to_page );
}

int fb_GfxPageSet(int work_page, int visible_page)
{
	FB_GFXCTX *context;
	int res;

	FB_GRAPHICS_LOCK( );

	if (!fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return -1;
	}

	poll_events();

	context = fb_hGetContext();
	res = context->work_page | (fb_gfx->visible_page << 8);

	fb_hPrepareTarget(context, NULL);
	fb_hSetPixelTransfer(context, MASK_A_32);

	if ((work_page < 0) && (visible_page < 0))
		work_page = visible_page = 0;

	if ((work_page >= 0) && (work_page < fb_gfx->num_pages)) {
		int i;
		for (i = 0; i < fb_gfx->h; i++)
			context->line[i] = fb_gfx->page[work_page] + (i * fb_gfx->pitch);
		context->work_page = work_page;
	}
	if ((visible_page >= 0) && (visible_page < fb_gfx->num_pages) && (visible_page != fb_gfx->visible_page)) {
		DRIVER_LOCK();
		fb_gfx->framebuffer = fb_gfx->page[visible_page];
		fb_gfx->visible_page = visible_page;
		if (fb_gfx->h > 0)
			fb_hMemSet(fb_gfx->dirty, TRUE, (size_t)fb_gfx->h);
		DRIVER_UNLOCK();
		update_driver();
	}

	FB_GRAPHICS_UNLOCK( );
	yield_host();
	return res;
}

/* end of gfx_page.c */
