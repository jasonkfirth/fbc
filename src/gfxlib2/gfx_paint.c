/*
    FreeBASIC gfxlib2: gfx_paint.c

    Discover and fill connected non-border spans for PAINT and PaintPattern.
    Discovery completes before any pixels are written, including on allocation
    failure. Driver locking covers both reads and writes. This file does not
    own image allocation, graphics mode selection or legacy color-plane decoding.
*/

#include "fb_gfx.h"
#include "gfx_paint_pattern.h"
#include <limits.h>
#include <math.h>

typedef struct SPAN
{
	int y, x1, x2;
	struct SPAN *row_next;
	struct SPAN *next;
} SPAN;

static SPAN *add_span(FB_GFXCTX *context, SPAN **span, int *x, int y, unsigned int border_color, int *result)
{
	SPAN *s;
	int x1, x2;

	x1 = x2 = *x;
	while ((x1 > context->view_x) && (context->get_pixel(context, x1 - 1, y) != border_color))
		x1--;
	while ((x2 < context->view_x + context->view_w - 1) && (context->get_pixel(context, x2 + 1, y) != border_color))
		x2++;
	*x = x2 + 1;
	for (s = span[y - context->view_y]; s; s = s->row_next) {
		if ((x1 == s->x1) && (x2 == s->x2))
			return NULL;
	}
	s = (SPAN *)malloc(sizeof(SPAN));
	if (!s) {
		*result = FB_RTERROR_OUTOFMEM;
		return NULL;
	}
	s->x1 = x1;
	s->x2 = x2;
	s->y = y;
	s->next = NULL;
	s->row_next = span[y - context->view_y];
	span[y - context->view_y] = s;

	return s;
}

static int paint(void *target, float fx, float fy, unsigned int color,
	unsigned int border_color, unsigned char *data, size_t pattern_size,
	int mode, int flags)
{
	FB_GFXCTX *context;
	int size, x, y;
	int result = FB_RTERROR_OK;
	float translated_x, translated_y;
	double checked_x, checked_y;
	unsigned char *dest, *src;
	SPAN **span = NULL, *s, *tail, *head;

	FB_GRAPHICS_LOCK( );

	if (!__fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	}

	context = fb_hGetContext( );
	fb_hPrepareTarget(context, target);

	if (flags & DEFAULT_COLOR_1)
		color = context->fg_color;
	else
		color = fb_hFixColor(context->target_bpp, color);

	if (flags & DEFAULT_COLOR_2)
		border_color = color;
	else
		border_color = fb_hFixColor(context->target_bpp, border_color);

	fb_hSetPixelTransfer(context,color);
	fb_hFixRelative(context, flags, &fx, &fy, NULL, NULL);
	/* Check the native float transform before it reaches CINT and int offsets. */
	translated_x = fx;
	translated_y = fy;
	if (context->flags & CTX_WINDOW_ACTIVE) {
		translated_x = ((fx - context->win_x) * (context->view_w - 1)) / context->win_w;
		translated_y = ((fy - context->win_y) * (context->view_h - 1)) / context->win_h;
	}
	checked_x = translated_x;
	checked_y = translated_y;
	if ((context->flags & (CTX_WINDOW_ACTIVE | CTX_WINDOW_SCREEN)) == CTX_WINDOW_ACTIVE)
		checked_y = (double)context->view_h - 1 - checked_y;
	if ((context->flags & CTX_VIEW_SCREEN) == 0) {
		checked_x += context->view_x;
		checked_y += context->view_y;
	}
	if (!isfinite(translated_x) || !isfinite(translated_y) ||
	    (double)translated_x < INT_MIN || (double)translated_x > INT_MAX ||
	    (double)translated_y < INT_MIN || (double)translated_y > INT_MAX ||
	    checked_x < INT_MIN || checked_x > INT_MAX ||
	    checked_y < INT_MIN || checked_y > INT_MAX) {
		FB_GRAPHICS_UNLOCK();
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	}
	fb_hTranslateCoord(context, fx, fy, &x, &y);

	if ((x < context->view_x) || (x >= context->view_x + context->view_w) ||
	    (y < context->view_y) || (y >= context->view_y + context->view_h)) {
		FB_GRAPHICS_UNLOCK( );
		return FB_RTERROR_OK;
	}

	DRIVER_LOCK();

	if (context->get_pixel(context, x, y) == border_color) {
		goto done;
	}

	if (context->view_h <= 0 || (size_t)context->view_h > SIZE_MAX / sizeof(*span)) {
		result = FB_RTERROR_ILLEGALFUNCTIONCALL;
		goto done;
	}
	span = (SPAN **)calloc(context->view_h, sizeof(*span));
	if (!span) {
		result = FB_RTERROR_OUTOFMEM;
		goto done;
	}

	tail = head = add_span(context, span, &x, y, border_color, &result);

	/* Find all spans to paint */
	while (tail && result == FB_RTERROR_OK) {
		if (tail->y - 1 >= context->view_y) {
			for (x = tail->x1; x <= tail->x2; x++) {
				if (context->get_pixel(context, x, tail->y - 1) != border_color) {
					s = add_span(context, span, &x, tail->y - 1, border_color, &result);
					if (s) {
						head->next = s;
						head = s;
					}
				}
			}
		}
		if (tail->y + 1 < context->view_y + context->view_h) {
			for (x = tail->x1; x <= tail->x2; x++) {
				if (context->get_pixel(context, x, tail->y + 1) != border_color) {
					s = add_span(context, span, &x, tail->y + 1, border_color, &result);
					if (s) {
						head->next = s;
						head = s;
					}
				}
			}
		}
		tail = tail->next;
	}

	if (result != FB_RTERROR_OK)
		goto done;

	/* Fill spans */
	for (y = context->view_y; y < context->view_y + context->view_h; y++) {
		for (s = span[y - context->view_y]; s; s = s->row_next) {

			dest = context->line[s->y] + (s->x1 * context->target_bpp);

			if (mode == PAINT_TYPE_FILL)
				context->pixel_set(dest, color, s->x2 - s->x1 + 1);
			else if (mode == FB_GFX_PAINT_PACKED) {
				for (x = s->x1; x <= s->x2; x++) {
					unsigned int pixel = fb_hGfxPackedPatternColor(data, pattern_size, x, s->y);
					pixel = fb_hFixColor(context->target_bpp, pixel);
					context->pixel_set(dest, pixel, 1);
					dest += context->target_bpp;
				}
			} else {
				src = data + (((s->y & 0x7) << 3) * context->target_bpp);
				if (s->x1 & 0x7) {
					if ((s->x1 & ~0x7) == (s->x2 & ~0x7))
						size = s->x2 - s->x1 + 1;
					else
						size = 8 - (s->x1 & 0x7);
					fb_hPixelCpy(dest, src + ((s->x1 & 0x7) * context->target_bpp), size);
					dest += size * context->target_bpp;
				}
				s->x2++;
				for (x = (s->x1 + 7) >> 3; x < (s->x2 & ~0x7) >> 3; x++) {
					fb_hPixelCpy(dest, src, 8);
					dest += 8 * context->target_bpp;
				}
				if ((s->x2 & 0x7) && ((s->x1 & ~0x7) != (s->x2 & ~0x7)))
					fb_hPixelCpy(dest, src, s->x2 & 0x7);
			}

			if (__fb_gfx->framebuffer == context->line[0])
				__fb_gfx->dirty[y] = TRUE;
		}
	}
done:
	if (span) {
		for (y = 0; y < context->view_h; y++) {
			for (s = span[y]; s; s = tail) {
				tail = s->row_next;
				free(s);
			}
		}
		free(span);
	}
	DRIVER_UNLOCK();
	FB_GRAPHICS_UNLOCK( );
	return result;
}

FBCALL void fb_GfxPaint(void *target, float fx, float fy, unsigned int color,
	unsigned int border_color, FBSTRING *pattern, int mode, int flags)
{
	unsigned char data[256] = { 0 };
	ssize_t pattern_size = (pattern && pattern->data) ? FB_STRSIZE(pattern) : 0;
	if (mode == PAINT_TYPE_PATTERN && pattern_size > 0)
		memcpy(data, pattern->data, MIN(sizeof(data), (size_t)pattern_size));
	fb_hStrDelTemp(pattern);
	/* PAINT historically has a void ABI and does not change Err in gfxlib2. */
	(void)paint(target, fx, fy, color, border_color, data, sizeof(data), mode, flags);
}

FBCALL int fb_GfxPaintPattern(void *target, float x, float y, FBSTRING *pattern,
	unsigned int foreground, unsigned int background, unsigned int border,
	int relative)
{
	unsigned char data[FB_GFX_PAINT_PACKED_HEADER + FB_GFX_PAINT_PACKED_MAX_ROWS];
	int size = fb_hGfxPackPattern(data, pattern, foreground, background);
	fb_hStrDelTemp(pattern);
	if (!size || !isfinite(x) || !isfinite(y))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	return fb_ErrorSetNum(paint(target, x, y, foreground, border, data, size,
		FB_GFX_PAINT_PACKED, relative ? COORD_TYPE_R : COORD_TYPE_A));
}

/* end of gfx_paint.c */
