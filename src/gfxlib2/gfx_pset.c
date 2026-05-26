/* pixel plotting */

#include "fb_gfx.h"

FBCALL void fb_GfxPset(void *target, float fx, float fy, unsigned int color, int flags, int ispreset)
{
	FB_GFXCTX *context;
	int x, y;

	FB_GRAPHICS_LOCK( );

	if (!__fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return;
	}

	context = fb_hGetContext( );
	fb_hPrepareTarget(context, target);

	if (flags & DEFAULT_COLOR_1) {
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

	if (__fb_gfx->framebuffer == context->line[0]) {
		DRIVER_LOCK();
		context->put_pixel(context, x, y, color);
		__fb_gfx->dirty[y] = TRUE;
		DRIVER_UNLOCK();
	} else {
		context->put_pixel(context, x, y, color);
	}

	FB_GRAPHICS_UNLOCK( );
}
