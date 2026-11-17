/*
 * FreeBASIC gfxlib2: gfx_pset.c
 * Purpose: Plot pixels using BASIC coordinate and color rules.
 * Responsibilities: target preparation, clipping, alpha blending and ordered pixel batches.
 * This file intentionally does NOT contain native windows or event dispatch.
 */

/* pixel plotting */

#include "../fb_gfx.h"
#include <limits.h>
#include <math.h>

static int pset_coord_is_valid(FB_GFXCTX *context, float fx, float fy)
{
	double checked_x;
	double checked_y;
	float translated_x = fx;
	float translated_y = fy;

	if (context->flags & CTX_WINDOW_ACTIVE) {
		if (!isfinite(context->win_w) || !isfinite(context->win_h) ||
		    !(fabsf(context->win_w) > 0.0f) ||
		    !(fabsf(context->win_h) > 0.0f) ||
		    (context->view_w <= 0) || (context->view_h <= 0))
			return FALSE;
		translated_x = ((fx - context->win_x) * (float)(context->view_w - 1)) /
			context->win_w;
		translated_y = ((fy - context->win_y) * (float)(context->view_h - 1)) /
			context->win_h;
	}

	checked_x = translated_x;
	checked_y = translated_y;
	if ((context->flags & (CTX_WINDOW_ACTIVE | CTX_WINDOW_SCREEN)) ==
	    CTX_WINDOW_ACTIVE)
		checked_y = (double)context->view_h - 1.0 - checked_y;
	if ((context->flags & CTX_VIEW_SCREEN) == 0) {
		checked_x += context->view_x;
		checked_y += context->view_y;
	}

	return isfinite(translated_x) && isfinite(translated_y) &&
		(checked_x >= (double)INT_MIN) &&
		(checked_x <= (double)INT_MAX) &&
		(checked_y >= (double)INT_MIN) &&
		(checked_y <= (double)INT_MAX);
}

FBCALL void fb_GfxPset(void *target, float fx, float fy, unsigned int color, int flags, int ispreset)
{
	FB_GFXCTX *context;
	int x, y;

	FB_GRAPHICS_LOCK( );

	if (!fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return;
	}

	context = fb_hGetContext( );
	fb_hPrepareTarget(context, target);

	if ((unsigned int)flags & DEFAULT_COLOR_1) {
		if (ispreset)
			color = context->bg_color;
		else
			color = context->fg_color;
	} else {
		color = fb_hFixColor(context->target_bpp, color);
	}

	fb_hSetPixelTransfer(context, color);

	if (((flags & COORD_TYPE_MASK) == COORD_TYPE_AA) &&
	    ((context->flags & CTX_WINDOW_ACTIVE) == 0)) {
		context->last_x = fx;
		context->last_y = fy;

		x = CINT(fx);
		y = CINT(fy);

		if ((context->flags & CTX_VIEW_SCREEN) == 0) {
			x += context->view_x;
			y += context->view_y;
		}
	} else {
		fb_hFixRelative(context, flags, &fx, &fy, NULL, NULL);
		fb_hTranslateCoord(context, fx, fy, &x, &y);
	}

	if ((x < context->view_x) || (y < context->view_y) ||
	    (x >= context->view_x + context->view_w) || (y >= context->view_y + context->view_h)) {
		FB_GRAPHICS_UNLOCK( );
		return;
	}

	if (fb_gfx->framebuffer == context->line[0]) {
		DRIVER_LOCK();
		context->put_pixel(context, x, y, color);
		fb_gfx->dirty[y] = TRUE;
		DRIVER_UNLOCK();
	} else {
		context->put_pixel(context, x, y, color);
	}

	FB_GRAPHICS_UNLOCK( );
}

/*
	Draw a bounded ordered group of screen pixels while acquiring gfxlib2's
	graphics and driver locks once. Partial alpha uses the same integer blend
	formula as the editor's PSET fallback.
*/
FBCALL int fb_GfxDrawPointBatch(const FB_GFX_POINT_BATCH_ITEM *points, int count)
{
	FB_GFXCTX *context;
	int64_t view_right;
	int64_t view_bottom;
	int index;

	if ((count < 0) || (count > 16384) || ((count > 0) && (points == NULL)))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	if (count == 0)
		return fb_ErrorSetNum(FB_RTERROR_OK);

	FB_GRAPHICS_LOCK();
	if (!fb_gfx) {
		FB_GRAPHICS_UNLOCK();
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	context = fb_hGetContext();
	fb_hPrepareTarget(context, NULL);
	if ((fb_gfx->depth != 32) || (context->target_bpp != 4)) {
		FB_GRAPHICS_UNLOCK();
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	if ((context->view_w <= 0) || (context->view_h <= 0)) {
		FB_GRAPHICS_UNLOCK();
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	view_right = (int64_t)context->view_x + (int64_t)context->view_w;
	view_bottom = (int64_t)context->view_y + (int64_t)context->view_h;

	fb_hSetPixelTransfer(context, MASK_A_32);
	DRIVER_LOCK();
	for (index = 0; index < count; index++) {
		float fx = (float)points[index].x;
		float fy = (float)points[index].y;
		unsigned int alpha = points[index].alpha;
		unsigned int source = points[index].color;
		unsigned int color;
		unsigned int destination;
		unsigned int red, green, blue;
		int x, y;

		if (alpha == 0)
			continue;
		if (alpha > 255)
			alpha = 255;

		fb_hFixRelative(context, COORD_TYPE_AA, &fx, &fy, NULL, NULL);
		if (!pset_coord_is_valid(context, fx, fy))
			continue;
		fb_hTranslateCoord(context, fx, fy, &x, &y);
		if (((int64_t)x < (int64_t)context->view_x) ||
		    ((int64_t)y < (int64_t)context->view_y) ||
		    ((int64_t)x >= view_right) || ((int64_t)y >= view_bottom))
			continue;

		if (alpha < 255) {
			destination = context->get_pixel(context, x, y);
			red = ((((source >> 16) & 0xFF) * alpha) +
			       (((destination >> 16) & 0xFF) * (255 - alpha))) / 255;
			green = ((((source >> 8) & 0xFF) * alpha) +
				 (((destination >> 8) & 0xFF) * (255 - alpha))) / 255;
			blue = (((source & 0xFF) * alpha) +
				((destination & 0xFF) * (255 - alpha))) / 255;
			color = fb_hFixColor(context->target_bpp,
				MASK_A_32 | (red << 16) | (green << 8) | blue);
		} else {
			color = fb_hFixColor(context->target_bpp, source);
		}

		context->put_pixel(context, x, y, color);
		if (fb_gfx->framebuffer == context->line[0])
			fb_gfx->dirty[y] = TRUE;
	}
	DRIVER_UNLOCK();
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* end of gfx_pset.c */
