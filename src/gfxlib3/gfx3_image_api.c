/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_image_api.c

    Purpose:

        Export FreeBASIC's CPU FB.IMAGE lifecycle and transfer ABI while
        retaining GPU pages as gfxlib3's normal screen representation.

    Responsibilities:

        - create, destroy, and describe current and QB image buffers
        - implement GET downloads and PUT uploads/blits
        - preserve a CPU barrier for custom BASIC PUT callbacks
        - expose built-in PUT callback symbols expected by generated programs
        - convert common external row formats

    This file intentionally does NOT contain:

        - primitive rasterization inside CPU images
        - image file codecs
        - GPU-only public surface extensions
*/

#include "gfx3_api_internal.h"
#include "gfx3_gpu_surface.h"
#include "gfx3_image.h"
#include "gfx3_target.h"

#include <math.h>

static void image_api_cache_remove(FB_GFX3_CONTEXT *context,
	const void *image_header);

/* ------------------------------------------------------------------------- */
/* Checked allocation and header helpers                                     */
/* ------------------------------------------------------------------------- */

static int image_api_u32_multiply(uint32_t left, uint32_t right,
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

static uint32_t image_api_bytes_per_pixel(uint32_t depth)
{
	return (depth + 7u) / 8u;
}

/*
	IMAGECREATE is a CPU compatibility operation.  Fill one row at the native
	pixel width, rather than deciding how to store every individual pixel.  The
	image header and its row pitch guarantee the 2- and 4-byte destinations are
	properly aligned; the old QB header still starts its pixel data at offset 4.
*/
static void image_api_fill(FB_GFX3_IMAGE_VIEW *view, uint32_t row_bytes,
	uint32_t color)
{
	uint32_t x;
	uint32_t y;

	if ((view == NULL) || (view->pixels == NULL))
		return;
	for (y = 0; y < view->height; y++) {
		unsigned char *row = view->pixels + ((size_t)y * view->pitch);

		switch (view->bytes_per_pixel) {
		case 1:
			memset(row, (unsigned char)color, view->width);
			break;
		case 2:
			for (x = 0; x < view->width; x++)
				((uint16_t *)row)[x] = (uint16_t)color;
			break;
		case 4:
			for (x = 0; x < view->width; x++)
				((uint32_t *)row)[x] = color;
			break;
		default:
			return;
		}
		if (view->pitch > row_bytes)
			memset(row + row_bytes, 0, view->pitch - row_bytes);
	}
}

/*
	Row conversion writes one pixel at a time and can receive an unaligned
	external buffer, unlike IMAGECREATE's owned storage above.  Keep this small
	copy helper separate from the allocation fast path.
*/
static void image_api_store_pixel(unsigned char *destination,
	uint32_t bytes_per_pixel, uint32_t color)
{
	if (bytes_per_pixel == 1u) {
		destination[0] = (unsigned char)color;
	} else if (bytes_per_pixel == 2u) {
		uint16_t value = (uint16_t)color;

		memcpy(destination, &value, sizeof(value));
	} else {
		memcpy(destination, &color, sizeof(color));
	}
}

/*
	CPU FB.IMAGE storage is permitted to be byte-aligned rather than naturally
	aligned. Read its native packed value through memcpy so the cache metadata
	does not introduce an unaligned 16- or 32-bit access on strict targets.
*/
static uint32_t image_api_load_pixel(const unsigned char *source,
	uint32_t bytes_per_pixel)
{
	uint32_t color = 0;

	if (source == NULL)
		return 0;
	if (bytes_per_pixel == 1u)
		return source[0];
	if (bytes_per_pixel == 2u) {
		uint16_t value;

		memcpy(&value, source, sizeof(value));
		return value;
	}
	if (bytes_per_pixel == 4u)
		memcpy(&color, source, sizeof(color));
	return color;
}

static void *image_api_create(int width, int height, uint32_t color,
	int depth, int flags, int use_new_header)
{
	FB_GFX3_DRAW_STATE *state = NULL;
	FB_GFX3_IMAGE_VIEW view;
	unsigned char *allocation;
	unsigned char *image;
	uintptr_t aligned;
	uint32_t bytes_per_pixel;
	uint32_t row_bytes;
	uint32_t pitch;
	uint32_t header_size;
	size_t pixel_bytes;
	size_t allocation_size;

	if ((width <= 0) || (height <= 0)) {
		fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
		return NULL;
	}
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state == NULL) {
		FB_GRAPHICS_UNLOCK();
		fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
		return NULL;
	}
	if (depth > 0)
		bytes_per_pixel = image_api_bytes_per_pixel((uint32_t)depth);
	else
		bytes_per_pixel = image_api_bytes_per_pixel(state->mode->depth);
	if (!((bytes_per_pixel == 1) || (bytes_per_pixel == 2) ||
	      (bytes_per_pixel == 4)) ||
	    (image_api_u32_multiply((uint32_t)width, bytes_per_pixel,
	     &row_bytes) != FB_GFX3_OK)) {
		FB_GRAPHICS_UNLOCK();
		fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
		return NULL;
	}
	if (use_new_header) {
		if (row_bytes > UINT32_MAX - 15u) {
			FB_GRAPHICS_UNLOCK();
			fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
			return NULL;
		}
		pitch = (row_bytes + 15u) & ~15u;
		header_size = FB_GFX3_IMAGE_NEW_HEADER_SIZE;
	} else {
		if ((width > 0x1FFF) || (height > 0xFFFF)) {
			FB_GRAPHICS_UNLOCK();
			fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
			return NULL;
		}
		pitch = row_bytes;
		header_size = 4;
	}
	if ((fb_gfx3_size_multiply(pitch, (uint32_t)height, &pixel_bytes) !=
	     FB_GFX3_OK) ||
	    (pixel_bytes > SIZE_MAX - header_size - sizeof(void *) - 15u)) {
		FB_GRAPHICS_UNLOCK();
		fb_ErrorSetNum(FB_RTERROR_OUTOFMEM);
		return NULL;
	}
	allocation_size = pixel_bytes + header_size + sizeof(void *) + 15u;
	allocation = (unsigned char *)malloc(allocation_size);
	if (allocation == NULL) {
		FB_GRAPHICS_UNLOCK();
		fb_ErrorSetNum(FB_RTERROR_OUTOFMEM);
		return NULL;
	}
	aligned = ((uintptr_t)allocation + sizeof(void *) + 15u) &
		~(uintptr_t)15u;
	image = (unsigned char *)aligned;
	memcpy(image - sizeof(void *), (const void *)&allocation,
		sizeof(allocation));
	if (fb_gfx3_image_initialize_header(image, use_new_header,
	    (uint32_t)width, (uint32_t)height, bytes_per_pixel, pitch) !=
	    FB_GFX3_OK) {
		free(allocation);
		FB_GRAPHICS_UNLOCK();
		fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
		return NULL;
	}
	fb_gfx3_image_parse(image, &view);
	fb_gfx3_image_cache_metadata_initialize(&view);
	if ((uint32_t)flags & FB_GFX3_DEFAULT_COLOR_1) {
		if (bytes_per_pixel == 1)
			color = 0;
		else if (bytes_per_pixel == 2)
			color = 0xF81Fu;
		else
			color = 0xFFFF00FFu;
	} else {
		color = fb_gfx3_image_fix_color(bytes_per_pixel, color);
	}
	image_api_fill(&view, row_bytes, color);
	FB_GRAPHICS_UNLOCK();
	fb_ErrorSetNum(FB_RTERROR_OK);
	/*
		The public image pointer is an aligned interior pointer.  Its allocation
		base is stored immediately before the image and recovered by
		fb_GfxImageDestroy(), so ownership transfers to the caller here.
	*/
	/* cppcheck-suppress memleak */
	return image;
}

FBCALL void *fb_GfxImageCreate(int width, int height, unsigned int color,
	int depth, int flags)
{
	return image_api_create(width, height, color, depth, flags, TRUE);
}

FBCALL void *fb_GfxImageCreateQB(int width, int height, unsigned int color,
	int depth, int flags)
{
	return image_api_create(width, height, color, depth, flags, FALSE);
}

FBCALL void fb_GfxImageDestroy(void *image)
{
	FB_GFX3_DRAW_STATE *state;
	void *allocation;

	if (image == NULL)
		return;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state != NULL)
		image_api_cache_remove(&state->mode->context, image);
	FB_GRAPHICS_UNLOCK();
	memcpy((void *)&allocation, (unsigned char *)image - sizeof(void *),
		sizeof(allocation));
	free(allocation);
}

/* ------------------------------------------------------------------------- */
/* IMAGEINFO                                                                 */
/* ------------------------------------------------------------------------- */

FBCALL int fb_GfxImageInfo(void *image, ssize_t *width, ssize_t *height,
	ssize_t *bytes_per_pixel, ssize_t *pitch, void **pixels, ssize_t *size)
{
	FB_GFX3_IMAGE_VIEW view;
	size_t pixel_bytes;

	if ((width == NULL) || (height == NULL) || (bytes_per_pixel == NULL) ||
	    (pitch == NULL) || (pixels == NULL) || (size == NULL) ||
	    (fb_gfx3_image_parse(image, &view) != FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(view.pitch, view.height, &pixel_bytes) !=
	     FB_GFX3_OK) || (pixel_bytes > (size_t)SSIZE_MAX - view.header_size)) {
		if (width != NULL)
			*width = -1;
		if (height != NULL)
			*height = -1;
		if (bytes_per_pixel != NULL)
			*bytes_per_pixel = -1;
		if (pitch != NULL)
			*pitch = -1;
		if (pixels != NULL)
			*pixels = NULL;
		if (size != NULL)
			*size = -1;
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	*width = (ssize_t)view.width;
	*height = (ssize_t)view.height;
	*bytes_per_pixel = (ssize_t)view.bytes_per_pixel;
	*pitch = (ssize_t)view.pitch;
	*pixels = view.pixels;
	*size = (ssize_t)(pixel_bytes + view.header_size);
	/* IMAGEINFO returns a writable pixel pointer under the public ABI. */
	fb_gfx3_image_cache_metadata_mark_external(&view);
	return fb_ErrorSetNum(FB_RTERROR_OK);
}

FBCALL int fb_GfxImageInfo32(void *image, int *width, int *height,
	int *bytes_per_pixel, int *pitch, void **pixels, int *size)
{
	ssize_t native_width;
	ssize_t native_height;
	ssize_t native_bpp;
	ssize_t native_pitch;
	ssize_t native_size;
	int result;

	if ((width == NULL) || (height == NULL) || (bytes_per_pixel == NULL) ||
	    (pitch == NULL) || (pixels == NULL) || (size == NULL))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	result = fb_GfxImageInfo(image, &native_width, &native_height,
		&native_bpp, &native_pitch, pixels, &native_size);
	*width = (int)native_width;
	*height = (int)native_height;
	*bytes_per_pixel = (int)native_bpp;
	*pitch = (int)native_pitch;
	*size = (int)native_size;
	return result;
}

FBCALL int fb_GfxImageInfo64(void *image, long long *width,
	long long *height, long long *bytes_per_pixel, long long *pitch,
	void **pixels, long long *size)
{
	ssize_t native_width;
	ssize_t native_height;
	ssize_t native_bpp;
	ssize_t native_pitch;
	ssize_t native_size;
	int result;

	if ((width == NULL) || (height == NULL) || (bytes_per_pixel == NULL) ||
	    (pitch == NULL) || (pixels == NULL) || (size == NULL))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	result = fb_GfxImageInfo(image, &native_width, &native_height,
		&native_bpp, &native_pitch, pixels, &native_size);
	*width = (long long)native_width;
	*height = (long long)native_height;
	*bytes_per_pixel = (long long)native_bpp;
	*pitch = (long long)native_pitch;
	*size = (long long)native_size;
	return result;
}

/* ------------------------------------------------------------------------- */
/* Coordinate translation used by GET and PUT                               */
/* ------------------------------------------------------------------------- */

static void image_api_fix_relative(FB_GFX3_DRAW_STATE *state, int flags,
	float *x1, float *y1, float *x2, float *y2)
{
	switch ((uint32_t)flags & FB_GFX3_COORDINATE_MASK) {
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

static int image_api_translate_screen(const FB_GFX3_DRAW_STATE *state,
	float x, float y, int *translated_x, int *translated_y)
{
	int view_width = state->view.x2 - state->view.x1 + 1;
	int view_height = state->view.y2 - state->view.y1 + 1;

	if (state->flags & FB_GFX3_WINDOW_ACTIVE) {
		if ((state->window_width == 0.0f) ||
		    (state->window_height == 0.0f))
			return FB_GFX3_INVALID;
		x = ((x - state->window_x) * (float)(view_width - 1)) /
			state->window_width;
		y = ((y - state->window_y) * (float)(view_height - 1)) /
			state->window_height;
	}
	if (!isfinite(x) || !isfinite(y))
		return FB_GFX3_INVALID;
	/*
		Legacy engines sometimes retain offscreen sprite coordinates in
		integer sentinel ranges.  CINT cannot safely convert the exact
		integer limits, but the later clip step only needs a value that is
		unambiguously outside the drawable range.  Saturating to half-range
		keeps its subtract-and-clip arithmetic within signed-int bounds.
	*/
	if ((double)x < INT_MIN + 1.0)
		*translated_x = INT_MIN / 2;
	else if ((double)x > INT_MAX - 1.0)
		*translated_x = INT_MAX / 2;
	else
		*translated_x = CINT(x);
	if ((double)y < INT_MIN + 1.0)
		*translated_y = INT_MIN / 2;
	else if ((double)y > INT_MAX - 1.0)
		*translated_y = INT_MAX / 2;
	else
		*translated_y = CINT(y);
	if ((state->flags & (FB_GFX3_WINDOW_ACTIVE | FB_GFX3_WINDOW_SCREEN)) ==
	    FB_GFX3_WINDOW_ACTIVE)
		*translated_y = view_height - 1 - *translated_y;
	if ((state->flags & FB_GFX3_VIEW_SCREEN) == 0) {
		*translated_x += state->view.x1;
		*translated_y += state->view.y1;
	}
	return FB_GFX3_OK;
}

static void image_api_order_coordinates(int *x1, int *y1, int *x2, int *y2)
{
	int temporary;

	if (*x2 < *x1) {
		temporary = *x1;
		*x1 = *x2;
		*x2 = temporary;
	}
	if (*y2 < *y1) {
		temporary = *y1;
		*y1 = *y2;
		*y2 = temporary;
	}
}

/* ------------------------------------------------------------------------- */
/* GET                                                                       */
/* ------------------------------------------------------------------------- */

/*
	GET must return a CPU FB.IMAGE before the call completes, so the first read
	of a GPU-owned screen page necessarily crosses the device boundary.  A run
	of GET calls, however, often reads many small sprite rectangles without an
	intervening screen write.  Waiting for one transfer per rectangle turns that
	otherwise GPU-rendered frame into hundreds of driver fences.

	Reuse the compatibility page shadow for this read-only snapshot. Every
	public screen write invalidates shadow_valid, and SCREENLOCK owns the same
	buffer when it is writable. Therefore a valid shadow is precisely the
	current work page, while a stale shadow is refreshed once as a complete
	page. RGB565 and BGRA pages both have a directly representable FB.IMAGE
	layout. Indexed modes still require palette-aware conversion. The caller
	holds the runtime graphics lock.
*/
static int image_api_ensure_work_shadow_locked(FB_GFX3_DRAW_STATE *state,
	FB_GFX3_MODE *mode)
{
	uint32_t page;
	uint32_t bytes_per_pixel;
	uint64_t row_bytes;
	size_t allocation_size;
	int result;

	if ((state == NULL) || (mode == NULL) ||
	    ((mode->depth != 16u) && (mode->depth != 32u)) ||
	    (mode->width == 0u) || (mode->height == 0u) ||
	    (mode->shadow_pages == NULL) || (mode->shadow_valid == NULL))
		return FB_GFX3_UNSUPPORTED;
	page = state->work_page;
	bytes_per_pixel = image_api_bytes_per_pixel(mode->depth);
	if (bytes_per_pixel == 0u)
		return FB_GFX3_UNSUPPORTED;
	row_bytes = (uint64_t)mode->width * bytes_per_pixel;
	if ((page >= mode->page_count) || (row_bytes > UINT32_MAX) ||
	    (fb_gfx3_size_multiply((size_t)row_bytes, mode->height,
	     &allocation_size) != FB_GFX3_OK) || (allocation_size == 0u))
		return FB_GFX3_INVALID;
	mode->shadow_pitch = (uint32_t)row_bytes;
	if (mode->shadow_pages[page] == NULL) {
		mode->shadow_pages[page] = (unsigned char *)malloc(allocation_size);
		if (mode->shadow_pages[page] == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		mode->shadow_valid[page] = FALSE;
	}
	if (mode->shadow_valid[page])
		return FB_GFX3_OK;
	result = fb_gfx3_surface_download(&mode->pages[page], 0, 0,
		mode->width, mode->height, mode->shadow_pitch,
		mode->shadow_pages[page]);
	if (result == FB_GFX3_OK)
		mode->shadow_valid[page] = TRUE;
	return result;
}

static int image_api_get(void *target, float x1, float y1, float x2,
	float y2, unsigned char *destination, int flags, FBARRAY *array,
	int use_new_header)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *gpu_source = NULL;
	FB_GFX3_IMAGE_VIEW source_view;
	FB_GFX3_MODE *mode = NULL;
	unsigned char *pixels;
	uint32_t bytes_per_pixel;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t row_bytes;
	uint32_t pitch;
	uint32_t header_size;
	size_t required_size;
	const unsigned char *shadow_row;
	int ix1 = 0;
	int iy1 = 0;
	int ix2 = 0;
	int iy2 = 0;
	uint32_t row;
	int result = FB_GFX3_OK;

	if (destination == NULL)
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	memset(&source_view, 0, sizeof(source_view));
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state == NULL) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	result = fb_gfx3_compat_flush_points_graphics_locked(state);
	if (result != FB_GFX3_OK)
		goto done;
	mode = state->mode;
	/*
		GET enters through the public graphics ABI while FB_GRAPHICS_LOCK is
		held.  That lock serializes mode replacement, page selection, and other
		public drawing calls, so taking mode->mutex as well only added a second
		uncontended mutex pair around an otherwise synchronous readback.
	*/
	image_api_fix_relative(state, flags, &x1, &y1, &x2, &y2);
	if (target == NULL) {
		result = image_api_translate_screen(state, x1, y1, &ix1, &iy1);
		if (result == FB_GFX3_OK)
			result = image_api_translate_screen(state, x2, y2, &ix2,
				&iy2);
		bytes_per_pixel = image_api_bytes_per_pixel(mode->depth);
	} else {
		result = fb_gfx3_gpu_surface_lookup_locked(target, mode,
			&gpu_source);
		if ((result != FB_GFX3_OK) || (gpu_source == NULL)) {
			gpu_source = NULL;
			if (fb_gfx3_image_parse(target, &source_view) !=
			    FB_GFX3_OK)
				goto done;
		}
		if (!isfinite(x1) || !isfinite(y1) || !isfinite(x2) ||
		    !isfinite(y2) || ((double)x1 < INT_MIN) ||
		    ((double)x1 > INT_MAX) || ((double)y1 < INT_MIN) ||
		    ((double)y1 > INT_MAX) || ((double)x2 < INT_MIN) ||
		    ((double)x2 > INT_MAX) || ((double)y2 < INT_MIN) ||
		    ((double)y2 > INT_MAX)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		ix1 = CINT(x1);
		iy1 = CINT(y1);
		ix2 = CINT(x2);
		iy2 = CINT(y2);
		bytes_per_pixel = (gpu_source != NULL) ?
			image_api_bytes_per_pixel(gpu_source->depth) :
			source_view.bytes_per_pixel;
		result = FB_GFX3_OK;
	}
	if (result != FB_GFX3_OK)
		goto done;
	image_api_order_coordinates(&ix1, &iy1, &ix2, &iy2);
	if (target == NULL) {
		if ((ix1 < state->view.x1) || (iy1 < state->view.y1) ||
		    (ix2 > state->view.x2) || (iy2 > state->view.y2)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
	} else if ((ix1 < 0) || (iy1 < 0) ||
	    ((uint32_t)ix2 >= ((gpu_source != NULL) ? gpu_source->width :
	    source_view.width)) ||
	    ((uint32_t)iy2 >= ((gpu_source != NULL) ? gpu_source->height :
	    source_view.height))) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	width = (uint32_t)(ix2 - ix1) + 1u;
	height = (uint32_t)(iy2 - iy1) + 1u;
	if (image_api_u32_multiply(width, bytes_per_pixel, &row_bytes) !=
	    FB_GFX3_OK) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	if (use_new_header) {
		if (row_bytes > UINT32_MAX - 15u) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		pitch = (row_bytes + 15u) & ~15u;
		header_size = FB_GFX3_IMAGE_NEW_HEADER_SIZE;
	} else {
		pitch = row_bytes;
		header_size = 4;
	}
	if ((fb_gfx3_size_multiply(pitch, height, &required_size) !=
	     FB_GFX3_OK) || (required_size > SIZE_MAX - header_size)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	required_size += header_size;
	if ((array != NULL) && (array->size > 0) &&
	    (required_size > (size_t)array->size)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	result = fb_gfx3_image_initialize_header(destination, use_new_header,
		width, height, bytes_per_pixel, pitch);
	if (result != FB_GFX3_OK)
		goto done;
	pixels = destination + header_size;
	if (target == NULL) {
		result = image_api_ensure_work_shadow_locked(state, mode);
		if (result == FB_GFX3_OK) {
			for (row = 0; row < height; row++) {
				shadow_row = mode->shadow_pages[state->work_page] +
					((size_t)(iy1 + (int)row) * mode->shadow_pitch) +
					((size_t)ix1 * bytes_per_pixel);
				memcpy(pixels + ((size_t)row * pitch), shadow_row, row_bytes);
				if (pitch > row_bytes)
					memset(pixels + ((size_t)row * pitch) + row_bytes, 0,
						pitch - row_bytes);
			}
		}
	} else if (gpu_source != NULL) {
		result = fb_gfx3_surface_download(gpu_source, ix1, iy1, width,
			height, pitch, pixels);
	} else {
		for (row = 0; row < height; row++) {
			memcpy(pixels + ((size_t)row * pitch), source_view.pixels +
				((size_t)(iy1 + (int)row) * source_view.pitch) +
				((size_t)ix1 * bytes_per_pixel), row_bytes);
			if (pitch > row_bytes)
				memset(pixels + ((size_t)row * pitch) + row_bytes, 0,
					pitch - row_bytes);
		}
	}

done:
	if (result == FB_GFX3_OK) {
		FB_GFX3_IMAGE_VIEW destination_view;

		if (fb_gfx3_image_parse(destination, &destination_view) == FB_GFX3_OK) {
			uint32_t first_pixel = image_api_load_pixel(
				destination_view.pixels,
				destination_view.bytes_per_pixel);

			fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_TRACE,
				"GET completed: page=%u size=%ux%u first-pixel=0x%08X",
				state->work_page, width, height, first_pixel);
			fb_gfx3_image_cache_metadata_touch(&destination_view);
		}
	} else if ((state != NULL) && (mode != NULL)) {
		fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_WARNING,
			"GET failed: result=%d target=%s source=(%d,%d)-(%d,%d) "
			"view=(%d,%d)-(%d,%d)",
			result, (target == NULL) ? "screen" : "image",
			ix1, iy1, ix2, iy2, state->view.x1, state->view.y1,
			state->view.x2, state->view.y2);
	}
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

FBCALL int fb_GfxGet(void *target, float x1, float y1, float x2, float y2,
	unsigned char *destination, int flags, FBARRAY *array)
{
	return image_api_get(target, x1, y1, x2, y2, destination, flags,
		array, TRUE);
}

FBCALL int fb_GfxGetQB(void *target, float x1, float y1, float x2, float y2,
	unsigned char *destination, int flags, FBARRAY *array)
{
	return image_api_get(target, x1, y1, x2, y2, destination, flags,
		array, FALSE);
}

/* ------------------------------------------------------------------------- */
/* PUT                                                                       */
/* ------------------------------------------------------------------------- */

static int image_api_clip_put(int *destination_x, int *destination_y,
	int *source_x, int *source_y, uint32_t *width, uint32_t *height,
	const FB_GFX3_RECT *clip)
{
	int64_t destination_x2;
	int64_t destination_y2;

	destination_x2 = (int64_t)*destination_x + *width - 1;
	destination_y2 = (int64_t)*destination_y + *height - 1;
	if ((destination_x2 < clip->x1) || (destination_y2 < clip->y1) ||
	    (*destination_x > clip->x2) || (*destination_y > clip->y2)) {
		*width = 0;
		*height = 0;
		return FB_GFX3_OK;
	}
	if (*destination_x < clip->x1) {
		int difference = clip->x1 - *destination_x;

		*source_x += difference;
		*width -= (uint32_t)difference;
		*destination_x = clip->x1;
	}
	if (*destination_y < clip->y1) {
		int difference = clip->y1 - *destination_y;

		*source_y += difference;
		*height -= (uint32_t)difference;
		*destination_y = clip->y1;
	}
	if (destination_x2 > clip->x2)
		*width -= (uint32_t)(destination_x2 - clip->x2);
	if (destination_y2 > clip->y2)
		*height -= (uint32_t)(destination_y2 - clip->y2);
	return FB_GFX3_OK;
}

/*
	A CPU FB.IMAGE remains available after gfxlib3 uploads its GPU cache entry.
	If a legacy POINT loop has already created a valid page shadow, apply the
	same built-in PUT operation to that existing memory. The actual screen draw
	still executes as a GPU blit; this small coherence copy prevents a later
	POINT from downloading the complete render target.

	CUSTOM is excluded because invoking an application callback twice would
	change observable behaviour. GPU-only source surfaces likewise have no CPU
	pixels to mirror and retain the normal invalidation path.
*/
static int image_api_mirror_screen_put_locked(FB_GFX3_DRAW_STATE *state,
	const FB_GFX3_IMAGE_VIEW *source_view, int source_x, int source_y,
	uint32_t width, uint32_t height, int destination_x, int destination_y,
	int put_mode, int alpha)
{
	FB_GFX3_MODE *mode;
	FB_GFX3_RECT clip;
	unsigned char *source_pixels;
	unsigned char *destination_pixels;
	uint32_t row;
	uint32_t page;
	uint32_t destination_bpp;
	size_t row_bytes;
	int result;

	if ((state == NULL) || (source_view == NULL) ||
	    (put_mode == FB_GFX3_BLIT_CUSTOM))
		return FB_GFX3_UNSUPPORTED;
	mode = state->mode;
	page = state->work_page;
	destination_bpp = image_api_bytes_per_pixel(mode->depth);
	if ((page >= mode->page_count) ||
	    ((mode->depth != 16u) && (mode->depth != 32u)) ||
	    (source_view->bytes_per_pixel != destination_bpp) ||
	    (mode->shadow_pages == NULL) ||
	    (mode->shadow_pages[page] == NULL) ||
	    (mode->shadow_valid == NULL) || !mode->shadow_valid[page] ||
	    (mode->shadow_pitch == 0u))
		return FB_GFX3_UNSUPPORTED;
	clip = state->view;
	result = image_api_clip_put(&destination_x, &destination_y, &source_x,
		&source_y, &width, &height, &clip);
	if ((result != FB_GFX3_OK) || (width == 0u) || (height == 0u))
		return result;
	source_pixels = source_view->pixels +
		((size_t)source_y * source_view->pitch) +
		((size_t)source_x * source_view->bytes_per_pixel);
	destination_pixels = mode->shadow_pages[page] +
		((size_t)destination_y * mode->shadow_pitch) +
		((size_t)destination_x * destination_bpp);
	if (put_mode == FB_GFX3_BLIT_PSET) {
		if (fb_gfx3_size_multiply(width, destination_bpp, &row_bytes) !=
		    FB_GFX3_OK)
			return FB_GFX3_INVALID;
		for (row = 0u; row < height; row++) {
			memcpy(destination_pixels + (size_t)row * mode->shadow_pitch,
				source_pixels + (size_t)row * source_view->pitch,
				row_bytes);
		}
		return FB_GFX3_OK;
	}
	return fb_gfx3_image_put_pixels(source_pixels, destination_pixels,
		width, height, source_view->pitch, mode->shadow_pitch,
		destination_bpp, put_mode, alpha, NULL, NULL);
}

/*
	Screen pages are normalized to one of gfxlib3's GPU storage depths, but a
	CPU FB.IMAGE can retain the depth with which it was created or BLOADed.
	gfxlib2 accepts PUT across the common RGB565 and BGRA image layouts.  Build
	a tightly packed temporary only for that mismatch so the ordinary matching
	depth path remains a direct upload.

	Indexed conversions need the active palette and are therefore kept out of
	this helper.  The compatibility images used by existing true-colour games
	are RGB565 or BGRA, where the conversion is unambiguous.
*/
static int image_api_convert_put_source(const FB_GFX3_IMAGE_VIEW *source_view,
	int source_x, int source_y, uint32_t width, uint32_t height,
	uint32_t target_bpp, unsigned char **converted, uint32_t *pitch)
{
	unsigned char *pixels;
	uint32_t row_bytes;
	size_t allocation_size;
	uint32_t x;
	uint32_t y;

	if ((source_view == NULL) || (converted == NULL) || (pitch == NULL) ||
	    (width == 0) || (height == 0) ||
	    !((source_view->bytes_per_pixel == 2) ||
	      (source_view->bytes_per_pixel == 4)) ||
	    !((target_bpp == 2) || (target_bpp == 4)) ||
	    (image_api_u32_multiply(width, target_bpp, &row_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(row_bytes, height, &allocation_size) !=
	     FB_GFX3_OK) || (allocation_size == 0u))
		return FB_GFX3_INVALID;
	pixels = (unsigned char *)malloc(allocation_size);
	if (pixels == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			uint32_t color = fb_gfx3_image_get_pixel_raw(source_view,
				source_x + (int)x, source_y + (int)y);

			if (source_view->bytes_per_pixel == 2)
				color = fb_gfx3_image_expand_color(2, color);
			if (target_bpp == 2)
				color = fb_gfx3_image_fix_color(2, color);
			image_api_store_pixel(pixels + ((size_t)y * row_bytes) +
				((size_t)x * target_bpp), target_bpp, color);
		}
	}
	*converted = pixels;
	*pitch = row_bytes;
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* GPU cache for stable CPU FB.IMAGE sources                                 */
/* ------------------------------------------------------------------------- */

static int image_api_hash_source(const FB_GFX3_IMAGE_VIEW *source_view,
	uint64_t *hash_result)
{
	const unsigned char *row;
	uint64_t hash = UINT64_C(0x9E3779B185EBCA87);
	uint32_t row_bytes;
	uint32_t offset;
	uint32_t y;

	if ((source_view == NULL) || (hash_result == NULL) ||
	    (source_view->pixels == NULL) || (source_view->width == 0) ||
	    (source_view->height == 0) ||
	    (image_api_u32_multiply(source_view->width,
	     source_view->bytes_per_pixel, &row_bytes) != FB_GFX3_OK) ||
	    (source_view->pitch < row_bytes))
		return FB_GFX3_INVALID;
	/*
		A normal FB.IMAGE exposes writable pixels directly.  There is no
		mutation callback to invalidate its GPU copy, so each compatible PUT
		must validate a content signature before it reuses that copy.  Hashing
		one byte at a time made a conventional sprite loop CPU-bound.  Mix
		whole unaligned machine words through memcpy instead: it remains safe
		for every FB.IMAGE pitch and catches raw image edits, without relying
		on an aliasing or alignment assumption the public image layout does
		not make.
	*/
	for (y = 0; y < source_view->height; y++) {
		row = source_view->pixels + ((size_t)y * source_view->pitch);
		for (offset = 0; offset + sizeof(uint64_t) <= row_bytes;
		     offset += sizeof(uint64_t)) {
			uint64_t word;

			memcpy(&word, row + offset, sizeof(word));
			hash ^= word + UINT64_C(0x9E3779B185EBCA87) +
				(hash << 6) + (hash >> 2);
			hash = (hash << 31) | (hash >> (64 - 31));
			hash *= UINT64_C(0xC2B2AE3D27D4EB4F);
		}
		for (; offset < row_bytes; offset++) {
			hash ^= row[offset];
			hash *= UINT64_C(0x100000001B3);
		}
	}
	/* Distinguish a differently shaped image with identical pixel bytes. */
	hash ^= ((uint64_t)source_view->width << 32) | source_view->height;
	hash ^= ((uint64_t)source_view->pitch << 32) |
		source_view->bytes_per_pixel;
	hash ^= hash >> 33;
	hash *= UINT64_C(0xFF51AFD7ED558CCD);
	hash ^= hash >> 33;
	*hash_result = hash;
	return FB_GFX3_OK;
}

static int image_api_hash_region(const FB_GFX3_IMAGE_VIEW *source_view,
	int source_x, int source_y, uint32_t width, uint32_t height,
	uint64_t *hash_result)
{
	const unsigned char *row;
	uint64_t hash = UINT64_C(0x9E3779B185EBCA87);
	uint32_t row_bytes;
	uint32_t offset;
	uint32_t y;

	if ((source_view == NULL) || (hash_result == NULL) ||
	    (source_view->pixels == NULL) || (source_x < 0) || (source_y < 0) ||
	    (width == 0u) || (height == 0u) ||
	    ((uint32_t)source_x >= source_view->width) ||
	    ((uint32_t)source_y >= source_view->height) ||
	    (width > source_view->width - (uint32_t)source_x) ||
	    (height > source_view->height - (uint32_t)source_y) ||
	    (image_api_u32_multiply(width, source_view->bytes_per_pixel,
	     &row_bytes) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	/*
		Oversized externally writable sheets can own many cached tiles. Hash
		only the tile represented by this entry so the bounded-memory fallback
		does not rescan the complete sheet once per sprite.
	*/
	for (y = 0u; y < height; y++) {
		row = source_view->pixels +
			((size_t)((uint32_t)source_y + y) * source_view->pitch) +
			((size_t)(uint32_t)source_x *
			 source_view->bytes_per_pixel);
		for (offset = 0u; offset + sizeof(uint64_t) <= row_bytes;
		     offset += sizeof(uint64_t)) {
			uint64_t word;

			memcpy(&word, row + offset, sizeof(word));
			hash ^= word + UINT64_C(0x9E3779B185EBCA87) +
				(hash << 6) + (hash >> 2);
			hash = (hash << 31) | (hash >> (64 - 31));
			hash *= UINT64_C(0xC2B2AE3D27D4EB4F);
		}
		for (; offset < row_bytes; offset++) {
			hash ^= row[offset];
			hash *= UINT64_C(0x100000001B3);
		}
	}
	hash ^= ((uint64_t)width << 32) | height;
	hash ^= ((uint64_t)source_view->pitch << 32) |
		source_view->bytes_per_pixel;
	hash ^= hash >> 33;
	hash *= UINT64_C(0xFF51AFD7ED558CCD);
	hash ^= hash >> 33;
	*hash_result = hash;
	return FB_GFX3_OK;
}

static size_t image_api_cache_snapshot_bytes_without_entry(
	const FB_GFX3_CONTEXT *context,
	const FB_GFX3_IMAGE_CACHE_ENTRY *ignored_entry)
{
	size_t total = 0u;
	uint32_t index;

	if (context == NULL)
		return 0u;
	for (index = 0u; index < FB_GFX3_IMAGE_CACHE_CAPACITY; index++) {
		const FB_GFX3_IMAGE_CACHE_ENTRY *entry =
			&context->image_cache[index];

		if ((entry == ignored_entry) || (entry->snapshot == NULL))
			continue;
		if (entry->snapshot_size > SIZE_MAX - total)
			return SIZE_MAX;
		total += entry->snapshot_size;
	}
	return total;
}

static void image_api_cache_reset_entry(FB_GFX3_CONTEXT *context,
	FB_GFX3_IMAGE_CACHE_ENTRY *entry)
{
	if (entry != NULL) {
		if (context != NULL)
			context->image_cache_snapshot_bytes =
				image_api_cache_snapshot_bytes_without_entry(context,
					entry);
		free(entry->snapshot);
		memset(entry, 0, sizeof(*entry));
	}
}

static void image_api_cache_discard_snapshot(FB_GFX3_CONTEXT *context,
	FB_GFX3_IMAGE_CACHE_ENTRY *entry)
{
	if (entry == NULL)
		return;
	if (context != NULL)
		context->image_cache_snapshot_bytes =
			image_api_cache_snapshot_bytes_without_entry(context, entry);
	free(entry->snapshot);
	entry->snapshot = NULL;
	entry->snapshot_size = 0u;
	entry->content_hash = 0u;
	entry->uniform_color = 0u;
	entry->uniform_full_image = FALSE;
}

static int image_api_cache_snapshot_size(const FB_GFX3_IMAGE_VIEW *source_view,
	size_t *size_result)
{
	uint32_t row_bytes;
	size_t size;

	if ((source_view == NULL) || (size_result == NULL) ||
	    (image_api_u32_multiply(source_view->width,
	     source_view->bytes_per_pixel, &row_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(row_bytes, source_view->height, &size) !=
	     FB_GFX3_OK))
		return FB_GFX3_INVALID;
	*size_result = size;
	return FB_GFX3_OK;
}

static int image_api_cache_snapshot_region_matches(
	const FB_GFX3_IMAGE_CACHE_ENTRY *entry,
	const FB_GFX3_IMAGE_VIEW *source_view, int source_x, int source_y,
	uint32_t width, uint32_t height, int *matches)
{
	uint32_t row_bytes;
	uint32_t region_row_bytes;
	uint32_t row;
	size_t size;
	uint64_t hash;
	int region_snapshot;
	int result;

	if ((entry == NULL) || (source_view == NULL) || (matches == NULL) ||
	    (source_x < 0) || (source_y < 0) || (width == 0u) ||
	    (height == 0u) ||
	    ((uint32_t)source_x >= source_view->width) ||
	    ((uint32_t)source_y >= source_view->height) ||
	    (width > source_view->width - (uint32_t)source_x) ||
	    (height > source_view->height - (uint32_t)source_y))
		return FB_GFX3_INVALID;
	region_snapshot = entry->caches_source_region;
	if (region_snapshot) {
		if ((entry->cached_source_x != (uint32_t)source_x) ||
		    (entry->cached_source_y != (uint32_t)source_y) ||
		    (entry->cached_source_width != width) ||
		    (entry->cached_source_height != height) ||
		    (image_api_u32_multiply(width,
		     source_view->bytes_per_pixel, &region_row_bytes) !=
		     FB_GFX3_OK) ||
		    (fb_gfx3_size_multiply(region_row_bytes, height, &size) !=
		     FB_GFX3_OK))
			return FB_GFX3_INVALID;
	} else {
		if ((image_api_cache_snapshot_size(source_view, &size) !=
		     FB_GFX3_OK) ||
		    (image_api_u32_multiply(width,
		     source_view->bytes_per_pixel, &region_row_bytes) !=
		     FB_GFX3_OK))
			return FB_GFX3_INVALID;
	}
	if (entry->snapshot_size == size) {
		if ((size != 0) && (entry->snapshot == NULL))
			return FB_GFX3_INVALID;
		row_bytes = source_view->width * source_view->bytes_per_pixel;
		for (row = 0; row < height; row++) {
			size_t packed_offset = region_snapshot ?
				(size_t)row * region_row_bytes :
				((size_t)((uint32_t)source_y + row) * row_bytes) +
					((size_t)(uint32_t)source_x *
					 source_view->bytes_per_pixel);
			size_t source_offset =
				((size_t)((uint32_t)source_y + row) *
				 source_view->pitch) +
				((size_t)(uint32_t)source_x *
				 source_view->bytes_per_pixel);

			if (memcmp(entry->snapshot + packed_offset,
			    source_view->pixels + source_offset,
			    region_row_bytes) != 0) {
				*matches = FALSE;
				return FB_GFX3_OK;
			}
		}
		*matches = TRUE;
		return FB_GFX3_OK;
	}
	if (entry->snapshot_size != 0)
		return FB_GFX3_INVALID;
	if (region_snapshot)
		result = image_api_hash_region(source_view, source_x, source_y,
			width, height, &hash);
	else
		result = image_api_hash_source(source_view, &hash);
	if (result != FB_GFX3_OK)
		return result;
	*matches = (entry->content_hash == hash);
	return FB_GFX3_OK;
}

static int image_api_cache_snapshot_store(FB_GFX3_CONTEXT *context,
	FB_GFX3_IMAGE_CACHE_ENTRY *entry,
	const FB_GFX3_IMAGE_VIEW *source_view, int region_snapshot,
	int source_x, int source_y, uint32_t width, uint32_t height)
{
	unsigned char *new_snapshot = NULL;
	uint32_t row_bytes;
	uint32_t row;
	uint32_t snapshot_height;
	uint32_t snapshot_width;
	uint32_t snapshot_x;
	uint32_t snapshot_y;
	uint32_t uniform_color;
	size_t bytes_without_entry;
	size_t snapshot_budget;
	size_t size;
	uint64_t hash;
	int uniform = TRUE;
	int result;

	if ((context == NULL) || (entry == NULL) || (source_view == NULL))
		return FB_GFX3_INVALID;
	if (region_snapshot) {
		if ((source_x < 0) || (source_y < 0) || (width == 0u) ||
		    (height == 0u) ||
		    ((uint32_t)source_x >= source_view->width) ||
		    ((uint32_t)source_y >= source_view->height) ||
		    (width > source_view->width - (uint32_t)source_x) ||
		    (height > source_view->height - (uint32_t)source_y))
			return FB_GFX3_INVALID;
		snapshot_x = (uint32_t)source_x;
		snapshot_y = (uint32_t)source_y;
		snapshot_width = width;
		snapshot_height = height;
	} else {
		snapshot_x = 0u;
		snapshot_y = 0u;
		snapshot_width = source_view->width;
		snapshot_height = source_view->height;
	}
	if ((image_api_u32_multiply(snapshot_width,
	     source_view->bytes_per_pixel, &row_bytes) != FB_GFX3_OK) ||
	    (row_bytes == 0u) || (snapshot_height == 0u) ||
	    (fb_gfx3_size_multiply(row_bytes, snapshot_height, &size) !=
	     FB_GFX3_OK) ||
	    (size == 0u))
		return FB_GFX3_INVALID;
	bytes_without_entry = image_api_cache_snapshot_bytes_without_entry(context,
		entry);
	snapshot_budget = fb_gfx3_target_image_cache_snapshot_budget();
	/*
		Exact snapshots let the C library's vectorized memcmp validate a stable
		image much more cheaply than a full serial content hash. A cache-wide
		budget prevents 128 unusually large images from multiplying the
		per-entry limit into an unreasonable allocation. Images which do not
		fit remain exact through the bounded-memory hash path.
	*/
	if ((size > FB_GFX3_IMAGE_CACHE_SNAPSHOT_MAX_BYTES) ||
	    (size > snapshot_budget) ||
	    (bytes_without_entry > snapshot_budget - size)) {
		if (region_snapshot)
			result = image_api_hash_region(source_view, source_x, source_y,
				width, height, &hash);
		else
			result = image_api_hash_source(source_view, &hash);
		if (result != FB_GFX3_OK)
			return result;
		free(entry->snapshot);
		entry->snapshot = NULL;
		entry->snapshot_size = 0;
		context->image_cache_snapshot_bytes = bytes_without_entry;
		entry->uniform_full_image = FALSE;
		entry->uniform_color = 0;
		entry->content_hash = hash;
		return FB_GFX3_OK;
	}
	new_snapshot = (unsigned char *)malloc(size);
	if (new_snapshot == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	if (region_snapshot) {
		uniform = FALSE;
		uniform_color = 0u;
	} else {
		uniform_color = image_api_load_pixel(source_view->pixels,
			source_view->bytes_per_pixel);
	}
	for (row = 0; row < snapshot_height; row++)
	{
		const unsigned char *source_row = source_view->pixels +
			((size_t)(snapshot_y + row) * source_view->pitch) +
			((size_t)snapshot_x * source_view->bytes_per_pixel);
		uint32_t column;

		memcpy(new_snapshot + ((size_t)row * row_bytes), source_row,
			row_bytes);
		for (column = 0; !region_snapshot && (column < snapshot_width);
		     column++) {
			if (image_api_load_pixel(source_row +
			    ((size_t)column * source_view->bytes_per_pixel),
			    source_view->bytes_per_pixel) != uniform_color) {
				uniform = FALSE;
				break;
			}
		}
	}
	/*
		Allocate and populate the replacement before releasing the old snapshot.
		A failed refresh must leave the prior cache record usable.
	*/
	free(entry->snapshot);
	entry->snapshot = new_snapshot;
	entry->snapshot_size = size;
	context->image_cache_snapshot_bytes = bytes_without_entry + size;
	entry->content_hash = 0;
	entry->uniform_full_image = uniform;
	entry->uniform_color = uniform_color;
	return FB_GFX3_OK;
}

static int image_api_cache_destroy_entry(FB_GFX3_CONTEXT *context,
	FB_GFX3_IMAGE_CACHE_ENTRY *entry)
{
	int result = FB_GFX3_OK;

	if ((context == NULL) || (entry == NULL))
		return FB_GFX3_INVALID;
	/*
		An atlas-backed entry owns only its cell metadata. The context owns the
		shared surface and retires it once after all cached commands have drained.
	*/
	if ((entry->surface.handle != 0) && !entry->uses_atlas)
		result = fb_gfx3_surface_destroy(&entry->surface);
	if (result == FB_GFX3_OK)
		image_api_cache_reset_entry(context, entry);
	return result;
}

/*
	Place a CPU image in the shared GPU atlas.

	Desktop compute backends expose heterogeneous BLITS packets, but Vulkan and
	OpenGL still bind one source allocation for each hardware draw or dispatch.
	Dynamically packing large backgrounds and ordinary sprites into the same
	allocation therefore removes source-change splits from otherwise contiguous
	PUT streams. The monotonic shelf allocator never relocates a live image, so
	queued source rectangles remain valid.

	GLES uses the original fixed-cell mapping to retain its smaller memory
	footprint. A failed atlas allocation is not fatal. A dedicated allocation can
	still succeed when a device cannot reserve the complete atlas.
*/
static int image_api_cache_assign_atlas(FB_GFX3_CONTEXT *context,
	FB_GFX3_IMAGE_CACHE_ENTRY *entry,
	const FB_GFX3_IMAGE_VIEW *source_view, uint32_t depth, int *assigned)
{
	uint32_t atlas_height;
	uint32_t atlas_width;
	uint32_t cell_size;
	uint32_t entry_index;
	uint32_t features;
	uint32_t region_height;
	uint32_t region_width;
	uint32_t usage;
	int dynamic_atlas;
	int result;

	if (assigned == NULL)
		return FB_GFX3_INVALID;
	*assigned = FALSE;
	if ((context == NULL) || (entry == NULL) || (source_view == NULL))
		return FB_GFX3_INVALID;
	features = context->renderer.backend.caps.features;
	dynamic_atlas = ((features & (FB_GFX3_FEATURE_COMPUTE |
		FB_GFX3_FEATURE_HETEROGENEOUS_BLITS)) ==
		(FB_GFX3_FEATURE_COMPUTE |
		FB_GFX3_FEATURE_HETEROGENEOUS_BLITS));
	if (dynamic_atlas) {
		cell_size = 0u;
		atlas_width = FB_GFX3_IMAGE_CACHE_ATLAS_DESKTOP_WIDTH;
		atlas_height = FB_GFX3_IMAGE_CACHE_ATLAS_DESKTOP_HEIGHT;
		if ((source_view->width >
		     FB_GFX3_IMAGE_CACHE_ATLAS_DESKTOP_MAX_IMAGE_SIZE) ||
		    (source_view->height >
		     FB_GFX3_IMAGE_CACHE_ATLAS_DESKTOP_MAX_IMAGE_SIZE))
			return FB_GFX3_OK;
	} else {
		cell_size = FB_GFX3_IMAGE_CACHE_ATLAS_GLES_CELL_SIZE;
		atlas_width = cell_size * FB_GFX3_IMAGE_CACHE_ATLAS_COLUMNS;
		atlas_height = cell_size * FB_GFX3_IMAGE_CACHE_ATLAS_ROWS;
		if ((source_view->width > cell_size) ||
		    (source_view->height > cell_size))
			return FB_GFX3_OK;
	}
	if (((features & FB_GFX3_FEATURE_PACKED_BLITS) == 0u) ||
	    (context->renderer.backend.caps.max_surface_width <
	     atlas_width) ||
	    (context->renderer.backend.caps.max_surface_height <
	     atlas_height))
		return FB_GFX3_OK;
	if (!context->image_cache_atlas_attempted) {
		context->image_cache_atlas_attempted = TRUE;
		usage = FB_GFX3_SURFACE_SAMPLED |
			FB_GFX3_SURFACE_TRANSFER_SOURCE |
			FB_GFX3_SURFACE_TRANSFER_DESTINATION;
		result = fb_gfx3_surface_create(context, &context->image_cache_atlas,
			atlas_width, atlas_height, depth, usage, 0);
		if (result != FB_GFX3_OK) {
			memset(&context->image_cache_atlas, 0,
				sizeof(context->image_cache_atlas));
			fb_gfx3_log_write(&context->logger, FB_GFX3_LOG_INFO,
				"sprite atlas allocation unavailable; using dedicated surfaces");
			return FB_GFX3_OK;
		}
		context->image_cache_atlas_dynamic = dynamic_atlas;
		context->image_cache_atlas_next_x = 0u;
		context->image_cache_atlas_next_y = 0u;
		context->image_cache_atlas_row_height = 0u;
	}
	if ((context->image_cache_atlas.handle == 0) ||
	    (context->image_cache_atlas.depth != depth))
		return FB_GFX3_OK;
	if (context->image_cache_atlas_dynamic) {
		const uint32_t alignment =
			FB_GFX3_IMAGE_CACHE_ATLAS_DESKTOP_ALIGNMENT;

		if ((source_view->width > UINT32_MAX - (alignment - 1u)) ||
		    (source_view->height > UINT32_MAX - (alignment - 1u)))
			return FB_GFX3_OK;
		region_width = (source_view->width + alignment - 1u) &
			~(alignment - 1u);
		region_height = (source_view->height + alignment - 1u) &
			~(alignment - 1u);
		if ((region_width == 0u) || (region_height == 0u) ||
		    (region_width > atlas_width) || (region_height > atlas_height))
			return FB_GFX3_OK;
		if (context->image_cache_atlas_next_x >
		    atlas_width - region_width) {
			if (context->image_cache_atlas_next_y >
			    atlas_height - context->image_cache_atlas_row_height)
				return FB_GFX3_OK;
			context->image_cache_atlas_next_x = 0u;
			context->image_cache_atlas_next_y +=
				context->image_cache_atlas_row_height;
			context->image_cache_atlas_row_height = 0u;
		}
		if (context->image_cache_atlas_next_y >
		    atlas_height - region_height)
			return FB_GFX3_OK;
		entry->surface = context->image_cache_atlas;
		entry->atlas_x = context->image_cache_atlas_next_x;
		entry->atlas_y = context->image_cache_atlas_next_y;
		entry->uses_atlas = TRUE;
		context->image_cache_atlas_next_x += region_width;
		if (region_height > context->image_cache_atlas_row_height)
			context->image_cache_atlas_row_height = region_height;
		*assigned = TRUE;
		return FB_GFX3_OK;
	}
	entry_index = (uint32_t)(entry - context->image_cache);
	if (entry_index >= FB_GFX3_IMAGE_CACHE_CAPACITY)
		return FB_GFX3_INVALID;
	entry->surface = context->image_cache_atlas;
	entry->atlas_x = (entry_index % FB_GFX3_IMAGE_CACHE_ATLAS_COLUMNS) *
		cell_size;
	entry->atlas_y = (entry_index / FB_GFX3_IMAGE_CACHE_ATLAS_COLUMNS) *
		cell_size;
	entry->uses_atlas = TRUE;
	*assigned = TRUE;
	return FB_GFX3_OK;
}

static int image_api_cache_find_or_create(FB_GFX3_CONTEXT *context,
	const FB_GFX3_IMAGE_VIEW *source_view, int source_x, int source_y,
	uint32_t source_width, uint32_t source_height, uint32_t depth,
	FB_GFX3_IMAGE_CACHE_ENTRY **entry_result, int *needs_upload)
{
	FB_GFX3_IMAGE_CACHE_ENTRY *entry = NULL;
	FB_GFX3_IMAGE_CACHE_ENTRY *empty = NULL;
	FB_GFX3_IMAGE_CACHE_ENTRY *oldest = NULL;
	uint32_t index;
	uint32_t lookup_entry;
	uint32_t lookup_slot;
	uint32_t usage;
	uint32_t generation = 0;
	FB_GFX3_IMAGE_VIEW allocation_view;
	uintptr_t lookup_key;
	int atlas_assigned;
	int content_changed = FALSE;
	int external_write = TRUE;
	int metadata_owned;
	int matches;
	int region_cache;
	int result;

	if ((context == NULL) || (source_view == NULL) ||
	    (entry_result == NULL) || (needs_upload == NULL) ||
	    (source_view->header == NULL))
		return FB_GFX3_INVALID;
	metadata_owned = fb_gfx3_image_cache_metadata_get(source_view,
		&generation, &external_write);
	/*
		A shallow tile sheet can be wider than the device's maximum texture
		dimension even though each requested sprite is small. Cache the requested
		source rectangle in that case. This keeps all rendering on the GPU while
		avoiding an impossible whole-sheet allocation on mobile GLES hardware.
	*/
	region_cache =
		(source_view->width >
		 context->renderer.backend.caps.max_surface_width) ||
		(source_view->height >
		 context->renderer.backend.caps.max_surface_height);
	if (region_cache &&
	    ((source_width >
	      context->renderer.backend.caps.max_surface_width) ||
	     (source_height >
	      context->renderer.backend.caps.max_surface_height)))
		return FB_GFX3_UNSUPPORTED;
	/*
		FB.IMAGE headers are naturally aligned, so mix several higher pointer
		groups rather than using the always-zero low bits directly. The table is
		a front end for the exact scan, not an ownership structure.
	*/
	lookup_key = (uintptr_t)source_view->header;
	if (region_cache) {
		lookup_key ^= ((uintptr_t)(uint32_t)source_x << 7);
		lookup_key ^= ((uintptr_t)(uint32_t)source_y << 17);
		lookup_key ^= ((uintptr_t)source_width << 27);
		lookup_key ^= ((uintptr_t)source_height << 3);
	}
	lookup_slot = (uint32_t)((lookup_key >> 4) ^ (lookup_key >> 12) ^
		(lookup_key >> 20)) & (FB_GFX3_IMAGE_CACHE_LOOKUP_CAPACITY - 1u);
	lookup_entry = context->image_cache_lookup[lookup_slot];
	if ((lookup_entry != 0u) &&
	    (lookup_entry <= FB_GFX3_IMAGE_CACHE_CAPACITY) &&
	    (context->image_cache[lookup_entry - 1u].image_header ==
	     source_view->header) &&
	    (context->image_cache[lookup_entry - 1u].caches_source_region ==
	     region_cache) &&
	    (!region_cache ||
	     ((context->image_cache[lookup_entry - 1u].cached_source_x ==
	       (uint32_t)source_x) &&
	      (context->image_cache[lookup_entry - 1u].cached_source_y ==
	       (uint32_t)source_y) &&
	      (context->image_cache[lookup_entry - 1u].cached_source_width ==
	       source_width) &&
	      (context->image_cache[lookup_entry - 1u].cached_source_height ==
	       source_height)))) {
		index = lookup_entry - 1u;
		entry = &context->image_cache[index];
	} else {
		for (index = 0; index < FB_GFX3_IMAGE_CACHE_CAPACITY; index++) {
			entry = &context->image_cache[index];
			if ((entry->image_header == source_view->header) &&
			    (entry->caches_source_region == region_cache) &&
			    (!region_cache ||
			     ((entry->cached_source_x == (uint32_t)source_x) &&
			      (entry->cached_source_y == (uint32_t)source_y) &&
			      (entry->cached_source_width == source_width) &&
			      (entry->cached_source_height == source_height))))
				break;
			if (entry->image_header == NULL) {
				if (empty == NULL)
					empty = entry;
				continue;
			}
			if ((oldest == NULL) || (entry->last_use < oldest->last_use))
				oldest = entry;
		}
	}
	if (index == FB_GFX3_IMAGE_CACHE_CAPACITY) {
		/*
			Prefer unused storage before evicting the least-recently-used image.
			The previous single-candidate search selected entry zero as "oldest"
			before it reached later empty slots. An application alternating two
			sprites therefore destroyed and recreated a GPU texture on every PUT,
			effectively reducing the 128-entry cache to one entry.
		*/
		entry = (empty != NULL) ? empty : oldest;
		if (entry == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		if (entry->image_header != NULL) {
			result = image_api_cache_destroy_entry(context, entry);
			if (result != FB_GFX3_OK)
				return result;
		}
	} else if ((entry->width != source_view->width) ||
	           (entry->height != source_view->height) ||
	           (entry->pitch != source_view->pitch) ||
	           (entry->bytes_per_pixel != source_view->bytes_per_pixel) ||
	           (entry->caches_source_region != region_cache) ||
	           (region_cache &&
	            ((entry->cached_source_x != (uint32_t)source_x) ||
	             (entry->cached_source_y != (uint32_t)source_y) ||
	             (entry->cached_source_width != source_width) ||
	             (entry->cached_source_height != source_height))) ||
	           (entry->surface.depth != depth)) {
		result = image_api_cache_destroy_entry(context, entry);
		if (result != FB_GFX3_OK)
			return result;
	} else {
		/*
			A gfxlib3-owned image that has not exposed pixels through IMAGEINFO
			can be trusted until a compatibility writer advances its generation.
			Images with an exposed pixel pointer retain exact snapshot validation.
		*/
		if (metadata_owned && !external_write && entry->metadata_owned) {
			matches = (entry->image_generation == generation);
			result = FB_GFX3_OK;
		} else {
			/*
				Direct IMAGEINFO access makes the public pixel pointer mutable,
				but a partial PUT can observe only its requested source rectangle.
				Validate that rectangle against the exact snapshot instead of
				rescanning a complete sprite sheet for every tile. A later PUT
				of a different region will validate that region before it is
				sampled, so direct writes remain visible at the same API boundary.
			*/
			result = image_api_cache_snapshot_region_matches(entry,
				source_view, source_x, source_y, source_width,
				source_height, &matches);
		}
		if (result != FB_GFX3_OK)
			return result;
		if (!matches) {
			/*
				Pixel mutation does not change the GPU allocation contract. Reuse
				the existing same-sized texture and queue an upload before its next
				blit. Destroying and recreating it made animated CPU images perform
				two render-thread round trips for every PUT.

				OpenGL and Vulkan both preserve the ordered upload-then-blit
				sequence, which is the same lifetime rule used by the context's
				reusable image upload surface.
			*/
			if (metadata_owned && !external_write) {
				image_api_cache_discard_snapshot(context, entry);
				result = FB_GFX3_OK;
			} else {
				result = image_api_cache_snapshot_store(context, entry,
					source_view, region_cache, source_x, source_y,
					source_width, source_height);
			}
			if (result != FB_GFX3_OK)
				return result;
			content_changed = TRUE;
		}
	}
	if (entry->surface.handle == 0) {
		if (metadata_owned && !external_write) {
			image_api_cache_discard_snapshot(context, entry);
			result = FB_GFX3_OK;
		} else {
			result = image_api_cache_snapshot_store(context, entry,
				source_view, region_cache, source_x, source_y,
				source_width, source_height);
		}
		if (result != FB_GFX3_OK) {
			image_api_cache_reset_entry(context, entry);
			return result;
		}
		allocation_view = *source_view;
		if (region_cache) {
			allocation_view.width = source_width;
			allocation_view.height = source_height;
		}
		result = image_api_cache_assign_atlas(context, entry, &allocation_view,
			depth, &atlas_assigned);
		if (result != FB_GFX3_OK) {
			image_api_cache_reset_entry(context, entry);
			return result;
		}
		if (!atlas_assigned) {
			usage = FB_GFX3_SURFACE_SAMPLED |
				FB_GFX3_SURFACE_TRANSFER_SOURCE |
				FB_GFX3_SURFACE_TRANSFER_DESTINATION;
			result = fb_gfx3_surface_create(context, &entry->surface,
				allocation_view.width, allocation_view.height, depth,
				usage, 0);
			if (result != FB_GFX3_OK) {
				image_api_cache_reset_entry(context, entry);
				return result;
			}
		}
		entry->image_header = source_view->header;
		entry->width = source_view->width;
		entry->height = source_view->height;
		entry->pitch = source_view->pitch;
		entry->bytes_per_pixel = source_view->bytes_per_pixel;
		entry->cached_source_x = region_cache ? (uint32_t)source_x : 0u;
		entry->cached_source_y = region_cache ? (uint32_t)source_y : 0u;
		entry->cached_source_width =
			region_cache ? source_width : source_view->width;
		entry->cached_source_height =
			region_cache ? source_height : source_view->height;
		entry->caches_source_region = region_cache;
		entry->image_generation = generation;
		entry->metadata_owned = metadata_owned;
		context->image_cache_lookup[lookup_slot] =
			(uint32_t)(entry - context->image_cache) + 1u;
		fb_gfx3_log_write(&context->logger, FB_GFX3_LOG_TRACE,
			"CPU image cache allocation: image=%ux%u source=%u,%u %ux%u "
			"atlas=%d origin=%u,%u",
			source_view->width, source_view->height,
			entry->cached_source_x, entry->cached_source_y,
			entry->cached_source_width, entry->cached_source_height,
			entry->uses_atlas, entry->atlas_x, entry->atlas_y);
		*needs_upload = TRUE;
	} else {
		entry->image_generation = generation;
		entry->metadata_owned = metadata_owned;
		context->image_cache_lookup[lookup_slot] = index + 1u;
		*needs_upload = content_changed;
	}
	context->image_cache_clock++;
	if (context->image_cache_clock == 0)
		context->image_cache_clock = 1;
	entry->last_use = context->image_cache_clock;
	*entry_result = entry;
	return FB_GFX3_OK;
}

static void image_api_cache_remove(FB_GFX3_CONTEXT *context,
	const void *image_header)
{
	uint32_t index;

	if ((context == NULL) || (image_header == NULL))
		return;
	for (index = 0; index < FB_GFX3_IMAGE_CACHE_CAPACITY; index++) {
		FB_GFX3_IMAGE_CACHE_ENTRY *entry = &context->image_cache[index];

		if (entry->image_header != image_header)
			continue;
		if (image_api_cache_destroy_entry(context, entry) != FB_GFX3_OK)
			fb_gfx3_log_write(&context->logger, FB_GFX3_LOG_WARNING,
				"could not retire destroyed CPU image cache entry");
	}
}

/*
	A full CPU image that is one native pixel value does not require texture
	sampling for the common opaque PUT forms. The cache has already compared the
	image against its exact snapshot, so this decision remains correct after a
	direct caller write. Keep this restricted to the complete source image:
	partial PUT rectangles can be uniform even when the full image is not, but
	discovering that on every sprite would move graphics work back to the CPU.
*/
static int image_api_put_uniform_full_image(FB_GFX3_SURFACE *destination,
	const FB_GFX3_RECT *clip, const FB_GFX3_IMAGE_CACHE_ENTRY *entry,
	int source_x, int source_y, uint32_t width, uint32_t height,
	int destination_x, int destination_y, int mode, int *handled)
{
	uint32_t color;
	uint32_t mask;
	uint32_t transparent_key;
	uint32_t destination_bytes_per_pixel;
	int64_t x2;
	int64_t y2;

	if (handled == NULL)
		return FB_GFX3_INVALID;
	*handled = FALSE;
	if ((destination == NULL) || (clip == NULL) || (entry == NULL) ||
	    !entry->uniform_full_image || (source_x != 0) || (source_y != 0) ||
	    (width != entry->width) || (height != entry->height))
		return FB_GFX3_OK;
	/*
		Desktop compute backends coalesce opaque rectangles. GLES instead has a
		faster instanced BLIT batch for this source shape; routing thousands of
		sprites through its individual rectangle path would be a regression.
	*/
	if ((destination->context == NULL) ||
	    ((destination->context->renderer.backend.caps.features &
	      FB_GFX3_FEATURE_COMPUTE) == 0))
		return FB_GFX3_OK;
	destination_bytes_per_pixel = image_api_bytes_per_pixel(destination->depth);
	if (destination_bytes_per_pixel == 1u) {
		mask = 0xFFu;
		transparent_key = 0u;
	} else if (destination_bytes_per_pixel == 2u) {
		mask = 0xFFFFu;
		transparent_key = 0xF81Fu;
	} else if (destination_bytes_per_pixel == 4u) {
		mask = UINT32_MAX;
		transparent_key = 0xFFFF00FFu;
	} else {
		return FB_GFX3_INVALID;
	}
	/*
		The cache snapshot describes the public CPU image while its GPU surface
		uses the destination depth. Convert the known uniform value here too;
		otherwise a 32-bit IMAGECREATE put onto a RGB565 screen would turn the
		rectangle shortcut into a different colour from its cached texture.
	*/
	color = entry->uniform_color;
	if ((entry->bytes_per_pixel == 2u) &&
	    (destination_bytes_per_pixel == 4u))
		color = fb_gfx3_image_expand_color(2, color);
	else if ((entry->bytes_per_pixel == 4u) &&
	         (destination_bytes_per_pixel == 2u))
		color = fb_gfx3_image_fix_color(2, color);
	color &= mask;
	switch (mode) {
	case FB_GFX3_BLIT_PSET:
		break;
	case FB_GFX3_BLIT_PRESET:
		color = (~color) & mask;
		break;
	case FB_GFX3_BLIT_TRANS:
		if (((destination_bytes_per_pixel == 4u) &&
		     ((color & 0x00FFFFFFu) == 0x00FF00FFu)) ||
		    ((destination_bytes_per_pixel != 4u) &&
		     (color == transparent_key))) {
			*handled = TRUE;
			return FB_GFX3_OK;
		}
		break;
	default:
		return FB_GFX3_OK;
	}
	x2 = (int64_t)destination_x + width - 1u;
	y2 = (int64_t)destination_y + height - 1u;
	if ((x2 > INT_MAX) || (y2 > INT_MAX))
		return FB_GFX3_INVALID;
	*handled = TRUE;
	return fb_gfx3_surface_rectangle_graphics_locked(destination, clip,
		destination_x,
		destination_y, (int)x2, (int)y2, color, 0, TRUE, 0);
}

int fb_gfx3_image_put_surface(FB_GFX3_SURFACE *destination_surface,
	const FB_GFX3_RECT *clip,
	const FB_GFX3_IMAGE_VIEW *source_view, int source_x, int source_y,
	uint32_t width, uint32_t height, int destination_x, int destination_y,
	int mode, int alpha, BLENDER *blender, void *parameter)
{
	FB_GFX3_IMAGE_CACHE_ENTRY *cache_entry;
	FB_GFX3_SURFACE *upload_surface;
	FB_GFX3_RECT source_rect;
	unsigned char *converted_source = NULL;
	unsigned char *source = NULL;
	unsigned char *destination = NULL;
	uint32_t target_bpp;
	uint32_t row_bytes;
	uint32_t source_pitch;
	size_t destination_size;
	uint32_t usage;
	int cache_needs_upload;
	int cache_candidate;
	int uniform_handled;
	const char *transfer_operation;
	int result;

	if ((destination_surface == NULL) || (clip == NULL) ||
	    (source_view == NULL) || (width == 0u) || (height == 0u))
		return FB_GFX3_INVALID;
	/*
		PUT CUSTOM uses a synchronized CPU read/modify/upload sequence, but
		it remains a destination drawing operation.  Do not let transfer
		capabilities bypass the render-target contract used by GPU PUT modes.
	*/
	if ((destination_surface->usage & FB_GFX3_SURFACE_RENDER_TARGET) == 0)
		return FB_GFX3_UNSUPPORTED;
	target_bpp = image_api_bytes_per_pixel(destination_surface->depth);
	if ((target_bpp == 0u) ||
	    (image_api_u32_multiply(width, target_bpp, &row_bytes) != FB_GFX3_OK) ||
	    (row_bytes == 0u))
		return FB_GFX3_INVALID;
	if (mode == FB_GFX3_BLIT_CUSTOM) {
		/* CUSTOM executes its user blender on CPU pixels, before cache routing. */
		if (target_bpp == source_view->bytes_per_pixel) {
			source = source_view->pixels +
				((size_t)source_y * source_view->pitch) +
				((size_t)source_x * target_bpp);
			source_pitch = source_view->pitch;
		} else {
			result = image_api_convert_put_source(source_view, source_x, source_y,
				width, height, target_bpp, &converted_source, &source_pitch);
			if (result != FB_GFX3_OK)
				return result;
			source = converted_source;
		}
		if ((blender == NULL) ||
		    (fb_gfx3_size_multiply(row_bytes, height, &destination_size) !=
		     FB_GFX3_OK) || (destination_size == 0u)) {
			free(converted_source);
			return FB_GFX3_INVALID;
		}
		destination = (unsigned char *)malloc(destination_size);
		if (destination == NULL) {
			free(converted_source);
			return FB_GFX3_OUT_OF_MEMORY;
		}
		result = fb_gfx3_surface_download(destination_surface, destination_x,
			destination_y, width, height, row_bytes, destination);
		if (result == FB_GFX3_OK)
			result = fb_gfx3_image_put_pixels(source, destination, width,
				height, source_pitch, row_bytes, target_bpp,
				mode, alpha, blender, parameter);
		if (result == FB_GFX3_OK)
			result = fb_gfx3_surface_upload(destination_surface, destination_x,
				destination_y, width, height, row_bytes, destination);
		free(destination);
		free(converted_source);
		return result;
	}
	/*
		A stable CPU FB.IMAGE is commonly reused for every sprite in a frame.
		Keep a validated GPU copy for the direct-format path so repeated PUTs
		enqueue only a GPU blit. The content hash preserves correctness for
		direct CPU writes that bypass the public image drawing entry points.
	*/
	cache_candidate = (target_bpp == source_view->bytes_per_pixel);
	if (cache_candidate) {
		transfer_operation = "image cache lookup";
		result = image_api_cache_find_or_create(destination_surface->context,
			source_view, source_x, source_y, width, height,
			destination_surface->depth, &cache_entry,
			&cache_needs_upload);
		if (result != FB_GFX3_OK)
			goto done;
		/*
			Initialize the cached surface before a uniform shortcut returns. A
			later AND, XOR, or alpha PUT can reuse this image and must not sample
			an allocation that a preceding rectangle-only PUT left unwritten.
		*/
		if (cache_needs_upload) {
			uint32_t upload_width = cache_entry->cached_source_width;
			uint32_t upload_height = cache_entry->cached_source_height;

			transfer_operation = "image cache upload";
			source = source_view->pixels +
				((size_t)cache_entry->cached_source_y *
				 source_view->pitch) +
				((size_t)cache_entry->cached_source_x *
				 source_view->bytes_per_pixel);
			source_pitch = source_view->pitch;
			result = fb_gfx3_surface_upload(&cache_entry->surface,
				(int)cache_entry->atlas_x, (int)cache_entry->atlas_y,
				upload_width, upload_height, source_pitch, source);
			if (result != FB_GFX3_OK)
				goto done;
		}
		result = image_api_put_uniform_full_image(destination_surface, clip,
			cache_entry, source_x, source_y, width, height, destination_x,
			destination_y, mode, &uniform_handled);
		if (result != FB_GFX3_OK)
			goto done;
		if (uniform_handled)
			goto done;
		transfer_operation = "image cache blit";
		source_rect.x1 = (int32_t)cache_entry->atlas_x + source_x -
			(int32_t)cache_entry->cached_source_x;
		source_rect.y1 = (int32_t)cache_entry->atlas_y + source_y -
			(int32_t)cache_entry->cached_source_y;
		source_rect.x2 = source_rect.x1 + (int32_t)width - 1;
		source_rect.y2 = source_rect.y1 + (int32_t)height - 1;
		result = fb_gfx3_surface_blit_graphics_locked(destination_surface, clip,
			&cache_entry->surface, &source_rect, destination_x, destination_y,
			(uint32_t)mode, (uint32_t)alpha);
		goto done;
	}
	if (target_bpp == source_view->bytes_per_pixel) {
		source = source_view->pixels + ((size_t)source_y * source_view->pitch) +
			((size_t)source_x * target_bpp);
		source_pitch = source_view->pitch;
	} else {
		result = image_api_convert_put_source(source_view, source_x, source_y,
			width, height, target_bpp, &converted_source, &source_pitch);
		if (result != FB_GFX3_OK)
			return result;
		source = converted_source;
	}
	usage = FB_GFX3_SURFACE_SAMPLED | FB_GFX3_SURFACE_TRANSFER_SOURCE |
		FB_GFX3_SURFACE_TRANSFER_DESTINATION;
	upload_surface = &destination_surface->context->image_upload_surface;
	if ((upload_surface->handle != 0) &&
	    ((upload_surface->depth != destination_surface->depth) ||
	     (upload_surface->width < width) ||
	     (upload_surface->height < height))) {
		transfer_operation = "image upload surface resize";
		result = fb_gfx3_surface_destroy(upload_surface);
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (upload_surface->handle == 0) {
		transfer_operation = "image upload surface creation";
		result = fb_gfx3_surface_create(destination_surface->context,
			upload_surface, width, height, destination_surface->depth,
			usage, 0);
		if (result != FB_GFX3_OK)
			goto done;
	}
	transfer_operation = "image upload surface upload";
	result = fb_gfx3_surface_upload(upload_surface, 0, 0, width, height,
		source_pitch, source);
	if (result == FB_GFX3_OK) {
		transfer_operation = "image upload surface blit";
		source_rect.x1 = 0;
		source_rect.y1 = 0;
		source_rect.x2 = (int32_t)width - 1;
		source_rect.y2 = (int32_t)height - 1;
		result = fb_gfx3_surface_blit_graphics_locked(destination_surface, clip,
			upload_surface, &source_rect, destination_x, destination_y,
			(uint32_t)mode, (uint32_t)alpha);
	}

done:
	if (result != FB_GFX3_OK) {
		fb_gfx3_log_write(&destination_surface->context->logger,
			FB_GFX3_LOG_ERROR, "CPU image PUT %s failed: %d",
			transfer_operation, result);
	}
	free(converted_source);
	return result;
}

FBCALL int fb_GfxPut(void *target, float x, float y, unsigned char *source,
	int x1, int y1, int x2, int y2, int flags, int mode, PUTTER *putter,
	int alpha, BLENDER *blender, void *parameter)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *gpu_destination = NULL;
	FB_GFX3_SURFACE *gpu_source = NULL;
	FB_GFX3_IMAGE_VIEW source_view;
	FB_GFX3_IMAGE_VIEW destination_view;
	FB_GFX3_MODE *mode_state;
	FB_GFX3_RECT clip;
	unsigned char *source_pixels;
	unsigned char *destination_pixels;
	uint32_t width = 0;
	uint32_t height = 0;
	int destination_x = 0;
	int destination_y = 0;
	int result;
	int lhs;
	int rhs;
	int gpu_clipping;

	(void)putter;
	memset(&source_view, 0, sizeof(source_view));
	memset(&destination_view, 0, sizeof(destination_view));
	if ((mode < FB_GFX3_BLIT_TRANS) || (mode > FB_GFX3_BLIT_BLEND))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state == NULL) {
		FB_GRAPHICS_UNLOCK();
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	mode_state = state->mode;
	result = fb_gfx3_gpu_surface_lookup_locked(source, mode_state,
		&gpu_source);
	if ((result != FB_GFX3_OK) || (gpu_source == NULL)) {
		gpu_source = NULL;
		if (fb_gfx3_image_parse(source, &source_view) != FB_GFX3_OK) {
			result = FB_GFX3_INVALID;
			goto done;
		}
	} else if (mode == FB_GFX3_BLIT_CUSTOM) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	result = fb_gfx3_compat_flush_points_graphics_locked(state);
	if (result != FB_GFX3_OK) {
		FB_GRAPHICS_UNLOCK();
		return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
	}
	/*
		A previous POINT/PSET loop may own dirty shadow rows. PUT is a GPU
		ordering boundary, so those rows must reach the render target before its
		blit command is queued.
	*/
	result = fb_gfx3_compat_commit_shadow(state);
	if (result != FB_GFX3_OK) {
		FB_GRAPHICS_UNLOCK();
		return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
	}
	/*
		PUT is called under FB_GRAPHICS_LOCK, which is the public serialization
		boundary for draw state and active-mode replacement.  The old nested
		mode mutex made every sprite pay two additional kernel synchronization
		operations even though no independent public writer can run here.
	*/
	switch ((uint32_t)flags & FB_GFX3_COORDINATE_MASK) {
	case FB_GFX3_COORDINATE_RA:
		lhs = FB_GFX3_COORDINATE_R;
		rhs = FB_GFX3_COORDINATE_A;
		break;
	case FB_GFX3_COORDINATE_RR:
		lhs = FB_GFX3_COORDINATE_R;
		rhs = FB_GFX3_COORDINATE_R;
		break;
	case FB_GFX3_COORDINATE_AA:
		lhs = FB_GFX3_COORDINATE_A;
		rhs = FB_GFX3_COORDINATE_A;
		break;
	case FB_GFX3_COORDINATE_AR:
		lhs = FB_GFX3_COORDINATE_A;
		rhs = FB_GFX3_COORDINATE_R;
		break;
	default:
		result = FB_GFX3_INVALID;
		goto done;
	}
	image_api_fix_relative(state, lhs, &x, &y, NULL, NULL);
	if (rhs == FB_GFX3_COORDINATE_R) {
		x2 += x1;
		y2 += y1;
	}
	if (x1 == (int)0xFFFF0000u) {
		x1 = 0;
		y1 = 0;
		x2 = (int)((gpu_source != NULL) ? gpu_source->width :
			source_view.width) - 1;
		y2 = (int)((gpu_source != NULL) ? gpu_source->height :
			source_view.height) - 1;
	} else {
		uint32_t source_width = (gpu_source != NULL) ? gpu_source->width :
			source_view.width;
		uint32_t source_height = (gpu_source != NULL) ? gpu_source->height :
			source_view.height;

		image_api_order_coordinates(&x1, &y1, &x2, &y2);
		if (x1 < 0)
			x1 = 0;
		if (y1 < 0)
			y1 = 0;
		if (x2 >= (int)source_width)
			x2 = (int)source_width - 1;
		if (y2 >= (int)source_height)
			y2 = (int)source_height - 1;
	}
	if ((x2 < x1) || (y2 < y1)) {
		result = FB_GFX3_OK;
		goto done;
	}
	width = (uint32_t)(x2 - x1) + 1u;
	height = (uint32_t)(y2 - y1) + 1u;
	if (target == NULL) {
		result = image_api_translate_screen(state, x, y, &destination_x,
			&destination_y);
		clip = state->view;
	} else {
		result = fb_gfx3_gpu_surface_lookup_locked(target, mode_state,
			&gpu_destination);
		if ((result != FB_GFX3_OK) || (gpu_destination == NULL)) {
			gpu_destination = NULL;
			if (fb_gfx3_image_parse(target, &destination_view) !=
			    FB_GFX3_OK) {
				result = FB_GFX3_INVALID;
				goto done;
			}
		}
		if (!isfinite(x) || !isfinite(y) || ((double)x < INT_MIN) ||
		    ((double)x > INT_MAX) || ((double)y < INT_MIN) ||
		    ((double)y > INT_MAX)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		destination_x = CINT(x);
		destination_y = CINT(y);
		clip.x1 = 0;
		clip.y1 = 0;
		clip.x2 = (int32_t)((gpu_destination != NULL) ?
			gpu_destination->width : destination_view.width) - 1;
		clip.y2 = (int32_t)((gpu_destination != NULL) ?
			gpu_destination->height : destination_view.height) - 1;
		result = FB_GFX3_OK;
	}
	if (result != FB_GFX3_OK)
		goto done;
	/*
		Built-in drawing to a GPU target keeps the original source rectangle and
		destination. Raster backends use fixed-function clipping, while compute
		backends reduce dispatch bounds on the renderer thread and retain the clip
		for shader-side pixel decisions. This removes partial-sprite arithmetic
		from the BASIC producer without dispatching known offscreen pixels.

		RAM destinations and CUSTOM callbacks still require an exact CPU-visible
		rectangle before pointer arithmetic, download, or user code can run.
	*/
	gpu_clipping = ((target == NULL) || (gpu_destination != NULL)) &&
		(mode != FB_GFX3_BLIT_CUSTOM);
	if (!gpu_clipping) {
		image_api_clip_put(&destination_x, &destination_y, &x1, &y1,
			&width, &height, &clip);
		if ((width == 0) || (height == 0)) {
			result = FB_GFX3_OK;
			goto done;
		}
	}
	if ((gpu_source != NULL) && ((target == NULL) ||
	    (gpu_destination != NULL))) {
		FB_GFX3_SURFACE *destination_surface = (target == NULL) ?
			&mode_state->pages[state->work_page] : gpu_destination;
		FB_GFX3_RECT source_rect;

		source_rect.x1 = x1;
		source_rect.y1 = y1;
		source_rect.x2 = x1 + (int32_t)width - 1;
		source_rect.y2 = y1 + (int32_t)height - 1;
		result = fb_gfx3_surface_blit_graphics_locked(destination_surface, &clip,
			gpu_source,
			&source_rect, destination_x, destination_y, (uint32_t)mode,
			(uint32_t)alpha);
		if ((result == FB_GFX3_OK) && (target == NULL)) {
			if (mode_state->shadow_valid != NULL)
				mode_state->shadow_valid[state->work_page] = FALSE;
			if ((width > (uint32_t)INT_MAX) ||
			    (height > (uint32_t)INT_MAX) ||
			    ((int64_t)destination_x + width - 1 > INT_MAX) ||
			    ((int64_t)destination_y + height - 1 > INT_MAX))
				fb_gfx3_compat_invalidate_point_cache_graphics_locked(state);
			else
				fb_gfx3_compat_invalidate_point_cache_rect_graphics_locked(
					state, destination_x, destination_y,
					destination_x + (int)width - 1,
					destination_y + (int)height - 1);
		}
	} else if (gpu_source != NULL) {
		/* A raw FB.IMAGE destination requires a CPU-visible source. */
		result = FB_GFX3_UNSUPPORTED;
	} else if (target == NULL) {
		result = fb_gfx3_image_put_surface(
			&mode_state->pages[state->work_page], &state->view,
			&source_view, x1, y1, width, height, destination_x,
			destination_y, mode, alpha, blender, parameter);
		if (result == FB_GFX3_OK) {
			int mirror_result = image_api_mirror_screen_put_locked(state,
				&source_view, x1, y1, width, height, destination_x,
				destination_y, mode, alpha);

			fb_gfx3_log_write(&mode_state->context.logger,
				FB_GFX3_LOG_TRACE,
				"CPU image PUT shadow mirror: mode %d, size %ux%u, "
				"result %d", mode, width, height, mirror_result);
			if ((mirror_result != FB_GFX3_OK) &&
			    (mode_state->shadow_valid != NULL))
				mode_state->shadow_valid[state->work_page] = FALSE;
			if ((width > (uint32_t)INT_MAX) ||
			    (height > (uint32_t)INT_MAX) ||
			    ((int64_t)destination_x + width - 1 > INT_MAX) ||
			    ((int64_t)destination_y + height - 1 > INT_MAX))
				fb_gfx3_compat_invalidate_point_cache_graphics_locked(state);
			else
				fb_gfx3_compat_invalidate_point_cache_rect_graphics_locked(
					state, destination_x, destination_y,
					destination_x + (int)width - 1,
					destination_y + (int)height - 1);
		}
	} else if (gpu_destination != NULL) {
		result = fb_gfx3_image_put_surface(gpu_destination, &clip, &source_view,
			x1, y1, width, height, destination_x, destination_y, mode,
			alpha, blender, parameter);
	} else if (source_view.bytes_per_pixel !=
	    destination_view.bytes_per_pixel) {
		result = FB_GFX3_INVALID;
	} else {
		source_pixels = source_view.pixels +
			((size_t)y1 * source_view.pitch) +
			((size_t)x1 * source_view.bytes_per_pixel);
		destination_pixels = destination_view.pixels +
			((size_t)destination_y * destination_view.pitch) +
			((size_t)destination_x * destination_view.bytes_per_pixel);
		result = fb_gfx3_image_put_pixels(source_pixels,
			destination_pixels, width, height, source_view.pitch,
			destination_view.pitch, source_view.bytes_per_pixel, mode,
			alpha, blender, parameter);
		if (result == FB_GFX3_OK)
			fb_gfx3_image_cache_metadata_touch(&destination_view);
	}

done:
	if (result != FB_GFX3_OK) {
		fb_gfx3_log_write(&mode_state->context.logger, FB_GFX3_LOG_ERROR,
			"PUT failed: result=%d mode=%d target=%s destination=(%d,%d) "
			"source=(%d,%d) size=%ux%u source-bpp=%u screen-depth=%u flags=%d", result, mode,
			(target == NULL) ? "screen" : "image", destination_x,
			destination_y, x1, y1, (unsigned int)width,
			(unsigned int)height, (unsigned int)((gpu_source != NULL) ?
			((gpu_source->depth <= 8u) ? 1u :
			 ((gpu_source->depth == 16u) ? 2u : 4u)) :
			source_view.bytes_per_pixel),
			(unsigned int)mode_state->depth, flags);
	}
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

/* ------------------------------------------------------------------------- */
/* PUT callback ABI                                                          */
/* ------------------------------------------------------------------------- */

static _Thread_local uint32_t image_api_putter_bpp = 4;

static void image_api_putter(unsigned char *source,
	unsigned char *destination, int width, int height, int source_pitch,
	int destination_pitch, int alpha, BLENDER *blender, void *parameter,
	int mode)
{
	if ((width <= 0) || (height <= 0) || (source_pitch <= 0) ||
	    (destination_pitch <= 0))
		return;
	fb_gfx3_image_put_pixels(source, destination, (uint32_t)width,
		(uint32_t)height, (uint32_t)source_pitch,
		(uint32_t)destination_pitch, image_api_putter_bpp, mode, alpha,
		blender, parameter);
}

#define FB_GFX3_DEFINE_PUTTER(name, put_mode) \
	void name(unsigned char *source, unsigned char *destination, int width, \
		int height, int source_pitch, int destination_pitch, int alpha, \
		BLENDER *blender, void *parameter) \
	{ \
		image_api_putter(source, destination, width, height, source_pitch, \
			destination_pitch, alpha, blender, parameter, put_mode); \
	}

FB_GFX3_DEFINE_PUTTER(fb_hPutTrans, FB_GFX3_BLIT_TRANS)
FB_GFX3_DEFINE_PUTTER(fb_hPutPSet, FB_GFX3_BLIT_PSET)
FB_GFX3_DEFINE_PUTTER(fb_hPutPReset, FB_GFX3_BLIT_PRESET)
FB_GFX3_DEFINE_PUTTER(fb_hPutAnd, FB_GFX3_BLIT_AND)
FB_GFX3_DEFINE_PUTTER(fb_hPutOr, FB_GFX3_BLIT_OR)
FB_GFX3_DEFINE_PUTTER(fb_hPutXor, FB_GFX3_BLIT_XOR)
FB_GFX3_DEFINE_PUTTER(fb_hPutAlpha, FB_GFX3_BLIT_ALPHA)
FB_GFX3_DEFINE_PUTTER(fb_hPutAdd, FB_GFX3_BLIT_ADD)
FB_GFX3_DEFINE_PUTTER(fb_hPutBlend, FB_GFX3_BLIT_BLEND)
FB_GFX3_DEFINE_PUTTER(fb_hPutCustom, FB_GFX3_BLIT_CUSTOM)

#undef FB_GFX3_DEFINE_PUTTER

/* ------------------------------------------------------------------------- */
/* Row conversion                                                            */
/* ------------------------------------------------------------------------- */

FBCALL void fb_GfxImageConvertRow(const unsigned char *source,
	int source_depth, unsigned char *destination, int destination_depth,
	int width, int source_is_rgb)
{
	int x;
	uint32_t color;
	uint16_t color16;

	if ((source == NULL) || (destination == NULL) || (width <= 0))
		return;
	/*
		The native 32-bit layout is BGRA in memory and is the common row layout
		used by FB.IMAGE, raw BSAVE blocks, and BMP decode destinations. With
		source_is_rgb clear, the public conversion contract preserves those four
		bytes exactly. Use memmove so an accidental overlapping row remains a
		bounded copy rather than invoking undefined C library behaviour.
	*/
	if ((source_depth == 32) && (destination_depth == 32) &&
	    (source_is_rgb == 0)) {
		size_t byte_count;

		if (fb_gfx3_size_multiply((size_t)width, 4u, &byte_count) !=
		    FB_GFX3_OK)
			return;
		memmove(destination, source, byte_count);
		return;
	}
	for (x = 0; x < width; x++) {
		if (source_depth <= 8) {
			color = source[x];
		} else if (source_depth == 24) {
			const unsigned char *pixel = source + ((size_t)x * 3u);

			if (source_is_rgb)
				color = 0xFF000000u | ((uint32_t)pixel[0] << 16) |
					((uint32_t)pixel[1] << 8) | pixel[2];
			else
				color = 0xFF000000u | ((uint32_t)pixel[2] << 16) |
					((uint32_t)pixel[1] << 8) | pixel[0];
		} else if (source_depth == 32) {
			const unsigned char *pixel = source + ((size_t)x * 4u);

			if (source_is_rgb)
				color = ((uint32_t)pixel[3] << 24) |
					((uint32_t)pixel[0] << 16) |
					((uint32_t)pixel[1] << 8) | pixel[2];
			else
				color = ((uint32_t)pixel[3] << 24) |
					((uint32_t)pixel[2] << 16) |
					((uint32_t)pixel[1] << 8) | pixel[0];
		} else {
			return;
		}
		if (destination_depth <= 8) {
			destination[x] = (unsigned char)color;
		} else if (destination_depth == 15 || destination_depth == 16) {
			color16 = (uint16_t)fb_gfx3_image_fix_color(2, color);
			memcpy(destination + ((size_t)x * 2u), &color16,
				sizeof(color16));
		} else if (destination_depth == 24 || destination_depth == 32) {
			memcpy(destination + ((size_t)x * 4u), &color, sizeof(color));
		} else {
			return;
		}
	}
}

/* end of gfx3_image_api.c */
