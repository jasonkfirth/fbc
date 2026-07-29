/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_image.c

    Purpose:

        Implement the CPU-visible FB.IMAGE compatibility surface used when a
        BASIC program explicitly requests addressable image memory.

    Responsibilities:

        - parse current and QB image headers with checked geometry
        - read and write 8, 16, and 32-bit pixels without alignment assumptions
        - draw initial pixel, line, box, ellipse, and arc primitives
        - apply the built-in and custom PUT modes to CPU image rows

    This file intentionally does NOT contain:

        - public FreeBASIC ABI wrappers
        - GPU uploads, downloads, or screen-page selection
        - image file codecs, PAINT, DRAW, or text rendering
*/

#include "gfx3_image.h"

#include <math.h>

/* ------------------------------------------------------------------------- */
/* Header and pixel access                                                   */
/* ------------------------------------------------------------------------- */

static uint32_t image_load_u32(const void *source)
{
	uint32_t value;

	memcpy(&value, source, sizeof(value));
	return value;
}

static int image_multiply_u32(uint32_t left, uint32_t right,
	uint32_t *result)
{
	uint64_t product;

	if (result == NULL)
		return FB_GFX3_INVALID;
	product = (uint64_t)left * right;
	if (product > UINT32_MAX)
		return FB_GFX3_INVALID;
	*result = (uint32_t)product;
	return FB_GFX3_OK;
}

static uint16_t image_load_u16(const void *source)
{
	uint16_t value;

	memcpy(&value, source, sizeof(value));
	return value;
}

static void image_store_u32(void *destination, uint32_t value)
{
	memcpy(destination, &value, sizeof(value));
}

static void image_store_u16(void *destination, uint16_t value)
{
	memcpy(destination, &value, sizeof(value));
}

uint32_t fb_gfx3_image_get_pixel_raw(const FB_GFX3_IMAGE_VIEW *view,
	int x, int y)
{
	const unsigned char *pixel = view->pixels + ((size_t)y * view->pitch) +
		((size_t)x * view->bytes_per_pixel);

	switch (view->bytes_per_pixel) {
	case 1:
		return pixel[0];
	case 2:
		return image_load_u16(pixel);
	default:
		return image_load_u32(pixel);
	}
}

void fb_gfx3_image_set_pixel_raw(FB_GFX3_IMAGE_VIEW *view, int x, int y,
	uint32_t color)
{
	unsigned char *pixel;

	if ((x < 0) || (y < 0) || ((uint32_t)x >= view->width) ||
	    ((uint32_t)y >= view->height))
		return;
	pixel = view->pixels + ((size_t)y * view->pitch) +
		((size_t)x * view->bytes_per_pixel);
	switch (view->bytes_per_pixel) {
	case 1:
		pixel[0] = (unsigned char)color;
		break;
	case 2:
		image_store_u16(pixel, (uint16_t)color);
		break;
	default:
		image_store_u32(pixel, color);
		break;
	}
}

void fb_gfx3_image_set_primitive_pixel(FB_GFX3_IMAGE_VIEW *view,
	const FB_GFX3_DRAW_STATE *state, int x, int y, uint32_t color)
{
	if ((view == NULL) || (x < 0) || (y < 0) ||
	    ((uint32_t)x >= view->width) || ((uint32_t)y >= view->height))
		return;
	if (fb_gfx3_compat_primitive_flags(state,
	    view->bytes_per_pixel * 8u, color) & FB_GFX3_PRIMITIVE_ALPHA_BLEND)
		color = fb_gfx3_alpha_primitive_pixel(color,
			fb_gfx3_image_get_pixel_raw(view, x, y));
	fb_gfx3_image_set_pixel_raw(view, x, y, color);
}

static void image_fill_span(FB_GFX3_IMAGE_VIEW *view,
	const FB_GFX3_DRAW_STATE *state, int y, int x1, int x2, uint32_t color)
{
	int x;

	if ((y < 0) || ((uint32_t)y >= view->height) || (x2 < 0) ||
	    (x1 >= (int)view->width))
		return;
	if (x1 < 0)
		x1 = 0;
	if (x2 >= (int)view->width)
		x2 = (int)view->width - 1;
	for (x = x1; x <= x2; x++)
		fb_gfx3_image_set_primitive_pixel(view, state, x, y, color);
}

int fb_gfx3_image_parse(void *image, FB_GFX3_IMAGE_VIEW *view)
{
	unsigned char *header = (unsigned char *)image;
	uint32_t first_word;
	uint32_t row_bytes;
	size_t image_bytes;

	if ((image == NULL) || (view == NULL))
		return FB_GFX3_INVALID;
	memset(view, 0, sizeof(*view));
	first_word = image_load_u32(header);
	view->header = header;
	if (first_word == FB_GFX3_IMAGE_HEADER_NEW) {
		view->bytes_per_pixel = image_load_u32(header + 4);
		view->width = image_load_u32(header + 8);
		view->height = image_load_u32(header + 12);
		view->pitch = image_load_u32(header + 16);
		view->header_size = FB_GFX3_IMAGE_NEW_HEADER_SIZE;
	} else {
		view->bytes_per_pixel = first_word & 7u;
		view->width = (first_word >> 3) & 0x1FFFu;
		view->height = (first_word >> 16) & 0xFFFFu;
		if (image_multiply_u32(view->width, view->bytes_per_pixel,
		    &view->pitch) != FB_GFX3_OK)
			return FB_GFX3_INVALID;
		view->header_size = 4;
	}
	if (!((view->bytes_per_pixel == 1) ||
	      (view->bytes_per_pixel == 2) ||
	      (view->bytes_per_pixel == 4)) ||
	    (view->width == 0) || (view->height == 0) ||
	    (view->width > INT_MAX) || (view->height > INT_MAX) ||
	    (image_multiply_u32(view->width, view->bytes_per_pixel,
	     &row_bytes) != FB_GFX3_OK) || (view->pitch < row_bytes) ||
	    (fb_gfx3_size_multiply(view->pitch, view->height,
	     &image_bytes) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	view->pixels = header + view->header_size;
	return FB_GFX3_OK;
}

int fb_gfx3_image_initialize_header(void *image, int use_new_header,
	uint32_t width, uint32_t height, uint32_t bytes_per_pixel,
	uint32_t pitch)
{
	unsigned char *header = (unsigned char *)image;
	uint32_t old_header;
	uint32_t row_bytes;

	if ((header == NULL) || (width == 0) || (height == 0) ||
	    !((bytes_per_pixel == 1) || (bytes_per_pixel == 2) ||
	      (bytes_per_pixel == 4)) ||
	    (image_multiply_u32(width, bytes_per_pixel, &row_bytes) !=
	     FB_GFX3_OK) || (pitch < row_bytes))
		return FB_GFX3_INVALID;
	if (use_new_header) {
		memset(header, 0, FB_GFX3_IMAGE_NEW_HEADER_SIZE);
		image_store_u32(header, FB_GFX3_IMAGE_HEADER_NEW);
		image_store_u32(header + 4, bytes_per_pixel);
		image_store_u32(header + 8, width);
		image_store_u32(header + 12, height);
		image_store_u32(header + 16, pitch);
		return FB_GFX3_OK;
	}
	if ((width > 0x1FFFu) || (height > 0xFFFFu) ||
	    (pitch != row_bytes))
		return FB_GFX3_INVALID;
	old_header = bytes_per_pixel | (width << 3) | (height << 16);
	image_store_u32(header, old_header);
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Owned-image cache metadata                                                */
/* ------------------------------------------------------------------------- */

/*
	gfxlib2 reserves the final twelve bytes of the new PUT header for backend
	state. gfxlib3 never exposes a GPU texture through those fields, so images
	created by this runtime use them only as a private mutation generation. The
	marker distinguishes owned images from BLOAD data and caller-built headers.
	Calling IMAGEINFO exposes writable pixels, so such an image intentionally
	returns to exact snapshot comparison before every cached PUT.
*/
enum {
	FB_GFX3_IMAGE_CACHE_METADATA_GENERATION_OFFSET = 20u,
	FB_GFX3_IMAGE_CACHE_METADATA_MAGIC_OFFSET = 24u,
	FB_GFX3_IMAGE_CACHE_METADATA_FLAGS_OFFSET = 28u
};

static int image_cache_metadata_is_owned(const FB_GFX3_IMAGE_VIEW *view)
{
	return (view != NULL) && (view->header != NULL) &&
		(view->header_size == FB_GFX3_IMAGE_NEW_HEADER_SIZE) &&
		(image_load_u32(view->header +
		 FB_GFX3_IMAGE_CACHE_METADATA_MAGIC_OFFSET) ==
		 FB_GFX3_IMAGE_CACHE_METADATA_MAGIC);
}

void fb_gfx3_image_cache_metadata_initialize(FB_GFX3_IMAGE_VIEW *view)
{
	if ((view == NULL) || (view->header == NULL) ||
	    (view->header_size != FB_GFX3_IMAGE_NEW_HEADER_SIZE))
		return;
	image_store_u32(view->header + FB_GFX3_IMAGE_CACHE_METADATA_GENERATION_OFFSET,
		1u);
	image_store_u32(view->header + FB_GFX3_IMAGE_CACHE_METADATA_MAGIC_OFFSET,
		FB_GFX3_IMAGE_CACHE_METADATA_MAGIC);
	image_store_u32(view->header + FB_GFX3_IMAGE_CACHE_METADATA_FLAGS_OFFSET,
		0u);
}

int fb_gfx3_image_cache_metadata_get(const FB_GFX3_IMAGE_VIEW *view,
	uint32_t *generation, int *external_write)
{
	if ((generation == NULL) || (external_write == NULL) ||
	    !image_cache_metadata_is_owned(view))
		return FALSE;
	*generation = image_load_u32(view->header +
		FB_GFX3_IMAGE_CACHE_METADATA_GENERATION_OFFSET);
	*external_write = (image_load_u32(view->header +
		FB_GFX3_IMAGE_CACHE_METADATA_FLAGS_OFFSET) &
		FB_GFX3_IMAGE_CACHE_METADATA_EXTERNAL_WRITE) != 0u;
	return TRUE;
}

void fb_gfx3_image_cache_metadata_touch(FB_GFX3_IMAGE_VIEW *view)
{
	uint32_t generation;

	if (!image_cache_metadata_is_owned(view))
		return;
	generation = image_load_u32(view->header +
		FB_GFX3_IMAGE_CACHE_METADATA_GENERATION_OFFSET);
	if (generation == UINT32_MAX)
		generation = 1u;
	else
		generation++;
	image_store_u32(view->header +
		FB_GFX3_IMAGE_CACHE_METADATA_GENERATION_OFFSET, generation);
}

void fb_gfx3_image_cache_metadata_mark_external(FB_GFX3_IMAGE_VIEW *view)
{
	uint32_t flags;

	if (!image_cache_metadata_is_owned(view))
		return;
	flags = image_load_u32(view->header +
		FB_GFX3_IMAGE_CACHE_METADATA_FLAGS_OFFSET);
	flags |= FB_GFX3_IMAGE_CACHE_METADATA_EXTERNAL_WRITE;
	image_store_u32(view->header + FB_GFX3_IMAGE_CACHE_METADATA_FLAGS_OFFSET,
		flags);
}

uint32_t fb_gfx3_image_fix_color(uint32_t bytes_per_pixel, uint32_t color)
{
	if (bytes_per_pixel == 1)
		return color & 0xFFu;
	if (bytes_per_pixel == 2) {
		uint32_t red = (color >> 16) & 0xFFu;
		uint32_t green = (color >> 8) & 0xFFu;
		uint32_t blue = color & 0xFFu;

		return (blue >> 3) | ((green << 3) & 0x07E0u) |
			((red << 8) & 0xF800u);
	}
	return color;
}

uint32_t fb_gfx3_image_expand_color(uint32_t bytes_per_pixel, uint32_t color)
{
	if (bytes_per_pixel != 2)
		return color;
	return ((color & 0x001Fu) << 3) | ((color >> 2) & 0x7u) |
		((color & 0x07E0u) << 5) | ((color >> 1) & 0x300u) |
		((color & 0xF800u) << 8) | ((color << 3) & 0x70000u);
}

/* ------------------------------------------------------------------------- */
/* CPU image primitives                                                      */
/* ------------------------------------------------------------------------- */

static void image_fix_relative(FB_GFX3_DRAW_STATE *state, uint32_t flags,
	float *x1, float *y1, float *x2, float *y2)
{
	if (state == NULL)
		return;
	switch (flags & FB_GFX3_COORDINATE_MASK) {
	case FB_GFX3_COORDINATE_R:
	case FB_GFX3_COORDINATE_RA:
		*x1 += state->last_x;
		*y1 += state->last_y;
		break;
	case FB_GFX3_COORDINATE_RR:
		*x1 += state->last_x;
		*y1 += state->last_y;
		if ((x2 == NULL) || (y2 == NULL))
			break;
		__attribute__((fallthrough));
	case FB_GFX3_COORDINATE_AR:
		if ((x2 != NULL) && (y2 != NULL)) {
			*x2 += *x1;
			*y2 += *y1;
		}
		break;
	default:
		break;
	}
	if ((x2 != NULL) && (y2 != NULL)) {
		state->last_x = *x2;
		state->last_y = *y2;
	} else {
		state->last_x = *x1;
		state->last_y = *y1;
	}
}

static uint32_t image_resolve_color(const FB_GFX3_IMAGE_VIEW *view,
	const FB_GFX3_DRAW_STATE *state, uint32_t color, uint32_t flags,
	int preset)
{
	if ((flags & FB_GFX3_DEFAULT_COLOR_1) && (state != NULL))
		return preset ? state->background_color : state->foreground_color;
	return fb_gfx3_image_fix_color(view->bytes_per_pixel, color);
}

int fb_gfx3_image_pset(FB_GFX3_IMAGE_VIEW *view,
	FB_GFX3_DRAW_STATE *state, float x, float y, uint32_t color,
	uint32_t flags, int preset)
{
	if ((view == NULL) || !isfinite(x) || !isfinite(y))
		return FB_GFX3_INVALID;
	image_fix_relative(state, flags, &x, &y, NULL, NULL);
	if (((double)x < INT_MIN) || ((double)x > INT_MAX) ||
	    ((double)y < INT_MIN) || ((double)y > INT_MAX))
		return FB_GFX3_INVALID;
	color = image_resolve_color(view, state, color, flags, preset);
	fb_gfx3_image_set_primitive_pixel(view, state, CINT(x), CINT(y), color);
	return FB_GFX3_OK;
}

uint32_t fb_gfx3_image_point(const FB_GFX3_IMAGE_VIEW *view,
	float x, float y)
{
	int ix;
	int iy;

	if ((view == NULL) || !isfinite(x) || !isfinite(y) ||
	    ((double)x < INT_MIN) || ((double)x > INT_MAX) ||
	    ((double)y < INT_MIN) || ((double)y > INT_MAX))
		return UINT32_MAX;
	ix = CINT(x);
	iy = CINT(y);
	if ((ix < 0) || (iy < 0) || ((uint32_t)ix >= view->width) ||
	    ((uint32_t)iy >= view->height))
		return UINT32_MAX;
	return fb_gfx3_image_expand_color(view->bytes_per_pixel,
		fb_gfx3_image_get_pixel_raw(view, ix, iy));
}

static void image_draw_line(FB_GFX3_IMAGE_VIEW *view,
	const FB_GFX3_DRAW_STATE *state, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style)
{
	int delta_x = abs(x2 - x1);
	int delta_y = -abs(y2 - y1);
	int step_x = (x1 < x2) ? 1 : -1;
	int step_y = (y1 < y2) ? 1 : -1;
	int error = delta_x + delta_y;
	uint32_t bit = 0x8000u;

	for (;;) {
		if (style & bit)
			fb_gfx3_image_set_primitive_pixel(view, state, x1, y1,
				color);
		if ((x1 == x2) && (y1 == y2))
			break;
		bit >>= 1;
		if (bit == 0)
			bit = 0x8000u;
		if ((error * 2) >= delta_y) {
			error += delta_y;
			x1 += step_x;
		}
		if ((error * 2) <= delta_x) {
			error += delta_x;
			y1 += step_y;
		}
	}
}

int fb_gfx3_image_line(FB_GFX3_IMAGE_VIEW *view,
	FB_GFX3_DRAW_STATE *state, float x1, float y1, float x2, float y2,
	uint32_t color, int type, uint32_t style, uint32_t flags)
{
	int ix1;
	int iy1;
	int ix2;
	int iy2;
	int x;
	int y;

	if ((view == NULL) || (type < FB_GFX3_LINE_TYPE_LINE) ||
	    (type > FB_GFX3_LINE_TYPE_FILLED_BOX) || !isfinite(x1) ||
	    !isfinite(y1) || !isfinite(x2) || !isfinite(y2))
		return FB_GFX3_INVALID;
	image_fix_relative(state, flags, &x1, &y1, &x2, &y2);
	if (((double)x1 < INT_MIN) || ((double)x1 > INT_MAX) ||
	    ((double)y1 < INT_MIN) || ((double)y1 > INT_MAX) ||
	    ((double)x2 < INT_MIN) || ((double)x2 > INT_MAX) ||
	    ((double)y2 < INT_MIN) || ((double)y2 > INT_MAX))
		return FB_GFX3_INVALID;
	ix1 = CINT(x1);
	iy1 = CINT(y1);
	ix2 = CINT(x2);
	iy2 = CINT(y2);
	color = image_resolve_color(view, state, color, flags, FALSE);
	style &= 0xFFFFu;
	if (type == FB_GFX3_LINE_TYPE_LINE) {
		image_draw_line(view, state, ix1, iy1, ix2, iy2, color, style);
		return FB_GFX3_OK;
	}
	if (ix2 < ix1) {
		x = ix1;
		ix1 = ix2;
		ix2 = x;
	}
	if (iy2 < iy1) {
		y = iy1;
		iy1 = iy2;
		iy2 = y;
	}
	if (type == FB_GFX3_LINE_TYPE_FILLED_BOX) {
		for (y = iy1; y <= iy2; y++)
			image_fill_span(view, state, y, ix1, ix2, color);
	} else {
		image_draw_line(view, state, ix1, iy1, ix2, iy1, color, style);
		image_draw_line(view, state, ix1, iy2, ix2, iy2, color, style);
		image_draw_line(view, state, ix1, iy1, ix1, iy2, color, style);
		image_draw_line(view, state, ix2, iy1, ix2, iy2, color, style);
	}
	return FB_GFX3_OK;
}

static void image_ellipse_scanline(FB_GFX3_IMAGE_VIEW *view,
	const FB_GFX3_DRAW_STATE *state, int y, int x1, int x2, uint32_t color,
	int filled)
{
	if (filled)
		image_fill_span(view, state, y, x1, x2, color);
	else {
		fb_gfx3_image_set_primitive_pixel(view, state, x1, y, color);
		fb_gfx3_image_set_primitive_pixel(view, state, x2, y, color);
	}
}

static void image_draw_full_ellipse(FB_GFX3_IMAGE_VIEW *view,
	const FB_GFX3_DRAW_STATE *state, int center_x, int center_y,
	float radius_x, float radius_y, uint32_t color, int filled)
{
	int d;
	int x1;
	int x2;
	int y1;
	int y2;
	int64_t aq;
	int64_t bq;
	int64_t dx;
	int64_t dy;
	int64_t r;
	int64_t rx;
	int64_t ry;

	d = CINT(radius_x);
	x1 = center_x - d;
	x2 = center_x + d;
	y1 = center_y;
	y2 = center_y;
	if (CINT(radius_y) == 0) {
		image_fill_span(view, state, center_y, x1, x2, color);
		return;
	}
	image_ellipse_scanline(view, state, center_y, x1, x2, color, filled);
	aq = (int64_t)(radius_x * radius_x);
	bq = (int64_t)(radius_y * radius_y);
	dx = aq << 1;
	dy = bq << 1;
	r = (int64_t)radius_x * bq;
	rx = r << 1;
	ry = 0;
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
		image_ellipse_scanline(view, state, y1, x1, x2, color, filled);
		image_ellipse_scanline(view, state, y2, x1, x2, color, filled);
	}
}

int fb_gfx3_image_ellipse(FB_GFX3_IMAGE_VIEW *view,
	FB_GFX3_DRAW_STATE *state, float x, float y, float radius,
	uint32_t color, float aspect, float start, float end, int filled,
	uint32_t flags)
{
	float radius_x;
	float radius_y;
	float increment;
	float angle;
	int center_x;
	int center_y;
	int steps;
	int i;

	if ((view == NULL) || !(radius > 0.0f) || !isfinite(radius) ||
	    !isfinite(aspect) || !isfinite(start) || !isfinite(end))
		return FB_GFX3_INVALID;
	image_fix_relative(state, flags, &x, &y, NULL, NULL);
	if (!isfinite(x) || !isfinite(y) || ((double)x < INT_MIN) ||
	    ((double)x > INT_MAX) || ((double)y < INT_MIN) ||
	    ((double)y > INT_MAX))
		return FB_GFX3_INVALID;
	center_x = CINT(x);
	center_y = CINT(y);
	color = image_resolve_color(view, state, color, flags, FALSE);
	if (aspect == 0.0f)
		aspect = 1.0f;
	if (!(aspect > 0.0f))
		return FB_GFX3_INVALID;
	if (aspect > 1.0f) {
		radius_x = radius / aspect;
		radius_y = radius;
	} else {
		radius_x = radius;
		radius_y = radius * aspect;
	}
	if ((start == 0.0f) && (end == 6.283186f)) {
		image_draw_full_ellipse(view, state, center_x, center_y, radius_x,
			radius_y, color, filled);
		return FB_GFX3_OK;
	}
	if (start < 0.0f) {
		start = -start;
		image_draw_line(view, state, center_x, center_y,
			center_x + CINT(cosf(start) * radius_x),
			center_y - CINT(sinf(start) * radius_y), color, 0xFFFFu);
	}
	if (end < 0.0f) {
		end = -end;
		image_draw_line(view, state, center_x, center_y,
			center_x + CINT(cosf(end) * radius_x),
			center_y - CINT(sinf(end) * radius_y), color, 0xFFFFu);
	}
	while (end < start)
		end += 6.28318530717958647692f;
	while ((end - start) > 6.28318530717958647692f)
		start += 6.28318530717958647692f;
	increment = 1.0f /
		(sqrtf(radius_x) * sqrtf(radius_y) * 1.5f);
	if (!isfinite(increment) || !(increment > 0.0f) ||
	    (((double)(end - start) / increment) > INT_MAX - 1.0))
		return FB_GFX3_INVALID;
	steps = (int)((end - start) / increment + 0.5f);
	for (i = 0; i <= steps; i++) {
		angle = start + ((float)i * increment);
		fb_gfx3_image_set_primitive_pixel(view, state,
			center_x + CINT(cosf(angle) * radius_x),
			center_y - CINT(sinf(angle) * radius_y), color);
	}
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* CPU PUT modes                                                             */
/* ------------------------------------------------------------------------- */

static uint32_t image_pixel_mask(uint32_t bytes_per_pixel)
{
	if (bytes_per_pixel == 1)
		return 0xFFu;
	if (bytes_per_pixel == 2)
		return 0xFFFFu;
	return UINT32_MAX;
}

static uint32_t image_apply_builtin(uint32_t bytes_per_pixel, int mode,
	uint32_t source, uint32_t destination, uint32_t blend_alpha,
	int *write_pixel)
{
	uint32_t alpha;
	uint32_t destination_rb;
	uint32_t destination_ga;
	uint32_t source_rb;
	uint32_t source_ga;
	uint32_t temporary1;
	uint32_t temporary2;
	uint32_t overflow;
	uint32_t mask = image_pixel_mask(bytes_per_pixel);

	*write_pixel = TRUE;
	switch (mode) {
	case FB_GFX3_BLIT_PSET:
		return source & mask;
	case FB_GFX3_BLIT_PRESET:
		return (~source) & mask;
	case FB_GFX3_BLIT_AND:
		return (source & destination) & mask;
	case FB_GFX3_BLIT_OR:
		return (source | destination) & mask;
	case FB_GFX3_BLIT_XOR:
		return (source ^ destination) & mask;
	case FB_GFX3_BLIT_TRANS:
		if (bytes_per_pixel == 1) {
			if ((source & mask) == 0)
				*write_pixel = FALSE;
			return source & mask;
		}
		if (bytes_per_pixel == 2) {
			if ((source & 0xFFFFu) == 0xF81Fu)
				*write_pixel = FALSE;
			return source & 0xFFFFu;
		}
		source &= 0x00FFFFFFu;
		if (source == 0x00FF00FFu)
			*write_pixel = FALSE;
		return source;
	case FB_GFX3_BLIT_ALPHA:
		if (bytes_per_pixel != 4)
			return source & mask;
		alpha = (source >> 24) + 1;
		source_rb = source & 0x00FF00FFu;
		source_ga = source & 0xFF00FF00u;
		destination_rb = destination & 0x00FF00FFu;
		destination_ga = destination & 0xFF00FF00u;
		source_rb = ((source_rb - destination_rb) * alpha) >> 8;
		source_ga = ((source_ga >> 8) - (destination_ga >> 8)) * alpha;
		return ((destination_rb + source_rb) & 0x00FF00FFu) |
			((destination_ga + source_ga) & 0xFF00FF00u);
	case FB_GFX3_BLIT_ADD:
		alpha = blend_alpha & 0xFFu;
		if (bytes_per_pixel == 1)
			return (source | destination) & mask;
		if (bytes_per_pixel == 2) {
			if ((source & 0xFFFFu) == 0xF81Fu) {
				*write_pixel = FALSE;
				return destination;
			}
			alpha = (alpha + 7) >> 3;
			source = ((source << 16) | source) & 0x07C0F81Fu;
			source = ((source * alpha) >> 5) & 0x07C0F81Fu;
			destination = ((destination << 16) | destination) &
				0x07C0F81Fu;
			source += destination;
			overflow = source & 0x08010020u;
			overflow -= overflow >> 5;
			source |= overflow;
			source &= 0x07C0F81Fu;
			source |= source >> 16;
			return source & 0xFFFFu;
		}
		if ((source & 0x00FFFFFFu) == 0x00FF00FFu) {
			*write_pixel = FALSE;
			return destination;
		}
		temporary1 = source & 0x00FF00FFu;
		temporary2 = (source >> 8) & 0x00FF00FFu;
		temporary1 = ((temporary1 * alpha) >> 8) & 0x00FF00FFu;
		temporary2 = (temporary2 * alpha) & 0xFF00FF00u;
		source = temporary1 | temporary2;
		temporary1 = source & 0x80808080u;
		temporary2 = destination & 0x80808080u;
		source = (source & 0x7F7F7F7Fu) +
			(destination & 0x7F7F7F7Fu);
		destination = temporary1;
		temporary1 |= temporary2;
		temporary2 = destination & temporary2;
		destination = temporary1 & source;
		source |= ((((temporary2 | destination) >> 7) + 0x7F7F7F7Fu) ^
			0x7F7F7F7Fu) | temporary1;
		return source;
	case FB_GFX3_BLIT_BLEND:
		alpha = blend_alpha & 0xFFu;
		if (alpha == 0) {
			*write_pixel = FALSE;
			return destination;
		}
		if (bytes_per_pixel == 1) {
			if ((source & mask) == 0)
				*write_pixel = FALSE;
			return source & mask;
		}
		if (bytes_per_pixel == 2) {
			if ((source & 0xFFFFu) == 0xF81Fu) {
				*write_pixel = FALSE;
				return destination;
			}
			alpha = (alpha + 7) >> 3;
			source_rb = source & 0xF81Fu;
			source_ga = source & 0x07E0u;
			destination_rb = destination & 0xF81Fu;
			destination_ga = destination & 0x07E0u;
			source_rb = ((source_rb - destination_rb) * alpha) >> 5;
			source_ga = ((source_ga - destination_ga) * alpha) >> 5;
			return ((destination_rb + source_rb) & 0xF81Fu) |
				((destination_ga + source_ga) & 0x07E0u);
		}
		if ((source & 0x00FFFFFFu) == 0x00FF00FFu) {
			*write_pixel = FALSE;
			return destination;
		}
		alpha++;
		source_rb = source & 0x00FF00FFu;
		source_ga = source & 0xFF00FF00u;
		destination_rb = destination & 0x00FF00FFu;
		destination_ga = destination & 0xFF00FF00u;
		source_rb = ((source_rb - destination_rb) * alpha) >> 8;
		source_ga = ((source_ga >> 8) - (destination_ga >> 8)) * alpha;
		return ((destination_rb + source_rb) & 0x00FF00FFu) |
			((destination_ga + source_ga) & 0xFF00FF00u);
	default:
		*write_pixel = FALSE;
		return destination;
	}
}

static uint32_t image_apply_custom(uint32_t bytes_per_pixel,
	uint32_t source, uint32_t destination, BLENDER *blender,
	void *parameter)
{
	uint32_t result;

	if (bytes_per_pixel == 2) {
		source = fb_gfx3_image_expand_color(2, source);
		destination = fb_gfx3_image_expand_color(2, destination);
	}
	result = blender(source, destination, parameter);
	if (bytes_per_pixel == 2)
		result = fb_gfx3_image_fix_color(2, result);
	return result & image_pixel_mask(bytes_per_pixel);
}

int fb_gfx3_image_put_pixels(unsigned char *source,
	unsigned char *destination, uint32_t width, uint32_t height,
	uint32_t source_pitch, uint32_t destination_pitch,
	uint32_t bytes_per_pixel, int mode, int alpha, BLENDER *blender,
	void *parameter)
{
	uint32_t x;
	uint32_t y;
	uint32_t source_color;
	uint32_t destination_color;
	uint32_t output;
	uint32_t row_bytes;
	int write_pixel;

	if ((source == NULL) || (destination == NULL) || (width == 0) ||
	    (height == 0) || !((bytes_per_pixel == 1) ||
	      (bytes_per_pixel == 2) || (bytes_per_pixel == 4)) ||
	    (image_multiply_u32(width, bytes_per_pixel, &row_bytes) !=
	     FB_GFX3_OK) || (source_pitch < row_bytes) ||
	    (destination_pitch < row_bytes) ||
	    (mode < FB_GFX3_BLIT_TRANS) || (mode > FB_GFX3_BLIT_BLEND) ||
	    ((mode == FB_GFX3_BLIT_CUSTOM) && (blender == NULL)))
		return FB_GFX3_INVALID;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			FB_GFX3_IMAGE_VIEW source_view = { 0 };
			FB_GFX3_IMAGE_VIEW destination_view = { 0 };

			source_view.pixels = source + ((size_t)y * source_pitch);
			source_view.width = width;
			source_view.height = 1;
			source_view.pitch = source_pitch;
			source_view.bytes_per_pixel = bytes_per_pixel;
			destination_view.pixels = destination +
				((size_t)y * destination_pitch);
			destination_view.width = width;
			destination_view.height = 1;
			destination_view.pitch = destination_pitch;
			destination_view.bytes_per_pixel = bytes_per_pixel;
			source_color = fb_gfx3_image_get_pixel_raw(&source_view,
				(int)x, 0);
			destination_color = fb_gfx3_image_get_pixel_raw(&destination_view,
				(int)x, 0);
			if (mode == FB_GFX3_BLIT_CUSTOM) {
				output = image_apply_custom(bytes_per_pixel, source_color,
					destination_color, blender, parameter);
				write_pixel = TRUE;
			} else {
				output = image_apply_builtin(bytes_per_pixel, mode,
					source_color, destination_color, (uint32_t)alpha,
					&write_pixel);
			}
			if (write_pixel)
				fb_gfx3_image_set_pixel_raw(&destination_view, (int)x, 0,
					output);
		}
	}
	return FB_GFX3_OK;
}

/* end of gfx3_image.c */
