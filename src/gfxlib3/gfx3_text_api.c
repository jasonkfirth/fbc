/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_text_api.c

    Purpose:

        Implement DRAW STRING compatibility for the built-in and user-supplied
        bitmap font formats.

    Responsibilities:

        - render the canonical 8 by 8 font as compact GPU glyph commands
        - draw the same glyphs directly into explicit CPU images
        - validate and assemble custom FreeBASIC font images
        - preserve PUT modes and custom blenders for user fonts

    This file intentionally does NOT contain:

        - graphical console state or PRINT behavior
        - scalable font shaping
        - platform text APIs
*/

#include "gfx3_api_internal.h"
#include "gfx3_backend_opengl.h"
#include "gfx3_backend_vulkan.h"
#include "gfx3_data.h"
#include "gfx3_gpu_surface.h"
#include "gfx3_image.h"

#include <math.h>

FBCALL int fb_GfxPut(void *target, float x, float y,
	unsigned char *source, int x1, int y1, int x2, int y2, int flags,
	int mode, PUTTER *putter, int alpha, BLENDER *blender,
	void *parameter);

/* ------------------------------------------------------------------------- */
/* Common coordinate and color helpers                                      */
/* ------------------------------------------------------------------------- */

static uint32_t text_target_bytes_per_pixel(
	const FB_GFX3_DRAW_STATE *state, const FB_GFX3_IMAGE_VIEW *target,
	const FB_GFX3_SURFACE *gpu_target)
{
	if (target != NULL)
		return target->bytes_per_pixel;
	if (gpu_target != NULL)
		return (gpu_target->depth + 7u) / 8u;
	return (state->mode->depth + 7u) / 8u;
}

static uint32_t text_resolve_color(const FB_GFX3_DRAW_STATE *state,
	const FB_GFX3_IMAGE_VIEW *target, const FB_GFX3_SURFACE *gpu_target,
	uint32_t color, uint32_t flags, int mode)
{
	if (mode == PUT_MODE_ALPHA)
		return color;
	if (flags & FB_GFX3_DEFAULT_COLOR_1)
		return state->foreground_color;
	return fb_gfx3_image_fix_color(
		text_target_bytes_per_pixel(state, target, gpu_target), color);
}

static int text_resolve_image_origin(FB_GFX3_DRAW_STATE *state,
	float *x, float *y, uint32_t flags, int *origin_x, int *origin_y)
{
	if ((state == NULL) || (x == NULL) || (y == NULL) ||
	    (origin_x == NULL) || (origin_y == NULL))
		return FB_GFX3_INVALID;
	if ((flags & FB_GFX3_COORDINATE_MASK) == FB_GFX3_COORDINATE_R) {
		*x += state->last_x;
		*y += state->last_y;
	}
	state->last_x = *x;
	state->last_y = *y;
	if (!isfinite(*x) || !isfinite(*y) ||
	    ((double)*x < INT_MIN) || ((double)*x > INT_MAX) ||
	    ((double)*y < INT_MIN) || ((double)*y > INT_MAX))
		return FB_GFX3_INVALID;
	*origin_x = CINT(*x);
	*origin_y = CINT(*y);
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Built-in font rendering                                                   */
/* ------------------------------------------------------------------------- */

static int text_draw_builtin_image(FB_GFX3_IMAGE_VIEW *target,
	FB_GFX3_DRAW_STATE *state, const unsigned char *font, float x, float y,
	uint32_t flags, const char *text, size_t length, uint32_t color)
{
	int origin_x;
	int origin_y;
	size_t character;
	int pixel_x;
	int pixel_y;
	int64_t destination_x;
	int64_t destination_y;
	int result;

	result = text_resolve_image_origin(state, &x, &y, flags,
		&origin_x, &origin_y);
	if (result != FB_GFX3_OK)
		return result;
	for (character = 0; character < length; ++character) {
		const unsigned char *glyph = font +
			((size_t)(unsigned char)text[character] * 8u);

		for (pixel_y = 0; pixel_y < 8; ++pixel_y) {
			for (pixel_x = 0; pixel_x < 8; ++pixel_x) {
				if ((glyph[pixel_y] & (1u << pixel_x)) == 0)
					continue;
				destination_x = (int64_t)origin_x +
					((int64_t)character * 8) + pixel_x;
				destination_y = (int64_t)origin_y + pixel_y;
				if ((destination_x >= INT_MIN) &&
				    (destination_x <= INT_MAX) &&
				    (destination_y >= INT_MIN) &&
				    (destination_y <= INT_MAX))
					fb_gfx3_image_set_primitive_pixel(target, state,
						(int)destination_x, (int)destination_y,
						color);
			}
		}
	}
	return FB_GFX3_OK;
}

static int text_build_builtin_glyphs(const unsigned char *font,
	int origin_x, int origin_y, const char *text, size_t length,
	uint32_t color, FB_GFX3_GLYPH *local_storage,
	size_t local_storage_count, FB_GFX3_GLYPH **glyphs_out,
	uint32_t *count_out)
{
	FB_GFX3_GLYPH *glyphs;
	size_t allocation_size;
	size_t character;

	if ((font == NULL) || (text == NULL) || (glyphs_out == NULL) ||
	    (count_out == NULL) || (length > UINT32_MAX) ||
	    (fb_gfx3_size_multiply(length, sizeof(glyphs[0]),
	     &allocation_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	*glyphs_out = NULL;
	*count_out = 0u;
	if (length == 0u)
		return FB_GFX3_OK;
	if ((local_storage != NULL) && (length <= local_storage_count)) {
		glyphs = local_storage;
		memset(glyphs, 0, allocation_size);
	} else {
		glyphs = (FB_GFX3_GLYPH *)calloc(1, allocation_size);
		if (glyphs == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
	}
	for (character = 0u; character < length; ++character) {
		const unsigned char *source = font +
			((size_t)(unsigned char)text[character] * 8u);
		int64_t destination_x = (int64_t)origin_x +
			((int64_t)character * 8);
		uint32_t row;

		if ((destination_x < INT32_MIN) || (destination_x > INT32_MAX)) {
			if (glyphs != local_storage)
				free(glyphs);
			return FB_GFX3_INVALID;
		}
		glyphs[character].x = (int32_t)destination_x;
		glyphs[character].y = origin_y;
		glyphs[character].foreground = color;
		glyphs[character].width = 8u;
		glyphs[character].height = 8u;
		for (row = 0u; row < 8u; ++row)
			glyphs[character].row[row] = source[row];
	}
	*glyphs_out = glyphs;
	*count_out = (uint32_t)length;
	return FB_GFX3_OK;
}

static int text_draw_builtin_gpu(FB_GFX3_SURFACE *target,
	FB_GFX3_DRAW_STATE *state, const unsigned char *font, float x, float y,
	uint32_t flags, const char *text, size_t length, uint32_t color)
{
	FB_GFX3_POINT *points;
	FB_GFX3_RECT clip;
	size_t maximum_points;
	size_t allocation_size;
	uint32_t point_count = 0;
	size_t character;
	int origin_x;
	int origin_y;
	int pixel_x;
	int pixel_y;
	int64_t destination_x;
	int64_t destination_y;
	uint32_t primitive_flags;
	int result;

	result = text_resolve_image_origin(state, &x, &y, flags,
		&origin_x, &origin_y);
	if (result != FB_GFX3_OK)
		return result;
	primitive_flags = fb_gfx3_compat_primitive_flags(state, target->depth,
		color);
	if ((primitive_flags == 0u) && (target->context != NULL) &&
	    ((target->context->renderer.backend_vtable ==
	      &__fb_gfx3_backend_opengl) ||
	     (target->context->renderer.backend_vtable ==
	      &__fb_gfx3_backend_vulkan))) {
		FB_GFX3_GLYPH *glyphs = NULL;
		FB_GFX3_GLYPH local_glyphs[32];
		uint32_t glyph_count = 0u;

		clip.x1 = 0;
		clip.y1 = 0;
		clip.x2 = (int32_t)target->width - 1;
		clip.y2 = (int32_t)target->height - 1;
		result = text_build_builtin_glyphs(font, origin_x, origin_y, text,
			length, color, local_glyphs,
			sizeof(local_glyphs) / sizeof(local_glyphs[0]), &glyphs,
			&glyph_count);
		if (result == FB_GFX3_OK)
			result = fb_gfx3_surface_glyphs(target, &clip, glyphs,
				glyph_count);
		if (glyphs != local_glyphs)
			free(glyphs);
		if (result != FB_GFX3_UNSUPPORTED)
			return result;
	}
	if ((fb_gfx3_size_multiply(length, 64u, &maximum_points) !=
	     FB_GFX3_OK) || (maximum_points > UINT32_MAX) ||
	    (fb_gfx3_size_multiply(maximum_points, sizeof(points[0]),
	     &allocation_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	if (maximum_points == 0)
		return FB_GFX3_OK;
	points = (FB_GFX3_POINT *)malloc(allocation_size);
	if (points == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	for (character = 0; character < length; ++character) {
		const unsigned char *glyph = font +
			((size_t)(unsigned char)text[character] * 8u);

		for (pixel_y = 0; pixel_y < 8; ++pixel_y) {
			for (pixel_x = 0; pixel_x < 8; ++pixel_x) {
				if ((glyph[pixel_y] & (1u << pixel_x)) == 0)
					continue;
				destination_x = (int64_t)origin_x +
					((int64_t)character * 8) + pixel_x;
				destination_y = (int64_t)origin_y + pixel_y;
				if ((destination_x < 0) || (destination_y < 0) ||
				    (destination_x >= target->width) ||
				    (destination_y >= target->height))
					continue;
				points[point_count].x = (int32_t)destination_x;
				points[point_count].y = (int32_t)destination_y;
				points[point_count].color = color;
				points[point_count].flags = primitive_flags;
				point_count++;
			}
		}
	}
	clip.x1 = 0;
	clip.y1 = 0;
	clip.x2 = (int32_t)target->width - 1;
	clip.y2 = (int32_t)target->height - 1;
	result = (point_count == 0) ? FB_GFX3_OK :
		fb_gfx3_surface_points(target, &clip, points, point_count);
	free(points);
	return result;
}

static int text_draw_builtin_screen(FB_GFX3_DRAW_STATE *state,
	const unsigned char *font, float x, float y, uint32_t flags,
	const char *text, size_t length, uint32_t color)
{
	FB_GFX3_POINT *points;
	size_t maximum_points;
	size_t allocation_size;
	uint32_t point_count = 0;
	size_t character;
	int origin_x;
	int origin_y;
	int pixel_x;
	int pixel_y;
	int64_t destination_x;
	int64_t destination_y;
	uint32_t primitive_flags;
	int result;

	if ((state == NULL) || (state->mode == NULL))
		return FB_GFX3_INVALID;
	result = fb_gfx3_compat_resolve_point(state, &x, &y, flags,
		&origin_x, &origin_y);
	if (result != FB_GFX3_OK)
		return result;
	primitive_flags = fb_gfx3_compat_primitive_flags(state,
		state->mode->depth, color);
	if ((primitive_flags == 0u) &&
	    ((state->mode->context.renderer.backend_vtable ==
	      &__fb_gfx3_backend_opengl) ||
	     (state->mode->context.renderer.backend_vtable ==
	      &__fb_gfx3_backend_vulkan))) {
		FB_GFX3_GLYPH *glyphs = NULL;
		FB_GFX3_GLYPH local_glyphs[32];
		uint32_t glyph_count = 0u;

		result = text_build_builtin_glyphs(font, origin_x, origin_y, text,
			length, color, local_glyphs,
			sizeof(local_glyphs) / sizeof(local_glyphs[0]), &glyphs,
			&glyph_count);
		if (result == FB_GFX3_OK)
			result = fb_gfx3_compat_glyphs_absolute(state, glyphs,
				glyph_count);
		if (glyphs != local_glyphs)
			free(glyphs);
		if (result != FB_GFX3_UNSUPPORTED)
			return result;
	}
	if ((fb_gfx3_size_multiply(length, 64u, &maximum_points) !=
	     FB_GFX3_OK) || (maximum_points > UINT32_MAX) ||
	    (fb_gfx3_size_multiply(maximum_points, sizeof(points[0]),
	     &allocation_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	if (maximum_points == 0)
		return FB_GFX3_OK;
	points = (FB_GFX3_POINT *)malloc(allocation_size);
	if (points == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	for (character = 0; character < length; ++character) {
		const unsigned char *glyph = font +
			((size_t)(unsigned char)text[character] * 8u);

		for (pixel_y = 0; pixel_y < 8; ++pixel_y) {
			for (pixel_x = 0; pixel_x < 8; ++pixel_x) {
				if ((glyph[pixel_y] & (1u << pixel_x)) == 0)
					continue;
				destination_x = (int64_t)origin_x +
					((int64_t)character * 8) + pixel_x;
				destination_y = (int64_t)origin_y + pixel_y;
				if ((destination_x < state->view.x1) ||
				    (destination_x > state->view.x2) ||
				    (destination_y < state->view.y1) ||
				    (destination_y > state->view.y2))
					continue;
				points[point_count].x = (int32_t)destination_x;
				points[point_count].y = (int32_t)destination_y;
				points[point_count].color = color;
				points[point_count].flags = primitive_flags;
				point_count++;
			}
		}
	}
	result = (point_count == 0) ? FB_GFX3_OK :
		fb_gfx3_compat_points_absolute(state, points, point_count);
	free(points);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Custom font assembly                                                      */
/* ------------------------------------------------------------------------- */

static uint32_t text_custom_font_transparent_color(
	uint32_t target_bytes_per_pixel)
{
	/*
	    DRAW STRING custom fonts are composed through the ordinary PUT path.
	    gfxlib2's TRANS key is depth-specific, so synthesized spacing must use
	    that key rather than calloc's zero bytes. A zero 32-bit pixel is an
	    ordinary opaque PUT pixel, not the historical magenta transparent key.
	*/
	if (target_bytes_per_pixel == 1u)
		return 0u;
	if (target_bytes_per_pixel == 2u)
		return 0xF81Fu;
	return 0x00FF00FFu;
}

typedef struct FB_GFX3_CUSTOM_FONT {
	const FB_GFX3_IMAGE_VIEW *image;
	uint32_t font_height;
	uint32_t first;
	uint32_t last;
	uint32_t glyph_offsets[256];
	uint32_t glyph_widths[256];
} FB_GFX3_CUSTOM_FONT;

static int text_parse_custom_font(const FB_GFX3_IMAGE_VIEW *font,
	uint32_t target_bytes_per_pixel, FB_GFX3_CUSTOM_FONT *custom_font)
{
	uint32_t first;
	uint32_t last;
	uint32_t glyph_count;
	uint32_t glyph_x = 0;
	uint32_t character_width;
	uint32_t code;

	if ((font == NULL) || (custom_font == NULL) ||
	    (font->height <= 1u) ||
	    (font->bytes_per_pixel != target_bytes_per_pixel) ||
	    (font->pitch < 4u) || (font->pixels[0] != 0))
		return FB_GFX3_INVALID;
	first = font->pixels[1];
	last = font->pixels[2];
	if (first > last) {
		uint32_t temporary = first;

		first = last;
		last = temporary;
	}
	glyph_count = last - first + 1u;
	if ((size_t)glyph_count + 3u > font->pitch)
		return FB_GFX3_INVALID;
	memset(custom_font, 0, sizeof(*custom_font));
	custom_font->image = font;
	custom_font->font_height = font->height - 1u;
	custom_font->first = first;
	custom_font->last = last;
	for (code = first; code <= last; ++code) {
		character_width = font->pixels[3u + code - first];
		if ((uint64_t)glyph_x + character_width > font->width)
			return FB_GFX3_INVALID;
		custom_font->glyph_offsets[code] = glyph_x;
		custom_font->glyph_widths[code] = character_width;
		glyph_x += character_width;
	}
	return FB_GFX3_OK;
}

static int text_create_custom_string_image(
	const FB_GFX3_CUSTOM_FONT *custom_font, const char *text, size_t length,
	unsigned char **image_result)
{
	const FB_GFX3_IMAGE_VIEW *font;
	uint32_t target_bytes_per_pixel;
	uint32_t output_width = 0;
	uint32_t output_pitch;
	uint32_t character_width;
	uint32_t code;
	uint32_t row;
	uint32_t transparent_color;
	size_t pixel_size;
	size_t allocation_size;
	size_t character;
	unsigned char *image;
	unsigned char *destination;
	const unsigned char *source;

	if ((custom_font == NULL) || (custom_font->image == NULL) ||
	    (text == NULL) || (image_result == NULL))
		return FB_GFX3_INVALID;
	*image_result = NULL;
	font = custom_font->image;
	target_bytes_per_pixel = font->bytes_per_pixel;
	if (!((target_bytes_per_pixel == 1u) ||
	      (target_bytes_per_pixel == 2u) ||
	      (target_bytes_per_pixel == 4u)) ||
	    (custom_font->font_height == 0u))
		return FB_GFX3_INVALID;
	for (character = 0; character < length; ++character) {
		code = (unsigned char)text[character];
		character_width = ((code >= custom_font->first) &&
			(code <= custom_font->last)) ? custom_font->glyph_widths[code] :
			custom_font->font_height;
		if (output_width > UINT32_MAX - character_width)
			return FB_GFX3_INVALID;
		output_width += character_width;
	}
	if (output_width == 0)
		return FB_GFX3_OK;
	if ((output_width > (UINT32_MAX - 15u) / target_bytes_per_pixel))
		return FB_GFX3_INVALID;
	output_pitch = (output_width * target_bytes_per_pixel + 15u) & ~15u;
	if ((fb_gfx3_size_multiply(output_pitch, custom_font->font_height,
	     &pixel_size) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_size_add(FB_GFX3_IMAGE_NEW_HEADER_SIZE, pixel_size,
	     &allocation_size) != FB_GFX3_OK) || (pixel_size == 0u) ||
	    (allocation_size <= FB_GFX3_IMAGE_NEW_HEADER_SIZE))
		return FB_GFX3_INVALID;
	image = (unsigned char *)calloc(1, allocation_size);
	if (image == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	if (fb_gfx3_image_initialize_header(image, TRUE, output_width,
	    custom_font->font_height, target_bytes_per_pixel, output_pitch) !=
	    FB_GFX3_OK) {
		free(image);
		return FB_GFX3_INVALID;
	}
	transparent_color = text_custom_font_transparent_color(
		target_bytes_per_pixel);
	for (row = 0; row < custom_font->font_height; ++row) {
		unsigned char *row_pixels = image + FB_GFX3_IMAGE_NEW_HEADER_SIZE +
			((size_t)row * output_pitch);
		uint32_t column;

		for (column = 0; column < output_width; ++column) {
			unsigned char *pixel = row_pixels +
				((size_t)column * target_bytes_per_pixel);

			switch (target_bytes_per_pixel) {
			case 1:
				pixel[0] = (unsigned char)transparent_color;
				break;
			case 2:
				memcpy(pixel, &transparent_color, 2u);
				break;
			default:
				memcpy(pixel, &transparent_color, 4u);
				break;
			}
		}
	}
	output_width = 0;
	for (character = 0; character < length; ++character) {
		code = (unsigned char)text[character];
		if ((code < custom_font->first) || (code > custom_font->last)) {
			output_width += custom_font->font_height;
			continue;
		}
		character_width = custom_font->glyph_widths[code];
		for (row = 0; row < custom_font->font_height; ++row) {
			source = font->pixels + ((size_t)(row + 1u) * font->pitch) +
				((size_t)custom_font->glyph_offsets[code] *
				 target_bytes_per_pixel);
			destination = image + FB_GFX3_IMAGE_NEW_HEADER_SIZE +
				((size_t)row * output_pitch) +
				((size_t)output_width * target_bytes_per_pixel);
			memcpy(destination, source,
				(size_t)character_width * target_bytes_per_pixel);
		}
		output_width += character_width;
	}
	*image_result = image;
	return FB_GFX3_OK;
}

static int text_custom_font_has_unsupported(
	const FB_GFX3_CUSTOM_FONT *custom_font, const char *text, size_t length)
{
	size_t character;

	if ((custom_font == NULL) || (text == NULL))
		return TRUE;
	for (character = 0; character < length; ++character) {
		uint32_t code = (unsigned char)text[character];

		if ((code < custom_font->first) || (code > custom_font->last))
			return TRUE;
	}
	return FALSE;
}

static int text_put_custom_image_cpu(FB_GFX3_IMAGE_VIEW *destination,
	const FB_GFX3_IMAGE_VIEW *source, int destination_x, int destination_y,
	int mode, int alpha, BLENDER *blender, void *parameter)
{
	int64_t left;
	int64_t top;
	int64_t right;
	int64_t bottom;
	uint32_t width;
	uint32_t height;
	unsigned char *source_pixels;
	unsigned char *destination_pixels;

	if ((destination == NULL) || (source == NULL) ||
	    (destination->bytes_per_pixel != source->bytes_per_pixel))
		return FB_GFX3_INVALID;
	left = (destination_x > 0) ? destination_x : 0;
	top = (destination_y > 0) ? destination_y : 0;
	right = (int64_t)destination_x + source->width;
	bottom = (int64_t)destination_y + source->height;
	if (right > destination->width)
		right = destination->width;
	if (bottom > destination->height)
		bottom = destination->height;
	if ((right <= left) || (bottom <= top))
		return FB_GFX3_OK;
	width = (uint32_t)(right - left);
	height = (uint32_t)(bottom - top);
	source_pixels = source->pixels +
		((size_t)(top - destination_y) * source->pitch) +
		((size_t)(left - destination_x) * source->bytes_per_pixel);
	destination_pixels = destination->pixels +
		((size_t)top * destination->pitch) +
		((size_t)left * destination->bytes_per_pixel);
	return fb_gfx3_image_put_pixels(source_pixels, destination_pixels, width,
		height, source->pitch, destination->pitch, source->bytes_per_pixel,
		mode, alpha, blender, parameter);
}

static int text_draw_custom_font_runs(FB_GFX3_DRAW_STATE *state,
	FB_GFX3_IMAGE_VIEW *target_image, FB_GFX3_SURFACE *gpu_target,
	const FB_GFX3_CUSTOM_FONT *custom_font, float x, float y, uint32_t flags,
	const char *text, size_t length, int mode, int alpha, BLENDER *blender,
	void *parameter)
{
	FB_GFX3_MODE *mode_state;
	FB_GFX3_RECT clip;
	FB_GFX3_SURFACE *surface_target = NULL;
	uint32_t horizontal_offset = 0;
	size_t character = 0;
	int origin_x;
	int origin_y;
	int result;

	if ((state == NULL) || (custom_font == NULL) || (text == NULL))
		return FB_GFX3_INVALID;
	if (target_image == NULL && gpu_target == NULL) {
		result = fb_gfx3_compat_resolve_point(state, &x, &y, flags,
			&origin_x, &origin_y);
		if (result != FB_GFX3_OK)
			return result;
		mode_state = state->mode;
		if ((mode_state == NULL) || (state->work_page >= mode_state->page_count))
			return FB_GFX3_INVALID;
		surface_target = &mode_state->pages[state->work_page];
		clip = state->view;
	} else {
		result = text_resolve_image_origin(state, &x, &y, flags,
			&origin_x, &origin_y);
		if (result != FB_GFX3_OK)
			return result;
		mode_state = state->mode;
		if (mode_state == NULL)
			return FB_GFX3_INVALID;
		if (gpu_target != NULL) {
			surface_target = gpu_target;
			clip.x1 = 0;
			clip.y1 = 0;
			clip.x2 = (int32_t)gpu_target->width - 1;
			clip.y2 = (int32_t)gpu_target->height - 1;
		}
	}
	if (mode_state->mutex == NULL)
		return FB_GFX3_INVALID;
	fb_MutexLock(mode_state->mutex);
	if (!mode_state->initialized ||
	    (state->generation != mode_state->generation)) {
		fb_MutexUnlock(mode_state->mutex);
		return FB_GFX3_INVALID;
	}
	while (character < length) {
		unsigned char *run_image = NULL;
		FB_GFX3_IMAGE_VIEW run_view;
		size_t run_start;
		size_t run_length;
		uint32_t code = (unsigned char)text[character];
		int64_t destination_x;

		if ((code < custom_font->first) || (code > custom_font->last)) {
			if (horizontal_offset > UINT32_MAX - custom_font->font_height) {
				result = FB_GFX3_INVALID;
				break;
			}
			horizontal_offset += custom_font->font_height;
			character++;
			continue;
		}
		run_start = character;
		while (character < length) {
			code = (unsigned char)text[character];
			if ((code < custom_font->first) || (code > custom_font->last))
				break;
			if (horizontal_offset > UINT32_MAX -
			    custom_font->glyph_widths[code]) {
				result = FB_GFX3_INVALID;
				break;
			}
			horizontal_offset += custom_font->glyph_widths[code];
			character++;
		}
		if (result != FB_GFX3_OK)
			break;
		run_length = character - run_start;
		result = text_create_custom_string_image(custom_font,
			text + run_start, run_length, &run_image);
		if (result != FB_GFX3_OK)
			break;
		/* A legal custom font may contain a zero-width glyph run. */
		if (run_image == NULL)
			continue;
		if (fb_gfx3_image_parse(run_image, &run_view) != FB_GFX3_OK) {
			free(run_image);
			result = FB_GFX3_INVALID;
			break;
		}
		/* horizontal_offset already includes this run, so subtract its width. */
		if (horizontal_offset < run_view.width) {
			free(run_image);
			result = FB_GFX3_INVALID;
			break;
		}
		destination_x = (int64_t)origin_x + horizontal_offset -
			run_view.width;
		if ((destination_x < INT_MIN) || (destination_x > INT_MAX)) {
			free(run_image);
			result = FB_GFX3_INVALID;
			break;
		}
		if (surface_target != NULL) {
			result = fb_gfx3_image_put_surface(surface_target, &clip,
				&run_view, 0, 0, run_view.width, run_view.height,
				(int)destination_x, origin_y, mode, alpha, blender, parameter);
		} else {
			result = text_put_custom_image_cpu(target_image, &run_view,
				(int)destination_x, origin_y, mode, alpha, blender, parameter);
		}
		free(run_image);
		if (result != FB_GFX3_OK)
			break;
	}
	fb_MutexUnlock(mode_state->mutex);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Public DRAW STRING ABI                                                    */
/* ------------------------------------------------------------------------- */

FBCALL int fb_GfxDrawString(void *target, float x, float y, int flags,
	FBSTRING *string, unsigned int color, void *font_image, int mode,
	PUTTER *putter, BLENDER *blender, void *parameter)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *gpu_target = NULL;
	FB_GFX3_IMAGE_VIEW target_image;
	FB_GFX3_IMAGE_VIEW font_view;
	FB_GFX3_IMAGE_VIEW *target_pointer = NULL;
	FB_GFX3_CUSTOM_FONT custom_font;
	const unsigned char *builtin_font;
	unsigned char *custom_image = NULL;
	ssize_t signed_length;
	size_t length;
	uint32_t resolved_color;
	uint32_t bytes_per_pixel;
	int put_flags;
	int result = FB_GFX3_OK;
	int runtime_result;

	if ((string == NULL) || (string->data == NULL))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	signed_length = FB_STRSIZE(string);
	if (signed_length < 0) {
		fb_hStrDelTemp(string);
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	length = (size_t)signed_length;
	builtin_font = (font_image == NULL) ? fb_gfx3_data_font_8x8() : NULL;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state == NULL) {
		result = FB_GFX3_INVALID;
		goto done_locked;
	}
	if (target != NULL) {
		if (fb_gfx3_gpu_surface_lookup_locked(target, state->mode,
		    &gpu_target) == FB_GFX3_OK) {
			target_pointer = NULL;
		} else if (fb_gfx3_image_parse(target, &target_image) !=
		    FB_GFX3_OK) {
			result = FB_GFX3_INVALID;
			goto done_locked;
		} else {
			target_pointer = &target_image;
		}
	}
	resolved_color = text_resolve_color(state, target_pointer, gpu_target,
		color, (uint32_t)flags, mode);
	if (font_image == NULL) {
		if (builtin_font == NULL) {
			result = FB_GFX3_FAILED;
		} else if (target == NULL) {
			result = text_draw_builtin_screen(state, builtin_font, x, y,
				(uint32_t)flags, string->data, length,
				resolved_color);
		} else if (gpu_target != NULL) {
			result = text_draw_builtin_gpu(gpu_target, state, builtin_font,
				x, y, (uint32_t)flags, string->data, length,
				resolved_color);
		} else {
			result = text_draw_builtin_image(target_pointer, state,
				builtin_font, x, y, (uint32_t)flags, string->data,
				length, resolved_color);
		}
		goto done_locked;
	}
	if (fb_gfx3_image_parse(font_image, &font_view) != FB_GFX3_OK) {
		result = FB_GFX3_INVALID;
		goto done_locked;
	}
	bytes_per_pixel = text_target_bytes_per_pixel(state, target_pointer,
		gpu_target);
	result = text_parse_custom_font(&font_view, bytes_per_pixel,
		&custom_font);
	if (result != FB_GFX3_OK)
		goto done_locked;
	if ((mode != PUT_MODE_TRANS) &&
	    text_custom_font_has_unsupported(&custom_font, string->data, length)) {
		result = text_draw_custom_font_runs(state, target_pointer, gpu_target,
			&custom_font, x, y, (uint32_t)flags, string->data, length, mode,
			(int)color, blender, parameter);
	} else {
		result = text_create_custom_string_image(&custom_font, string->data,
			length, &custom_image);
	}

done_locked:
	if ((result == FB_GFX3_OK) && (target_pointer != NULL))
		fb_gfx3_image_cache_metadata_touch(target_pointer);
	FB_GRAPHICS_UNLOCK();
	if ((result == FB_GFX3_OK) && (custom_image != NULL)) {
		put_flags = (((uint32_t)flags & FB_GFX3_COORDINATE_MASK) ==
			FB_GFX3_COORDINATE_R) ? FB_GFX3_COORDINATE_RA :
			FB_GFX3_COORDINATE_AA;
		runtime_result = fb_GfxPut(target, x, y, custom_image,
			(int)0xFFFF0000u, 0, 0, 0, put_flags, mode, putter,
			(int)color, blender, parameter);
		if (runtime_result != FB_RTERROR_OK)
			result = FB_GFX3_FAILED;
	}
	free(custom_image);
	fb_hStrDelTemp(string);
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

/* end of gfx3_text_api.c */
