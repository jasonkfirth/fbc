/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_paint_api.c

    Purpose:

        Implement border-based PAINT compatibility for CPU images and GPU
        screen pages.

    Responsibilities:

        - resolve PAINT coordinates and default colors
        - route bounded desktop screen fills to the renderer's GPU shader
        - perform bounded flood discovery without recursive C calls
        - support solid and 8 by 8 pattern fills
        - synchronize CPU-compatibility targets through one download and a
          tightly bounded dirty-rectangle upload

    This file intentionally does NOT contain:

        - a compute-shader flood-fill implementation
        - primitive line or ellipse drawing
        - image allocation or file codecs
*/

#include "gfx3_api_internal.h"
#include "gfx3_backend_gles.h"
#include "gfx3_backend_opengl.h"
#include "gfx3_backend_vulkan.h"
#include "gfx3_gpu_surface.h"
#include "gfx3_image.h"

#include <math.h>

enum {
	FB_GFX3_PAINT_FILL = 0,
	FB_GFX3_PAINT_PATTERN = 1,
	FB_GFX3_PAINT_PATTERN_MAX = 256,
	/* Keep this identical to the OpenGL compute queue safety limit. */
	FB_GFX3_PAINT_GPU_MAX_PIXELS = 1048576
};

/* ------------------------------------------------------------------------- */
/* Flood-fill core                                                           */
/* ------------------------------------------------------------------------- */

static uint32_t paint_pattern_color(const unsigned char *pattern,
	size_t pattern_size, uint32_t bytes_per_pixel, uint32_t x, uint32_t y)
{
	size_t offset;
	uint16_t value16;
	uint32_t value32;

	offset = ((((size_t)y & 7u) * 8u) + ((size_t)x & 7u)) *
		bytes_per_pixel;
	if ((pattern == NULL) || (offset > pattern_size) ||
	    (pattern_size - offset < bytes_per_pixel))
		return 0;
	if (bytes_per_pixel == 1)
		return pattern[offset];
	if (bytes_per_pixel == 2) {
		memcpy(&value16, pattern + offset, sizeof(value16));
		return value16;
	}
	memcpy(&value32, pattern + offset, sizeof(value32));
	return value32;
}

/*
	Solid opaque PAINT spans do not need the per-pixel bounds and alpha decision
	inside the generic image primitive helper. The flood core has already bounded
	the complete span, and this byte-wise form remains safe for unaligned image
	rows used by caller-owned FB.IMAGE buffers.
*/
static void paint_write_opaque_span(FB_GFX3_IMAGE_VIEW *view, uint32_t y,
	uint32_t left, uint32_t right, uint32_t color)
{
	unsigned char *destination;
	uint32_t count;
	size_t byte_count;
	size_t filled;
	size_t copy_count;

	if ((view == NULL) || (view->pixels == NULL) || (left > right) ||
	    (y >= view->height) || (right >= view->width))
		return;
	destination = view->pixels + ((size_t)y * view->pitch) +
		((size_t)left * view->bytes_per_pixel);
	count = right - left + 1u;
	if (view->bytes_per_pixel == 1u) {
		memset(destination, (unsigned char)color, count);
		return;
	}
	if ((view->bytes_per_pixel != 2u) && (view->bytes_per_pixel != 4u))
		return;
	if (fb_gfx3_size_multiply(count, view->bytes_per_pixel, &byte_count) !=
	    FB_GFX3_OK)
		return;
	/*
		Seed one exact native pixel, then double the initialized prefix. This
		keeps unaligned caller-owned rows safe and lets the C library move whole
		cache-line spans instead of issuing one tiny memcpy for every pixel.
	*/
	if (view->bytes_per_pixel == 2u) {
		uint16_t value = (uint16_t)color;

		memcpy(destination, &value, sizeof(value));
		filled = sizeof(value);
	} else {
		uint32_t value = color;

		memcpy(destination, &value, sizeof(value));
		filled = sizeof(value);
	}
	while (filled < byte_count) {
		copy_count = byte_count - filled;
		if (copy_count > filled)
			copy_count = filled;
		memcpy(destination + filled, destination, copy_count);
		filled += copy_count;
	}
}

static int paint_flood(FB_GFX3_IMAGE_VIEW *view,
	FB_GFX3_DRAW_STATE *state, int start_x, int start_y,
	uint32_t fill_color, uint32_t border_color,
	const unsigned char *pattern, size_t pattern_size, int paint_mode,
	uint32_t pattern_origin_x, uint32_t pattern_origin_y,
	FB_GFX3_RECT *dirty, unsigned char *reused_visited,
	size_t *reused_queue, size_t reused_capacity)
{
	unsigned char *visited;
	size_t *queue;
	size_t pixel_count;
	size_t queue_bytes;
	size_t head = 0;
	size_t tail = 0;
	size_t index;
	uint32_t x;
	uint32_t y;
	uint32_t left;
	uint32_t right;
	uint32_t scan_y;
	uint32_t scan_x;
	uint32_t neighbour;
	uint32_t color;
	int opaque_solid;

	if (dirty != NULL) {
		dirty->x1 = INT_MAX;
		dirty->y1 = INT_MAX;
		dirty->x2 = INT_MIN;
		dirty->y2 = INT_MIN;
	}
	if ((view == NULL) || (start_x < 0) || (start_y < 0) ||
	    ((uint32_t)start_x >= view->width) ||
	    ((uint32_t)start_y >= view->height))
		return FB_GFX3_OK;
	if (fb_gfx3_image_get_pixel_raw(view, start_x, start_y) == border_color)
		return FB_GFX3_OK;
	opaque_solid = (paint_mode == FB_GFX3_PAINT_FILL) &&
		((view->bytes_per_pixel == 1u) ||
		 (view->bytes_per_pixel == 2u) ||
		 (view->bytes_per_pixel == 4u)) &&
		((fb_gfx3_compat_primitive_flags(state,
		view->bytes_per_pixel * 8u, fill_color) &
		FB_GFX3_PRIMITIVE_ALPHA_BLEND) == 0u);
	if ((fb_gfx3_size_multiply(view->width, view->height,
	     &pixel_count) != FB_GFX3_OK) ||
	    (pixel_count == 0u) ||
	    (fb_gfx3_size_multiply(pixel_count, sizeof(queue[0]),
	     &queue_bytes) != FB_GFX3_OK) ||
	    (queue_bytes == 0u))
		return FB_GFX3_INVALID;
	if ((reused_visited != NULL) && (reused_queue != NULL) &&
	    (reused_capacity >= pixel_count)) {
		visited = reused_visited;
		queue = reused_queue;
		memset(visited, 0, pixel_count);
	} else {
		visited = (unsigned char *)calloc(pixel_count, 1);
		queue = (size_t *)malloc(queue_bytes);
	}
	if ((visited == NULL) || (queue == NULL)) {
		free(queue);
		free(visited);
		return FB_GFX3_OUT_OF_MEMORY;
	}
	index = ((size_t)(uint32_t)start_y * view->width) +
		(uint32_t)start_x;
	/*
		Visited values distinguish a queued span seed from a completed pixel.
		Marking an entire neighbouring run queued prevents another parent span
		from adding one FIFO entry per pixel before that run is processed.
	*/
	visited[index] = 1u;
	queue[tail++] = index;
	while (head < tail) {
		index = queue[head++];
		y = (uint32_t)(index / view->width);
		x = (uint32_t)(index - ((size_t)y * view->width));
		if ((visited[index] == 2u) ||
		    (fb_gfx3_image_get_pixel_raw(view, (int)x, (int)y) ==
		     border_color))
			continue;
		/*
			Find the complete unfilled run containing this seed. A queued pixel
			belongs to the same region and must be consumed here; a completed
			pixel is a boundary already expanded by another queue entry.
		*/
		left = x;
		while (left > 0u) {
			size_t candidate = ((size_t)y * view->width) + left - 1u;

			if ((visited[candidate] == 2u) ||
			    (fb_gfx3_image_get_pixel_raw(view, (int)left - 1,
			     (int)y) == border_color))
				break;
			left--;
		}
		right = x;
		while (right + 1u < view->width) {
			size_t candidate = ((size_t)y * view->width) + right + 1u;

			if ((visited[candidate] == 2u) ||
			    (fb_gfx3_image_get_pixel_raw(view, (int)right + 1,
			     (int)y) == border_color))
				break;
			right++;
		}
		/*
			A screen PAINT stages only the current VIEW rectangle.  Its local
			queue coordinates must not re-anchor the 8 by 8 pattern: gfxlib2
			tiles a pattern against the target's absolute pixel coordinates.
		*/
		if (opaque_solid)
			paint_write_opaque_span(view, y, left, right, fill_color);
		if (opaque_solid) {
			/* The span write has already covered every pixel in this interval. */
			memset(visited + ((size_t)y * view->width) + left, 2,
				right - left + 1u);
		} else {
			for (scan_x = left; scan_x <= right; scan_x++) {
				index = ((size_t)y * view->width) + scan_x;
				color = (paint_mode == FB_GFX3_PAINT_PATTERN) ?
					paint_pattern_color(pattern, pattern_size,
						view->bytes_per_pixel, scan_x + pattern_origin_x,
						y + pattern_origin_y) : fill_color;
				if (paint_mode == FB_GFX3_PAINT_PATTERN)
					fb_gfx3_image_set_pixel_raw(view, (int)scan_x, (int)y, color);
				else
					fb_gfx3_image_set_primitive_pixel(view, state, (int)scan_x,
						(int)y, color);
				visited[index] = 2u;
			}
		}
		if (dirty != NULL) {
			if ((int)left < dirty->x1)
				dirty->x1 = (int)left;
			if ((int)y < dirty->y1)
				dirty->y1 = (int)y;
			if ((int)right > dirty->x2)
				dirty->x2 = (int)right;
			if ((int)y > dirty->y2)
				dirty->y2 = (int)y;
		}
		for (neighbour = 0u; neighbour < 2u; neighbour++) {
			if (neighbour == 0u) {
				if (y == 0u)
					continue;
				scan_y = y - 1u;
			} else {
				if (y + 1u >= view->height)
					continue;
				scan_y = y + 1u;
			}
			if (scan_y == y)
				continue;
			for (scan_x = left; scan_x <= right; scan_x++) {
				size_t candidate = ((size_t)scan_y * view->width) + scan_x;

				if ((visited[candidate] != 0u) ||
				    (fb_gfx3_image_get_pixel_raw(view, (int)scan_x,
				     (int)scan_y) == border_color))
					continue;
				if (tail >= pixel_count) {
					if (visited != reused_visited) {
						free(queue);
						free(visited);
					}
					return FB_GFX3_INVALID;
				}
				queue[tail++] = candidate;
				/* One queued seed represents this whole adjacent unfilled run. */
				do {
					visited[candidate] = 1u;
					scan_x++;
					if (scan_x > right)
						break;
					candidate = ((size_t)scan_y * view->width) + scan_x;
				} while ((visited[candidate] == 0u) &&
					(fb_gfx3_image_get_pixel_raw(view, (int)scan_x,
					 (int)scan_y) != border_color));
				/* The for loop supplies the next candidate after this run. */
				if (scan_x <= right)
					scan_x--;
			}
		}
	}
	if (visited != reused_visited) {
		free(queue);
		free(visited);
	}
	return FB_GFX3_OK;
}

static int paint_screen_scratch_ensure(FB_GFX3_MODE *mode,
	size_t pixel_count)
{
	unsigned char *visited;
	size_t *queue;
	size_t queue_bytes;

	if ((mode == NULL) || (pixel_count == 0u) ||
	    (fb_gfx3_size_multiply(pixel_count, sizeof(queue[0]),
	     &queue_bytes) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	if ((mode->paint_visited != NULL) && (mode->paint_queue != NULL) &&
	    (mode->paint_scratch_capacity >= pixel_count))
		return FB_GFX3_OK;
	visited = (unsigned char *)malloc(pixel_count);
	queue = (size_t *)malloc(queue_bytes);
	if ((visited == NULL) || (queue == NULL)) {
		free(queue);
		free(visited);
		return FB_GFX3_OUT_OF_MEMORY;
	}
	free(mode->paint_queue);
	free(mode->paint_visited);
	mode->paint_visited = visited;
	mode->paint_queue = queue;
	mode->paint_scratch_capacity = pixel_count;
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Target adapters                                                           */
/* ------------------------------------------------------------------------- */

static void paint_fix_image_relative(FB_GFX3_DRAW_STATE *state,
	uint32_t flags, float *x, float *y)
{
	if ((state == NULL) || (x == NULL) || (y == NULL))
		return;
	if ((flags & FB_GFX3_COORDINATE_MASK) == FB_GFX3_COORDINATE_R) {
		*x += state->last_x;
		*y += state->last_y;
	}
	state->last_x = *x;
	state->last_y = *y;
}

static int paint_gpu_compute_available(const FB_GFX3_SURFACE *surface,
	uint32_t width, uint32_t height, int paint_mode)
{
	size_t pixel_count;

	if ((surface == NULL) || ((paint_mode != FB_GFX3_PAINT_FILL) &&
	    (paint_mode != FB_GFX3_PAINT_PATTERN)) ||
	    (surface->context == NULL))
		return FALSE;
	if (fb_gfx3_size_multiply(width, height, &pixel_count) != FB_GFX3_OK)
		return FALSE;
	if (surface->context->renderer.backend_vtable ==
	    &__fb_gfx3_backend_gles) {
		/*
			ES 3.0 has no compute or image-store primitive. Its exact frontier
			shader needs one full texture pass per Manhattan-distance expansion.
			For a normal page a single transfer round trip plus the bounded CPU
			flood is substantially faster, while GPU-only surfaces without an
			upload capability must retain the all-GPU frontier implementation.
		*/
		return (pixel_count != 0) &&
			((surface->usage & FB_GFX3_SURFACE_TRANSFER_DESTINATION) == 0);
	}
	/*
		The desktop backends use one exact scanline-queue compute dispatch. One
		workgroup discovers four-neighbour spans while 256 shader lanes mark and
		write each span. Screen pages must use this path too: downloading a page,
		flooding it on the application thread, and uploading it again defeats both
		GPU residency and producer offloading. The pixel bound limits scratch
		storage and the duration of a deliberately irregular flood.
	*/
	if (paint_mode == FB_GFX3_PAINT_PATTERN)
		return ((surface->context->renderer.backend_vtable ==
			 &__fb_gfx3_backend_opengl) ||
			(surface->context->renderer.backend_vtable ==
			 &__fb_gfx3_backend_vulkan)) &&
			pixel_count <= FB_GFX3_PAINT_GPU_MAX_PIXELS;
	if ((surface->context->renderer.backend_vtable !=
	     &__fb_gfx3_backend_opengl) &&
	    (surface->context->renderer.backend_vtable !=
	     &__fb_gfx3_backend_vulkan))
		return FALSE;
	return pixel_count <= FB_GFX3_PAINT_GPU_MAX_PIXELS;
}

static int paint_image(FB_GFX3_IMAGE_VIEW *view,
	FB_GFX3_DRAW_STATE *state, float x, float y, uint32_t color,
	uint32_t border_color, const unsigned char *pattern,
	size_t pattern_size, int paint_mode, uint32_t flags)
{
	paint_fix_image_relative(state, flags, &x, &y);
	if (!isfinite(x) || !isfinite(y) ||
	    ((double)x < INT_MIN) || ((double)x > INT_MAX) ||
	    ((double)y < INT_MIN) || ((double)y > INT_MAX))
		return FB_GFX3_INVALID;
	if (flags & FB_GFX3_DEFAULT_COLOR_1)
		color = state->foreground_color;
	else
		color = fb_gfx3_image_fix_color(view->bytes_per_pixel, color);
	if (flags & FB_GFX3_DEFAULT_COLOR_2)
		border_color = color;
	else
		border_color = fb_gfx3_image_fix_color(view->bytes_per_pixel,
			border_color);
	return paint_flood(view, state, CINT(x), CINT(y), color, border_color,
		pattern, pattern_size, paint_mode, 0, 0, NULL, NULL, NULL, 0u);
}

/*
	The public upload API deliberately copies pitch times height bytes so it can
	be fully asynchronous once the call returns.  A dirty rectangle beginning
	in the middle of a downloaded row cannot safely borrow the original pitch:
	the final copied row would extend beyond the allocation.  Pack the changed
	rows first, both to retain that ownership contract and to avoid uploading
	the untouched part of a PAINT target.
*/
static int paint_upload_dirty(FB_GFX3_SURFACE *surface, int origin_x,
	int origin_y, const FB_GFX3_IMAGE_VIEW *view, const FB_GFX3_RECT *dirty)
{
	unsigned char *packed;
	const unsigned char *source_row;
	size_t row_bytes;
	size_t allocation_size;
	uint32_t width;
	uint32_t height;
	uint32_t row;
	int result;

	if ((surface == NULL) || (view == NULL) || (dirty == NULL) ||
	    (dirty->x1 > dirty->x2) || (dirty->y1 > dirty->y2) ||
	    (dirty->x1 < 0) || (dirty->y1 < 0) ||
	    (view->bytes_per_pixel == 0u) ||
	    ((uint32_t)dirty->x2 >= view->width) ||
	    ((uint32_t)dirty->y2 >= view->height))
		return FB_GFX3_INVALID;
	width = (uint32_t)(dirty->x2 - dirty->x1 + 1);
	height = (uint32_t)(dirty->y2 - dirty->y1 + 1);
	if ((fb_gfx3_size_multiply(width, view->bytes_per_pixel, &row_bytes) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(row_bytes, height, &allocation_size) !=
	     FB_GFX3_OK) || (row_bytes == 0u) || (allocation_size == 0u))
		return FB_GFX3_INVALID;
	packed = (unsigned char *)malloc(allocation_size);
	if (packed == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	for (row = 0; row < height; row++) {
		source_row = view->pixels +
			((size_t)(dirty->y1 + (int)row) * view->pitch) +
			((size_t)dirty->x1 * view->bytes_per_pixel);
		memcpy(packed + ((size_t)row * row_bytes), source_row, row_bytes);
	}
	result = fb_gfx3_surface_upload(surface, origin_x + dirty->x1,
		origin_y + dirty->y1, width, height, (uint32_t)row_bytes, packed);
	free(packed);
	return result;
}

static int paint_screen(FB_GFX3_DRAW_STATE *state, float x, float y,
	uint32_t color, uint32_t border_color, const unsigned char *pattern,
	size_t pattern_size, int paint_mode, uint32_t flags)
{
	FB_GFX3_IMAGE_VIEW view;
	FB_GFX3_MODE *mode;
	unsigned char *pixels;
	size_t allocation_size;
	uint32_t bytes_per_pixel;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	FB_GFX3_RECT dirty;
	int translated_x;
	int translated_y;
	int use_page_shadow = FALSE;
	int use_gpu_compute;
	int result;

	result = fb_gfx3_compat_resolve_point(state, &x, &y, flags,
		&translated_x, &translated_y);
	if (result != FB_GFX3_OK)
		return result;
	mode = state->mode;
	if ((mode == NULL) || (mode->mutex == NULL))
		return FB_GFX3_INVALID;
	if ((state->view.x1 > state->view.x2) ||
	    (state->view.y1 > state->view.y2))
		return FB_GFX3_INVALID;
	width = (uint32_t)(state->view.x2 - state->view.x1 + 1);
	height = (uint32_t)(state->view.y2 - state->view.y1 + 1);
	bytes_per_pixel = (mode->depth + 7u) / 8u;
	if ((width == 0u) || (height == 0u) || (bytes_per_pixel == 0u) ||
	    (width > UINT32_MAX / bytes_per_pixel) ||
	    (fb_gfx3_size_multiply(width * bytes_per_pixel, height,
	     &allocation_size) != FB_GFX3_OK) || (allocation_size == 0u))
		return FB_GFX3_INVALID;
	pitch = width * bytes_per_pixel;
	fb_MutexLock(mode->mutex);
	if (!mode->initialized || (state->generation != mode->generation) ||
	    (state->work_page >= mode->page_count)) {
		fb_MutexUnlock(mode->mutex);
		return FB_GFX3_INVALID;
	}
	if (flags & FB_GFX3_DEFAULT_COLOR_1)
		color = state->foreground_color;
	else
		color = fb_gfx3_image_fix_color(bytes_per_pixel, color);
	if (flags & FB_GFX3_DEFAULT_COLOR_2)
		border_color = color;
	else
		border_color = fb_gfx3_image_fix_color(bytes_per_pixel, border_color);
	use_gpu_compute = paint_gpu_compute_available(
		&mode->pages[state->work_page], width, height, paint_mode);
	if (use_gpu_compute) {
		/*
			A GPU-only PAINT must observe a preceding SCREENLOCK edit. Normal
			screen pages deliberately skip this upload and retain their CPU shadow
			across adjacent PAINT calls until a later GPU consumer needs it.
		*/
		fb_MutexUnlock(mode->mutex);
		result = fb_gfx3_compat_commit_shadow(state);
		if (result != FB_GFX3_OK)
			return result;
		fb_MutexLock(mode->mutex);
		if (!mode->initialized || (state->generation != mode->generation) ||
		    (state->work_page >= mode->page_count)) {
			fb_MutexUnlock(mode->mutex);
			return FB_GFX3_INVALID;
		}
		result = fb_gfx3_surface_paint(&mode->pages[state->work_page],
			&state->view, translated_x, translated_y, color, border_color,
			fb_gfx3_compat_primitive_flags(state, mode->depth, color),
			(uint32_t)paint_mode, pattern, pattern_size,
			(uint32_t)state->view.x1, (uint32_t)state->view.y1);
		if (result == FB_GFX3_OK) {
			if (mode->shadow_valid != NULL)
				mode->shadow_valid[state->work_page] = FALSE;
			fb_MutexUnlock(mode->mutex);
			return FB_GFX3_OK;
		}
	}
	/*
		Repeated PAINT commands commonly recolor the same full screen region.
		Keep a coherent CPU copy for that exact case after the first download.
		Every changed rectangle is still uploaded before this function returns,
		and every normal GPU draw invalidates the copy, so it cannot hide writes
		from a later BASIC command.  VIEW-relative PAINT deliberately retains
		the compact temporary path below because the mode shadow is page-sized.
	*/
	if ((mode->depth == 32) && (state->view.x1 == 0) &&
	    (state->view.y1 == 0) && (width == mode->width) &&
	    (height == mode->height) && (mode->shadow_pages != NULL) &&
	    (mode->shadow_valid != NULL) && (mode->shadow_dirty != NULL)) {
		if (mode->shadow_pages[state->work_page] == NULL)
			mode->shadow_pages[state->work_page] =
				(unsigned char *)malloc(allocation_size);
		if (mode->shadow_pages[state->work_page] != NULL) {
			mode->shadow_pitch = pitch;
			pixels = mode->shadow_pages[state->work_page];
			use_page_shadow = TRUE;
		} else
			pixels = NULL;
	} else
		pixels = (unsigned char *)malloc(allocation_size);
	if (pixels == NULL) {
		fb_MutexUnlock(mode->mutex);
		return FB_GFX3_OUT_OF_MEMORY;
	}
	if (use_page_shadow) {
		result = paint_screen_scratch_ensure(mode, (size_t)width * height);
		if (result != FB_GFX3_OK) {
			fb_MutexUnlock(mode->mutex);
			return result;
		}
	}
	if (use_page_shadow && mode->shadow_valid[state->work_page])
		result = FB_GFX3_OK;
	else
		result = fb_gfx3_surface_download(&mode->pages[state->work_page],
			state->view.x1, state->view.y1, width, height, pitch, pixels);
	if (result == FB_GFX3_OK) {
		memset(&view, 0, sizeof(view));
		view.pixels = pixels;
		view.width = width;
		view.height = height;
		view.pitch = pitch;
		view.bytes_per_pixel = bytes_per_pixel;
		result = paint_flood(&view, state,
			translated_x - state->view.x1,
			translated_y - state->view.y1, color, border_color,
			pattern, pattern_size, paint_mode,
			(uint32_t)state->view.x1, (uint32_t)state->view.y1, &dirty,
			use_page_shadow ? mode->paint_visited : NULL,
			use_page_shadow ? mode->paint_queue : NULL,
			use_page_shadow ? mode->paint_scratch_capacity : 0u);
	}
	if ((result == FB_GFX3_OK) && (dirty.x1 <= dirty.x2)) {
		if (use_page_shadow) {
			/*
				The page shadow is now newer than the GPU. Do not upload each
				consecutive PAINT independently: POINT, SCREENSYNC, page changes,
				or the next GPU primitive call the shared ordered commit helper.
			*/
			mode->shadow_dirty[state->work_page] = TRUE;
		} else {
			result = paint_upload_dirty(&mode->pages[state->work_page],
				state->view.x1, state->view.y1, &view, &dirty);
		}
	}
	if ((result == FB_GFX3_OK) && (mode->shadow_valid != NULL))
		mode->shadow_valid[state->work_page] =
			(unsigned char)use_page_shadow;
	fb_MutexUnlock(mode->mutex);
	if (!use_page_shadow)
		free(pixels);
	return result;
}

static int paint_gpu_surface(FB_GFX3_SURFACE *surface,
	FB_GFX3_DRAW_STATE *state, float x, float y, uint32_t color,
	uint32_t border_color, const unsigned char *pattern,
	size_t pattern_size, int paint_mode, uint32_t flags)
{
	FB_GFX3_IMAGE_VIEW view;
	unsigned char *pixels;
	size_t allocation_size;
	uint32_t bytes_per_pixel;
	uint32_t pitch;
	/* An inverted rectangle means that no upload is required. */
	FB_GFX3_RECT dirty = { 0, 0, -1, -1 };
	int result;

	/*
		PAINT needs a readback/upload compatibility barrier internally, but
		it is still a drawing statement.  Transfer capability alone must not
		let it modify a surface deliberately created without render-target use.
	*/
	if ((surface == NULL) || (state == NULL) || (surface->width == 0u) ||
	    (surface->height == 0u))
		return FB_GFX3_INVALID;
	if ((surface->usage & FB_GFX3_SURFACE_RENDER_TARGET) == 0)
		return FB_GFX3_UNSUPPORTED;
	bytes_per_pixel = (surface->depth + 7u) / 8u;
	if ((bytes_per_pixel == 0u) ||
	    (surface->width > UINT32_MAX / bytes_per_pixel) ||
	    (fb_gfx3_size_multiply(surface->width * bytes_per_pixel,
	     surface->height, &allocation_size) != FB_GFX3_OK) ||
	    (allocation_size == 0u))
		return FB_GFX3_INVALID;
	pitch = surface->width * bytes_per_pixel;
	paint_fix_image_relative(state, flags, &x, &y);
	if (!isfinite(x) || !isfinite(y) ||
	    ((double)x < INT_MIN) || ((double)x > INT_MAX) ||
	    ((double)y < INT_MIN) || ((double)y > INT_MAX))
		return FB_GFX3_INVALID;
	if (flags & FB_GFX3_DEFAULT_COLOR_1)
		color = state->foreground_color;
	else
		color = fb_gfx3_image_fix_color(bytes_per_pixel, color);
	if (flags & FB_GFX3_DEFAULT_COLOR_2)
		border_color = color;
	else
		border_color = fb_gfx3_image_fix_color(bytes_per_pixel, border_color);
	if (paint_gpu_compute_available(surface, surface->width, surface->height,
	    paint_mode))
		return fb_gfx3_surface_paint(surface,
			&(FB_GFX3_RECT){ 0, 0, (int32_t)surface->width - 1,
				(int32_t)surface->height - 1 }, CINT(x), CINT(y), color,
			border_color, fb_gfx3_compat_primitive_flags(state,
				surface->depth, color), (uint32_t)paint_mode, pattern,
			pattern_size, 0, 0);
	pixels = (unsigned char *)malloc(allocation_size);
	if (pixels == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	result = fb_gfx3_surface_download(surface, 0, 0, surface->width,
		surface->height, pitch, pixels);
	if (result == FB_GFX3_OK) {
		memset(&view, 0, sizeof(view));
		view.pixels = pixels;
		view.width = surface->width;
		view.height = surface->height;
		view.pitch = pitch;
		view.bytes_per_pixel = bytes_per_pixel;
		result = paint_flood(&view, state, CINT(x), CINT(y), color,
			border_color, pattern, pattern_size, paint_mode, 0, 0, &dirty,
			NULL, NULL, 0u);
	}
	if ((result == FB_GFX3_OK) && (dirty.x1 <= dirty.x2))
		result = paint_upload_dirty(surface, 0, 0, &view, &dirty);
	free(pixels);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Public PAINT ABI                                                          */
/* ------------------------------------------------------------------------- */

FBCALL void fb_GfxPaint(void *target, float x, float y, unsigned int color,
	unsigned int border_color, FBSTRING *pattern, int paint_mode, int flags)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *gpu_surface;
	FB_GFX3_IMAGE_VIEW image;
	unsigned char pattern_data[FB_GFX3_PAINT_PATTERN_MAX];
	size_t pattern_size = 0;
	ssize_t string_size;
	int result = FB_GFX3_INVALID;

	memset(pattern_data, 0, sizeof(pattern_data));
	if ((paint_mode == FB_GFX3_PAINT_PATTERN) && (pattern != NULL) &&
	    (pattern->data != NULL)) {
		string_size = FB_STRSIZE(pattern);
		if (string_size > 0) {
			pattern_size = ((size_t)string_size < sizeof(pattern_data)) ?
				(size_t)string_size : sizeof(pattern_data);
			memcpy(pattern_data, pattern->data, pattern_size);
		}
	}
	if (pattern != NULL)
		fb_hStrDelTemp(pattern);
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state != NULL) {
		/*
			PAINT reads the current destination before it writes.  Small opaque
			boxes may still be condensed in the caller's PSET batch, so submit
			them first or a following readback can observe an older surface.
		*/
		result = fb_gfx3_compat_flush_points(state);
		if (result != FB_GFX3_OK) {
			FB_GRAPHICS_UNLOCK();
			fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
			return;
		}
		if (target == NULL) {
			result = paint_screen(state, x, y, color, border_color,
				pattern_data, pattern_size, paint_mode,
				(uint32_t)flags);
			if (result == FB_GFX3_OK)
				fb_gfx3_compat_invalidate_point_cache_graphics_locked(state);
		}
		else if (fb_gfx3_gpu_surface_lookup_locked(target, state->mode,
		    &gpu_surface) == FB_GFX3_OK)
			result = paint_gpu_surface(gpu_surface, state, x, y, color,
				border_color, pattern_data, pattern_size, paint_mode,
				(uint32_t)flags);
		else if (fb_gfx3_image_parse(target, &image) == FB_GFX3_OK) {
			result = paint_image(&image, state, x, y, color, border_color,
				pattern_data, pattern_size, paint_mode,
				(uint32_t)flags);
			if (result == FB_GFX3_OK)
				fb_gfx3_image_cache_metadata_touch(&image);
		}
	}
	FB_GRAPHICS_UNLOCK();
	fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

/* end of gfx3_paint_api.c */
