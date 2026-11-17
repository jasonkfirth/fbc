/*
 * FreeBASIC gfxlib2: gfx_point.c
 * Purpose: Read target pixels or the current drawing position.
 * Responsibilities: target preparation, clipping and drawing semantics.
 * This file intentionally does NOT contain native windows or event dispatch.
 */

/* pixel retrieving */

#include "../fb_gfx.h"
#include <limits.h>
#include <math.h>

/* POINT reserves this exact value to request a cursor coordinate. */
static int gfx2_float_equal(float left, float right)
{
	return (left <= right) && (left >= right);
}

FBCALL unsigned int fb_GfxPoint(void *target, float fx, float fy)
{
	FB_GFXCTX *context;
	int x, y;
	unsigned int color;
	float cursor_value;
	double cursor_function;

	FB_GRAPHICS_LOCK( );

	if (!fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return UINT_MAX;
	}

	if( gfx2_float_equal(fy, -8388607.0f) ) {
		FB_GRAPHICS_UNLOCK( );
		cursor_function = (double)fx;
		if( !isfinite( cursor_function ) ||
			(cursor_function < (double)INT_MIN) ||
			(cursor_function > (double)INT_MAX) )
			return UINT_MAX;

		cursor_value = fb_GfxCursor( (int)cursor_function );
		if( !isfinite( (double)cursor_value ) || (cursor_value < 0.0f) ||
			((double)cursor_value > (double)UINT_MAX) )
			return UINT_MAX;

		return (unsigned int)cursor_value;
	}

	context = fb_hGetContext( );
	fb_hPrepareTarget(context, target);
	fb_hSetPixelTransfer(context, MASK_A_32);

	fb_hTranslateCoord(context, fx, fy, &x, &y);

	if ((x < context->view_x) || (y < context->view_y) ||
	    (x >= context->view_x + context->view_w) || (y >= context->view_y + context->view_h)) {
		FB_GRAPHICS_UNLOCK( );
		return UINT_MAX;
	}

	DRIVER_LOCK();
	color = context->get_pixel(context, x, y);
	DRIVER_UNLOCK();

	if (fb_gfx->depth == 16)
		/* approximate: for each component we also report high bits in lower bits of new value */
		color = (((color & 0x001F) << 3) | ((color >> 2) & 0x7) |
			 ((color & 0x07E0) << 5) | ((color >> 1) & 0x300) |
			 ((color & 0xF800) << 8) | ((color << 3) & 0x70000));

	FB_GRAPHICS_UNLOCK( );

	/* Returning a RGBA value or palette index in a signed 32bit/64bit
	   Integer, without sign-extension on 64bit.
	   (palette indices can't be negative anyways, and sign-extending an
	   RGBA value would cause it to compare unequal to a normal,
	   non-sign-extended one) */
	return color;
}

/* end of gfx_point.c */
