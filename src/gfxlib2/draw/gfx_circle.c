/*
 * FreeBASIC gfxlib2: gfx_circle.c
 * Purpose: Rasterize circles, ellipses and arcs.
 * Responsibilities: target preparation, clipping and drawing semantics.
 * This file intentionally does NOT contain native windows or event dispatch.
 */

/* CIRCLE statement */

#include "../fb_gfx.h"
#include <math.h>
#include <stdlib.h>
#include <stddef.h>

static void draw_scanline(FB_GFXCTX *ctx, int y, int x1, int x2, unsigned int color, int fill, char *filled)
{
	size_t scanline_width;

	if ((y >= ctx->view_y) && (y < ctx->view_y + ctx->view_h)) {
		if (fill) {
			if (!filled || (x2 < ctx->view_x) || (x1 >= ctx->view_x + ctx->view_w) ||
			    (filled[y - ctx->view_y]))
				return;
			filled[y - ctx->view_y] = TRUE;
			x1 = MAX(x1, ctx->view_x);
			x2 = MIN(x2, ctx->view_x + ctx->view_w - 1);
			if (x1 > x2)
				return;
			scanline_width = (size_t)x2 - (size_t)x1 + 1u;
			ctx->pixel_set(ctx->line[y] + ((ptrdiff_t)x1 * (ptrdiff_t)ctx->target_bpp), (int)color, scanline_width);
		}
		else {
			if ((x1 >= ctx->view_x) && (x1 < ctx->view_x + ctx->view_w))
				ctx->put_pixel(ctx, x1, y, color);
			if ((x2 >= ctx->view_x) && (x2 < ctx->view_x + ctx->view_w))
				ctx->put_pixel(ctx, x2, y, color);
		}
	}
}

static void draw_ellipse
	(
		FB_GFXCTX *ctx,
		int x,
		int y,
		float a,
		float b,
		unsigned int color,
		int fill,
		int *top,
		int *bottom
	)
{
	int d, x1, y1, x2, y2;
	int b_is_zero;
	long long dx, dy, aq, bq, r, rx, ry;
	char *filled = NULL;
	b_is_zero = (fpclassify(b) == FP_ZERO);

	x1 = (int)((float)x - a);
	x2 = (int)((float)x + a);
	y1 = y2 = y;

	/* Keep a large viewport fill guard off the stack. */
	if (fill || b_is_zero) {
		if (ctx->view_h <= 0) {
			*top = y;
			*bottom = y;
			return;
		}
		filled = (char *)calloc((size_t)ctx->view_h, sizeof(*filled));
		if (!filled) {
			*top = y;
			*bottom = y;
			return;
		}
	}

	if (b_is_zero) {
		draw_scanline(ctx, y, x1, x2, color, TRUE, filled);
		*top = y;
		*bottom = y;
		free(filled);
		return;
	}

	draw_scanline(ctx, y, x1, x2, color, fill, filled);

	aq = (long long)(a * a);
	bq = (long long)(b * b);
	dx = aq << 1;
	dy = bq << 1;
	r = (long long)(a * (float)bq);
	rx = r << 1;
	ry = 0;
	d = (int)a;

	while (d > 0) {
		if (r > 0) {
			y1++;
			y2--;
			ry += dx;
			r -= ry;
		}
		if (r <= 0) {
			d--;
			x1++;
			x2--;
			rx -= dy;
			r += rx;
		}
		draw_scanline(ctx, y1, x1, x2, color, fill, filled);
		draw_scanline(ctx, y2, x1, x2, color, fill, filled);
	}

	/* Tell caller exactly which rows were drawn so the SET_DIRTY() will be
	   correct */
	*top = y2;
	*bottom = y1;
	free(filled);
}

static void get_arc_point(float angle, float a, float b, int *x, int *y)
{
	float c, s;

	c = cosf(angle) * a;
	s = sinf(angle) * b;
	*x = CINT(c);
	*y = CINT(s);
}

/*
	Arc endpoints use an exact full-circle sentinel. Keep that test exact
	without treating nearby angles as a complete circle.
*/
static int gfx2_float_equal(float left, float right)
{
	return (left <= right) && (left >= right);
}

FBCALL void fb_GfxEllipse(void *target, float fx, float fy, float radius, unsigned int color, float aspect, float start, float end, int fill, int flags)
{
	FB_GFXCTX *context;
	int x, y, x1, y1, top, bottom;
	unsigned int orig_color;
	float a, b, orig_x, orig_y;

	FB_GRAPHICS_LOCK( );

	if (!fb_gfx || radius <= 0.0f) {
		FB_GRAPHICS_UNLOCK( );
		return;
	}

	context = fb_hGetContext();
	orig_x = fx;
	orig_y = fy;

	fb_hPrepareTarget(context, target);

	orig_color = color;
	if ((unsigned int)flags & DEFAULT_COLOR_1)
		color = (unsigned int)context->fg_color;
	else
		color = fb_hFixColor(context->target_bpp, color);
	
	fb_hSetPixelTransfer(context, color);

	fb_hFixRelative(context, flags, &fx, &fy, NULL, NULL);

	fb_hTranslateCoord(context, fx, fy, &x, &y);

	if (context->flags & CTX_WINDOW_ACTIVE) {
		/* radius gets multiplied by the VIEW/WINDOW width ratio (aspect is unchanged) */
		radius *= ((float)context->view_w / context->win_w);
	}

	if (fpclassify(aspect) == FP_ZERO)
		aspect = fb_gfx->aspect;

	if (aspect > 1.0f) {
		a = (radius / aspect);
		b = radius;
	} else {
		a = radius;
		b = (radius * aspect);
	}

	if ((fpclassify(start) != FP_ZERO) ||
	    !gfx2_float_equal(end, 3.141593f * 2.0f)) {
		float increment;
		const double full_turn = 2.0 * PI;

		if (start < 0.0f) {
			start = -start;
			get_arc_point(start, a, b, &x1, &y1);
			x1 = (int)(orig_x + (float)x1);
			y1 = (int)(orig_y - (float)y1);
			fb_GfxLine(target, orig_x, orig_y, (float)x1, (float)y1, orig_color, LINE_TYPE_LINE, 0xFFFF, COORD_TYPE_AA | (flags & ~COORD_TYPE_MASK));
		}
		if (end < 0.0f) {
			end = -end;
			get_arc_point(end, a, b, &x1, &y1);
			x1 = (int)(orig_x + (float)x1);
			y1 = (int)(orig_y - (float)y1);
			fb_GfxLine(target, orig_x, orig_y, (float)x1, (float)y1, orig_color, LINE_TYPE_LINE, 0xFFFF, COORD_TYPE_AA | (flags & ~COORD_TYPE_MASK));
		}
		
		while (end < start)
			end = (float)((double)end + full_turn);
		while ((double)(end - start) > full_turn)
			start = (float)((double)start + full_turn);

		increment = 1.0f / (sqrtf(a) * sqrtf(b) * 1.5f);

		DRIVER_LOCK();

		top = bottom = y;
		{
			int i;
			int steps = (int)((end - start) / increment + 0.5f);

			for (i = 0; i <= steps; i++) {
				float angle = start + ((float)i * increment);

				get_arc_point(angle, a, b, &x1, &y1);
				x1 = x + x1;
				y1 = y - y1;
				if ((x1 < context->view_x) || (x1 >= context->view_x + context->view_w) ||
				    (y1 < context->view_y) || (y1 >= context->view_y + context->view_h))
					continue;
				context->put_pixel(context, x1, y1, color);
				if (y1 > bottom)
					bottom = y1;
				if (y1 < top)
					top = y1;
			}
		}
	} else {
		DRIVER_LOCK();
		draw_ellipse(context, x, y, a, b, color, fill, &top, &bottom);
	}

	top = MID(context->view_y, top, context->view_y + context->view_h - 1);
	bottom = MID(context->view_y, bottom, context->view_y + context->view_h - 1);
	if( top > bottom )
		SWAP( top, bottom );

	SET_DIRTY(context, top, (size_t)bottom - (size_t)top + 1u);

	/* QuickBASIC leaves the graphics pen at the center of a circle or arc.
	   Negative arc angles draw radial lines through fb_GfxLine(), which updates
	   the pen to each endpoint, so restore the logical center afterwards. */
	context->last_x = fx;
	context->last_y = fy;

	DRIVER_UNLOCK();
	FB_GRAPHICS_UNLOCK( );
}

/* end of gfx_circle.c */
