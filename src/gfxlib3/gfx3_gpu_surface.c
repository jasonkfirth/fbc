/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_gpu_surface.c

    Purpose:

        Implement opaque GPU-resident surfaces for FreeBASIC programs and
        make them usable as ordinary graphics primitive targets.

    Responsibilities:

        - allocate and track mode-owned opaque surface descriptors
        - load assets and expose checked surface lifecycle and transfer calls
        - submit scaling, rotation, and Mode 7 as projective GPU commands
        - preserve image-target coordinate and color behavior for primitives
        - reject stale, foreign, and already destroyed descriptors safely

    This file intentionally does NOT contain:

        - CPU-visible FB.IMAGE allocation or writable pixel pointers
		- shader code or backend resource layouts
		- screen page ownership and presentation policy
*/

#include "gfx3_api_internal.h"
#include "gfx3_file_api.h"
#include "gfx3_gpu_surface.h"
#include "gfx3_image.h"

#include <math.h>

typedef struct FB_GFX3_GPU_SURFACE_NODE {
	struct FB_GFX3_GPU_SURFACE_NODE *next;
	FB_GFX3_MODE *mode;
	uint64_t generation;
	FB_GFX3_SURFACE surface;
	unsigned char *mapped_pixels;
	size_t mapped_size;
	uint32_t mapped_pitch;
	uint32_t mapped_access;
	int32_t mapped_x;
	int32_t mapped_y;
	uint32_t mapped_width;
	uint32_t mapped_height;
} FB_GFX3_GPU_SURFACE_NODE;

static FB_GFX3_GPU_SURFACE_NODE *gpu_surface_list;

#define FB_GFX3_MAP_READ 0x00000001u
#define FB_GFX3_MAP_WRITE 0x00000002u

typedef struct FB_GFX3_PUBLIC_POINT {
	int32_t x;
	int32_t y;
	uint32_t color;
	uint32_t alpha;
} FB_GFX3_PUBLIC_POINT;

/* ------------------------------------------------------------------------- */
/* Descriptor registry                                                       */
/* ------------------------------------------------------------------------- */

static FB_GFX3_GPU_SURFACE_NODE *gpu_surface_find_locked(void *target,
	FB_GFX3_MODE *mode, FB_GFX3_GPU_SURFACE_NODE ***previous_next)
{
	FB_GFX3_GPU_SURFACE_NODE **link = &gpu_surface_list;

	while (*link != NULL) {
		FB_GFX3_GPU_SURFACE_NODE *node = *link;

		if ((void *)node == target) {
			if ((mode == NULL) || (node->mode != mode) ||
			    (node->generation != mode->generation))
				return NULL;
			if (previous_next != NULL)
				*previous_next = link;
			return node;
		}
		link = &node->next;
	}
	return NULL;
}

static void gpu_surface_release_map(FB_GFX3_GPU_SURFACE_NODE *node)
{
	if (node == NULL)
		return;
	free(node->mapped_pixels);
	node->mapped_pixels = NULL;
	node->mapped_size = 0;
	node->mapped_pitch = 0;
	node->mapped_access = 0;
	node->mapped_x = 0;
	node->mapped_y = 0;
	node->mapped_width = 0;
	node->mapped_height = 0;
}

static uint32_t gpu_surface_bytes_per_pixel(uint32_t depth)
{
	if (depth <= 8)
		return 1;
	if (depth == 16)
		return 2;
	return 4;
}

static int gpu_surface_usage_valid(unsigned int usage)
{
	const uint32_t known_usage = FB_GFX3_SURFACE_RENDER_TARGET |
		FB_GFX3_SURFACE_SAMPLED | FB_GFX3_SURFACE_TRANSFER_SOURCE |
		FB_GFX3_SURFACE_TRANSFER_DESTINATION;

	return (usage & ~known_usage) == 0u;
}

int fb_gfx3_gpu_surface_lookup_locked(void *target, FB_GFX3_MODE *mode,
	FB_GFX3_SURFACE **surface)
{
	FB_GFX3_GPU_SURFACE_NODE *node;

	if ((target == NULL) || (mode == NULL) || (surface == NULL))
		return FB_GFX3_INVALID;
	node = gpu_surface_find_locked(target, mode, NULL);
	if ((node == NULL) || (node->mapped_pixels != NULL))
		return FB_GFX3_INVALID;
	*surface = &node->surface;
	return FB_GFX3_OK;
}

void fb_gfx3_gpu_surfaces_destroy_all_locked(FB_GFX3_MODE *mode)
{
	FB_GFX3_GPU_SURFACE_NODE **link = &gpu_surface_list;

	while (*link != NULL) {
		FB_GFX3_GPU_SURFACE_NODE *node = *link;

		if (node->mode != mode) {
			link = &node->next;
			continue;
		}
		*link = node->next;
		gpu_surface_release_map(node);
		fb_gfx3_surface_destroy(&node->surface);
		free(node);
	}
}

/* ------------------------------------------------------------------------- */
/* Public extension lifecycle and transfers                                  */
/* ------------------------------------------------------------------------- */

FBCALL void *fb_Gfx3SurfaceCreate(int width, int height, int depth,
	unsigned int usage, unsigned int clear_color)
{
	FB_GFX3_GPU_SURFACE_NODE *node = NULL;
	FB_GFX3_DRAW_STATE *state;
	uint32_t known_usage = FB_GFX3_SURFACE_RENDER_TARGET |
		FB_GFX3_SURFACE_SAMPLED | FB_GFX3_SURFACE_TRANSFER_SOURCE |
		FB_GFX3_SURFACE_TRANSFER_DESTINATION;
	int result = FB_GFX3_INVALID;

	if ((width <= 0) || (height <= 0) || !gpu_surface_usage_valid(usage))
		goto done;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state == NULL)
		goto unlock;
	if (depth <= 0)
		depth = (int)state->mode->depth;
	if (!((depth == 1) || (depth == 2) || (depth == 4) ||
	      (depth == 8) || (depth == 16) || (depth == 32)))
		goto unlock;
	if (usage == 0)
		usage = known_usage;
	node = (FB_GFX3_GPU_SURFACE_NODE *)calloc(1, sizeof(*node));
	if (node == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto unlock;
	}
	result = fb_gfx3_surface_create(&state->mode->context, &node->surface,
		(uint32_t)width, (uint32_t)height, (uint32_t)depth, usage,
		clear_color);
	if (result != FB_GFX3_OK) {
		free(node);
		node = NULL;
		goto unlock;
	}
	node->mode = state->mode;
	node->generation = state->mode->generation;
	node->next = gpu_surface_list;
	gpu_surface_list = node;

unlock:
	FB_GRAPHICS_UNLOCK();
done:
	fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
	return node;
}

FBCALL void *fb_Gfx3SurfaceLoad(FBSTRING *filename, int depth,
	unsigned int usage)
{
	FB_GFX3_GPU_SURFACE_NODE *node = NULL;
	FB_GFX3_DRAW_STATE *state;
	unsigned char *pixels = NULL;
	uint32_t width = 0u;
	uint32_t height = 0u;
	uint32_t pitch = 0u;
	int result = FB_GFX3_INVALID;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state == NULL) {
		if (filename != NULL)
			fb_hStrDelTemp(filename);
		goto unlock;
	}
	if (depth <= 0)
		depth = (int)state->mode->depth;
	if (usage == 0u)
		usage = FB_GFX3_SURFACE_SAMPLED |
			FB_GFX3_SURFACE_TRANSFER_DESTINATION;
	if (!((depth == 8) || (depth == 16) || (depth == 32)) ||
	    !gpu_surface_usage_valid(usage) ||
	    ((usage & (FB_GFX3_SURFACE_SAMPLED |
	      FB_GFX3_SURFACE_TRANSFER_DESTINATION)) !=
	     (FB_GFX3_SURFACE_SAMPLED |
	      FB_GFX3_SURFACE_TRANSFER_DESTINATION))) {
		if (filename != NULL)
			fb_hStrDelTemp(filename);
		goto unlock;
	}
	result = fb_gfx3_file_load_bitmap_pixels_locked(filename,
		(uint32_t)depth, &pixels, &width, &height, &pitch);
	if (result != FB_GFX3_OK)
		goto unlock;
	node = (FB_GFX3_GPU_SURFACE_NODE *)calloc(1u, sizeof(*node));
	if (node == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto unlock;
	}
	result = fb_gfx3_surface_create(&state->mode->context, &node->surface,
		width, height, (uint32_t)depth, usage, 0u);
	if (result == FB_GFX3_OK)
		result = fb_gfx3_surface_upload(&node->surface, 0, 0, width,
			height, pitch, pixels);
	if (result != FB_GFX3_OK) {
		if (node->surface.handle != 0)
			fb_gfx3_surface_destroy(&node->surface);
		free(node);
		node = NULL;
		goto unlock;
	}
	node->mode = state->mode;
	node->generation = state->mode->generation;
	node->next = gpu_surface_list;
	gpu_surface_list = node;

unlock:
	free(pixels);
	FB_GRAPHICS_UNLOCK();
	fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
	return node;
}

FBCALL int fb_Gfx3SurfaceDestroy(void *surface_pointer)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_GPU_SURFACE_NODE *node;
	FB_GFX3_GPU_SURFACE_NODE **link = NULL;
	int result = FB_GFX3_INVALID;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state != NULL) {
		node = gpu_surface_find_locked(surface_pointer, state->mode, &link);
		if ((node != NULL) && (link != NULL) &&
		    (node->mapped_pixels == NULL)) {
			result = fb_gfx3_surface_destroy(&node->surface);
			if (result == FB_GFX3_OK) {
				*link = node->next;
				free(node);
			}
		}
	}
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

FBCALL int fb_Gfx3SurfaceInfo(void *surface_pointer, int *width, int *height,
	int *depth, unsigned int *usage)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *surface;
	int result = FB_GFX3_INVALID;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state != NULL) &&
	    (fb_gfx3_gpu_surface_lookup_locked(surface_pointer, state->mode,
	     &surface) == FB_GFX3_OK)) {
		if (width != NULL)
			*width = (int)surface->width;
		if (height != NULL)
			*height = (int)surface->height;
		if (depth != NULL)
			*depth = (int)surface->depth;
		if (usage != NULL)
			*usage = surface->usage;
		result = FB_GFX3_OK;
	}
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

FBCALL int fb_Gfx3SurfaceUpload(void *surface_pointer, int x, int y,
	int width, int height, int pitch, const void *pixels)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *surface;
	int result = FB_GFX3_INVALID;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state != NULL) && (width > 0) && (height > 0) && (pitch > 0) &&
	    (fb_gfx3_gpu_surface_lookup_locked(surface_pointer, state->mode,
	     &surface) == FB_GFX3_OK))
		result = fb_gfx3_surface_upload(surface, x, y, (uint32_t)width,
			(uint32_t)height, (uint32_t)pitch, pixels);
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

FBCALL int fb_Gfx3SurfaceDownload(void *surface_pointer, int x, int y,
	int width, int height, int pitch, void *pixels)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *surface;
	int result = FB_GFX3_INVALID;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state != NULL) && (width > 0) && (height > 0) && (pitch > 0) &&
	    (fb_gfx3_gpu_surface_lookup_locked(surface_pointer, state->mode,
	     &surface) == FB_GFX3_OK))
		result = fb_gfx3_surface_download(surface, x, y, (uint32_t)width,
			(uint32_t)height, (uint32_t)pitch, pixels);
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

static int gpu_surface_map_region_locked(FB_GFX3_GPU_SURFACE_NODE *node,
	int x, int y, int width, int height, unsigned int access, void **pixels,
	int *pitch)
{
	size_t row_bytes;
	size_t allocation_size;
	uint32_t bytes_per_pixel;
	int result = FB_GFX3_INVALID;

	if ((node == NULL) || (pixels == NULL) || (pitch == NULL) ||
	    (node->mapped_pixels != NULL) || (width <= 0) || (height <= 0) ||
	    (x < 0) || (y < 0) || (width > (INT_MAX - x)) ||
	    (height > (INT_MAX - y)) || (access == 0) ||
	    (access & ~(FB_GFX3_MAP_READ | FB_GFX3_MAP_WRITE)) ||
	    ((uint32_t)x + (uint32_t)width > node->surface.width) ||
	    ((uint32_t)y + (uint32_t)height > node->surface.height))
		return result;
	/*
	    The requested rectangle is staged as a self-contained image. Downloading
	    it even for a write map preserves the pixels that the caller does not
	    modify. Mapping consequently always needs download authority; a writable
	    map also needs upload authority for its eventual commit.
	*/
	if (!(node->surface.usage & FB_GFX3_SURFACE_TRANSFER_SOURCE) ||
	    ((access & FB_GFX3_MAP_WRITE) &&
	     !(node->surface.usage & FB_GFX3_SURFACE_TRANSFER_DESTINATION)))
		return result;
	bytes_per_pixel = gpu_surface_bytes_per_pixel(node->surface.depth);
	if ((fb_gfx3_size_multiply((uint32_t)width, bytes_per_pixel,
	     &row_bytes) != FB_GFX3_OK) || (row_bytes > INT_MAX) ||
	    (fb_gfx3_size_multiply(row_bytes, (uint32_t)height,
	     &allocation_size) != FB_GFX3_OK) || (allocation_size == 0))
		return result;
	node->mapped_pixels = (unsigned char *)malloc(allocation_size);
	if (node->mapped_pixels == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	result = fb_gfx3_surface_download(&node->surface, x, y,
		(uint32_t)width, (uint32_t)height, (uint32_t)row_bytes,
		node->mapped_pixels);
	if (result != FB_GFX3_OK) {
		gpu_surface_release_map(node);
		return result;
	}
	node->mapped_size = allocation_size;
	node->mapped_pitch = (uint32_t)row_bytes;
	node->mapped_access = access;
	node->mapped_x = x;
	node->mapped_y = y;
	node->mapped_width = (uint32_t)width;
	node->mapped_height = (uint32_t)height;
	*pixels = node->mapped_pixels;
	*pitch = (int)row_bytes;
	return FB_GFX3_OK;
}

FBCALL int fb_Gfx3SurfaceMap(void *surface_pointer, unsigned int access,
	void **pixels, int *pitch)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_GPU_SURFACE_NODE *node;
	int result = FB_GFX3_INVALID;

	if (pixels != NULL)
		*pixels = NULL;
	if (pitch != NULL)
		*pitch = 0;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	node = (state == NULL) ? NULL :
		gpu_surface_find_locked(surface_pointer, state->mode, NULL);
	if (node != NULL)
		result = gpu_surface_map_region_locked(node, 0, 0,
			(int)node->surface.width, (int)node->surface.height, access,
			pixels, pitch);
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

FBCALL int fb_Gfx3SurfaceMapRect(void *surface_pointer, int x, int y,
	int width, int height, unsigned int access, void **pixels, int *pitch)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_GPU_SURFACE_NODE *node;
	int result = FB_GFX3_INVALID;

	if (pixels != NULL)
		*pixels = NULL;
	if (pitch != NULL)
		*pitch = 0;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	node = (state == NULL) ? NULL :
		gpu_surface_find_locked(surface_pointer, state->mode, NULL);
	if (node != NULL)
		result = gpu_surface_map_region_locked(node, x, y, width, height,
			access, pixels, pitch);

	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

FBCALL int fb_Gfx3SurfaceUnmap(void *surface_pointer)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_GPU_SURFACE_NODE *node;
	int result = FB_GFX3_INVALID;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	node = (state == NULL) ? NULL :
		gpu_surface_find_locked(surface_pointer, state->mode, NULL);
	if ((node == NULL) || (node->mapped_pixels == NULL))
		goto unlock;
	if (node->mapped_access & FB_GFX3_MAP_WRITE)
		result = fb_gfx3_surface_upload(&node->surface, node->mapped_x,
			node->mapped_y, node->mapped_width, node->mapped_height,
			node->mapped_pitch, node->mapped_pixels);
	else
		result = FB_GFX3_OK;
	if (result == FB_GFX3_OK)
		gpu_surface_release_map(node);

unlock:
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

FBCALL int fb_Gfx3SurfaceClear(void *surface_pointer,
	unsigned int clear_color)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *surface;
	FB_GFX3_RECT clip;
	int result = FB_GFX3_INVALID;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state != NULL) &&
	    (fb_gfx3_gpu_surface_lookup_locked(surface_pointer, state->mode,
	     &surface) == FB_GFX3_OK)) {
		clip.x1 = 0;
		clip.y1 = 0;
		clip.x2 = (int32_t)surface->width - 1;
		clip.y2 = (int32_t)surface->height - 1;
		result = fb_gfx3_surface_clear(surface, &clip, clear_color, 0);
	}
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

FBCALL int fb_Gfx3SurfaceBlit(void *destination_pointer,
	void *source_pointer, int source_x, int source_y, int width, int height,
	int destination_x, int destination_y, int mode, unsigned int alpha)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *destination;
	FB_GFX3_SURFACE *source;
	FB_GFX3_RECT clip;
	FB_GFX3_RECT source_rect;
	int64_t source_x2;
	int64_t source_y2;
	int64_t destination_x2;
	int64_t destination_y2;
	int screen_destination = FALSE;
	int result = FB_GFX3_INVALID;

	source_x2 = (int64_t)source_x + width - 1;
	source_y2 = (int64_t)source_y + height - 1;
	destination_x2 = (int64_t)destination_x + width - 1;
	destination_y2 = (int64_t)destination_y + height - 1;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state != NULL) && (width > 0) && (height > 0) &&
	    (source_x2 >= INT32_MIN) && (source_x2 <= INT32_MAX) &&
	    (source_y2 >= INT32_MIN) && (source_y2 <= INT32_MAX) &&
	    (destination_x2 >= INT32_MIN) &&
	    (destination_x2 <= INT32_MAX) &&
	    (destination_y2 >= INT32_MIN) &&
	    (destination_y2 <= INT32_MAX) &&
	    (mode >= FB_GFX3_BLIT_TRANS) && (mode <= FB_GFX3_BLIT_BLEND) &&
	    (mode != FB_GFX3_BLIT_CUSTOM)) {
		if (destination_pointer == NULL) {
			if (state->work_page >= state->mode->page_count)
				goto unlock;
			destination = &state->mode->pages[state->work_page];
			clip = state->view;
			screen_destination = TRUE;
			/*
				A deferred PSET stream or writable SCREENPTR shadow must precede
				this screen blit. Opaque GPU surfaces have no such compatibility
				state and retain the original direct lookup path below.
			*/
			result = fb_gfx3_compat_commit_shadow(state);
			if (result == FB_GFX3_OK)
				result =
					fb_gfx3_compat_flush_points_graphics_locked(state);
			if (result != FB_GFX3_OK)
				goto unlock;
		} else {
			result = fb_gfx3_gpu_surface_lookup_locked(destination_pointer,
				state->mode, &destination);
			if (result != FB_GFX3_OK)
				goto unlock;
			clip.x1 = 0;
			clip.y1 = 0;
			clip.x2 = (int32_t)destination->width - 1;
			clip.y2 = (int32_t)destination->height - 1;
		}
		result = fb_gfx3_gpu_surface_lookup_locked(source_pointer,
			state->mode, &source);
		if (result != FB_GFX3_OK)
			goto unlock;
		source_rect.x1 = source_x;
		source_rect.y1 = source_y;
		source_rect.x2 = (int32_t)source_x2;
		source_rect.y2 = (int32_t)source_y2;
		result = fb_gfx3_surface_blit_graphics_locked(destination, &clip,
			source,
			&source_rect, destination_x, destination_y, (uint32_t)mode,
			alpha);
		if ((result == FB_GFX3_OK) && screen_destination) {
			if (state->mode->shadow_valid != NULL)
				state->mode->shadow_valid[state->work_page] = FALSE;
			fb_gfx3_compat_invalidate_point_cache_rect_graphics_locked(
				state, destination_x, destination_y,
				(int)destination_x2, (int)destination_y2);
		}
	}

unlock:
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

/* ------------------------------------------------------------------------- */
/* GPU-resident transforms                                                   */
/* ------------------------------------------------------------------------- */

static int gpu_surface_transform_destination_locked(FB_GFX3_DRAW_STATE *state,
	void *destination_pointer, FB_GFX3_SURFACE **destination,
	FB_GFX3_RECT *clip, int *screen_destination)
{
	if ((state == NULL) || (state->mode == NULL) || (destination == NULL) ||
	    (clip == NULL) || (screen_destination == NULL))
		return FB_GFX3_INVALID;
	if (destination_pointer == NULL) {
		if (state->work_page >= state->mode->page_count)
			return FB_GFX3_INVALID;
		*destination = &state->mode->pages[state->work_page];
		*clip = state->view;
		*screen_destination = TRUE;
		return FB_GFX3_OK;
	}
	if (fb_gfx3_gpu_surface_lookup_locked(destination_pointer, state->mode,
	    destination) != FB_GFX3_OK)
		return FB_GFX3_INVALID;
	clip->x1 = 0;
	clip->y1 = 0;
	clip->x2 = (int32_t)(*destination)->width - 1;
	clip->y2 = (int32_t)(*destination)->height - 1;
	*screen_destination = FALSE;
	return FB_GFX3_OK;
}

static void gpu_surface_transform_invalidate_screen_locked(
	FB_GFX3_DRAW_STATE *state, const FB_GFX3_RECT *bounds)
{
	if ((state == NULL) || (state->mode == NULL) || (bounds == NULL))
		return;
	if (state->mode->shadow_valid != NULL)
		state->mode->shadow_valid[state->work_page] = FALSE;
	fb_gfx3_compat_invalidate_point_cache_rect_graphics_locked(state,
		bounds->x1, bounds->y1, bounds->x2, bounds->y2);
}

static int gpu_surface_transform_submit_locked(FB_GFX3_DRAW_STATE *state,
	void *destination_pointer, void *source_pointer,
	const FB_GFX3_RECT *source_rect, const FB_GFX3_RECT *destination_bounds,
	const float inverse[9], int mode, unsigned int alpha, int filter, int wrap)
{
	FB_GFX3_SURFACE *destination;
	FB_GFX3_SURFACE *source;
	FB_GFX3_RECT clip;
	int screen_destination;
	int result;

	if ((state == NULL) || (source_rect == NULL) ||
	    (destination_bounds == NULL) || (inverse == NULL) ||
	    (mode < FB_GFX3_BLIT_TRANS) || (mode > FB_GFX3_BLIT_BLEND) ||
	    (mode == FB_GFX3_BLIT_CUSTOM) ||
	    (filter < FB_GFX3_TRANSFORM_FILTER_NEAREST) ||
	    (filter > FB_GFX3_TRANSFORM_FILTER_LINEAR) ||
	    (wrap < FB_GFX3_TRANSFORM_WRAP_CLAMP) ||
	    (wrap > FB_GFX3_TRANSFORM_WRAP_REPEAT))
		return FB_GFX3_INVALID;
	result = fb_gfx3_compat_flush_points_graphics_locked(state);
	if (result != FB_GFX3_OK)
		return result;
	result = gpu_surface_transform_destination_locked(state,
		destination_pointer, &destination, &clip, &screen_destination);
	if ((result != FB_GFX3_OK) ||
	    (fb_gfx3_gpu_surface_lookup_locked(source_pointer, state->mode,
	     &source) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	if ((source_rect->x1 < 0) || (source_rect->y1 < 0) ||
	    (source_rect->x1 > source_rect->x2) ||
	    (source_rect->y1 > source_rect->y2) ||
	    (source_rect->x2 >= (int32_t)source->width) ||
	    (source_rect->y2 >= (int32_t)source->height) ||
	    (destination_bounds->x1 > destination_bounds->x2) ||
	    (destination_bounds->y1 > destination_bounds->y2))
		return FB_GFX3_INVALID;
	result = fb_gfx3_surface_transform_blit(destination, &clip, source,
		source_rect, destination_bounds, inverse, (uint32_t)mode, alpha,
		(uint32_t)filter, (uint32_t)wrap);
	if ((result == FB_GFX3_OK) && screen_destination)
		gpu_surface_transform_invalidate_screen_locked(state,
			destination_bounds);
	return result;
}

FBCALL int fb_Gfx3SurfaceBlitScaled(void *destination_pointer,
	void *source_pointer, int source_x, int source_y, int source_width,
	int source_height, int destination_x, int destination_y,
	int destination_width, int destination_height, int mode,
	unsigned int alpha, int filter)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_RECT source_rect;
	FB_GFX3_RECT destination_bounds;
	float inverse[9] = { 0.0f };
	int64_t source_right;
	int64_t source_bottom;
	int64_t destination_right;
	int64_t destination_bottom;
	int result = FB_GFX3_INVALID;

	source_right = (int64_t)source_x + source_width - 1;
	source_bottom = (int64_t)source_y + source_height - 1;
	destination_right = (int64_t)destination_x + destination_width - 1;
	destination_bottom = (int64_t)destination_y + destination_height - 1;
	if ((source_width <= 0) || (source_height <= 0) ||
	    (destination_width <= 0) || (destination_height <= 0) ||
	    (source_right > INT32_MAX) || (source_bottom > INT32_MAX) ||
	    (destination_right > INT32_MAX) ||
	    (destination_bottom > INT32_MAX))
		goto done;
	source_rect.x1 = source_x;
	source_rect.y1 = source_y;
	source_rect.x2 = (int32_t)source_right;
	source_rect.y2 = (int32_t)source_bottom;
	destination_bounds.x1 = destination_x;
	destination_bounds.y1 = destination_y;
	destination_bounds.x2 = (int32_t)destination_right;
	destination_bounds.y2 = (int32_t)destination_bottom;
	inverse[0] = (float)source_width / (float)destination_width;
	inverse[2] = (float)source_x - ((float)destination_x * inverse[0]);
	inverse[4] = (float)source_height / (float)destination_height;
	inverse[5] = (float)source_y - ((float)destination_y * inverse[4]);
	inverse[8] = 1.0f;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state != NULL)
		result = gpu_surface_transform_submit_locked(state,
			destination_pointer, source_pointer, &source_rect,
			&destination_bounds, inverse, mode, alpha, filter,
			FB_GFX3_TRANSFORM_WRAP_CLAMP);
	FB_GRAPHICS_UNLOCK();

done:
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

FBCALL int fb_Gfx3SurfaceBlitRotated(void *destination_pointer,
	void *source_pointer, int source_x, int source_y, int source_width,
	int source_height, float destination_x, float destination_y,
	float angle_degrees, float scale_x, float scale_y, float pivot_x,
	float pivot_y, int mode, unsigned int alpha, int filter)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_RECT source_rect;
	FB_GFX3_RECT destination_bounds;
	float inverse[9] = { 0.0f };
	double angle;
	double cosine;
	double sine;
	double minimum_x = HUGE_VAL;
	double minimum_y = HUGE_VAL;
	double maximum_x = -HUGE_VAL;
	double maximum_y = -HUGE_VAL;
	double corner_x[2];
	double corner_y[2];
	int ix;
	int iy;
	int result = FB_GFX3_INVALID;

	if ((source_width <= 0) || (source_height <= 0) ||
	    !isfinite(destination_x) || !isfinite(destination_y) ||
	    !isfinite(angle_degrees) || !isfinite(scale_x) ||
	    !isfinite(scale_y) || !isfinite(pivot_x) || !isfinite(pivot_y) ||
	    (scale_x == 0.0f) || (scale_y == 0.0f) ||
	    ((int64_t)source_x + source_width - 1 > INT32_MAX) ||
	    ((int64_t)source_y + source_height - 1 > INT32_MAX))
		goto done;
	if (pivot_x < 0.0f)
		pivot_x = (float)source_width * 0.5f;
	if (pivot_y < 0.0f)
		pivot_y = (float)source_height * 0.5f;
	angle = (double)angle_degrees * (3.14159265358979323846 / 180.0);
	cosine = cos(angle);
	sine = sin(angle);
	corner_x[0] = -(double)pivot_x;
	corner_x[1] = (double)source_width - pivot_x;
	corner_y[0] = -(double)pivot_y;
	corner_y[1] = (double)source_height - pivot_y;
	for (iy = 0; iy < 2; ++iy) {
		for (ix = 0; ix < 2; ++ix) {
			double local_x = corner_x[ix] * scale_x;
			double local_y = corner_y[iy] * scale_y;
			double transformed_x = destination_x +
				(cosine * local_x) - (sine * local_y);
			double transformed_y = destination_y +
				(sine * local_x) + (cosine * local_y);

			if (transformed_x < minimum_x)
				minimum_x = transformed_x;
			if (transformed_x > maximum_x)
				maximum_x = transformed_x;
			if (transformed_y < minimum_y)
				minimum_y = transformed_y;
			if (transformed_y > maximum_y)
				maximum_y = transformed_y;
		}
	}
	if ((minimum_x < INT32_MIN) || (minimum_y < INT32_MIN) ||
	    (maximum_x > (double)INT32_MAX + 1.0) ||
	    (maximum_y > (double)INT32_MAX + 1.0))
		goto done;
	source_rect.x1 = source_x;
	source_rect.y1 = source_y;
	source_rect.x2 = source_x + source_width - 1;
	source_rect.y2 = source_y + source_height - 1;
	destination_bounds.x1 = (int32_t)floor(minimum_x);
	destination_bounds.y1 = (int32_t)floor(minimum_y);
	destination_bounds.x2 = (int32_t)ceil(maximum_x) - 1;
	destination_bounds.y2 = (int32_t)ceil(maximum_y) - 1;
	inverse[0] = (float)(cosine / scale_x);
	inverse[1] = (float)(sine / scale_x);
	inverse[2] = (float)source_x + pivot_x -
		(inverse[0] * destination_x) - (inverse[1] * destination_y);
	inverse[3] = (float)(-sine / scale_y);
	inverse[4] = (float)(cosine / scale_y);
	inverse[5] = (float)source_y + pivot_y -
		(inverse[3] * destination_x) - (inverse[4] * destination_y);
	inverse[8] = 1.0f;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state != NULL)
		result = gpu_surface_transform_submit_locked(state,
			destination_pointer, source_pointer, &source_rect,
			&destination_bounds, inverse, mode, alpha, filter,
			FB_GFX3_TRANSFORM_WRAP_CLAMP);
	FB_GRAPHICS_UNLOCK();

done:
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

FBCALL int fb_Gfx3SurfaceMode7(void *destination_pointer,
	void *source_pointer, int source_x, int source_y, int source_width,
	int source_height, int destination_x, int destination_y,
	int destination_width, int destination_height, float camera_x,
	float camera_y, float camera_height, float camera_angle_degrees,
	float horizon_y, float focal_length, int mode, unsigned int alpha,
	int filter)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_RECT source_rect;
	FB_GFX3_RECT destination_bounds;
	float inverse[9] = { 0.0f };
	double angle;
	double cosine;
	double sine;
	double centre_x;
	double absolute_camera_x;
	double absolute_camera_y;
	int result = FB_GFX3_INVALID;

	if ((source_width <= 0) || (source_height <= 0) ||
	    (destination_width <= 0) || (destination_height <= 0) ||
	    !isfinite(camera_x) || !isfinite(camera_y) ||
	    !isfinite(camera_height) || !isfinite(camera_angle_degrees) ||
	    !isfinite(horizon_y) || !isfinite(focal_length) ||
	    (camera_height <= 0.0f) || (focal_length <= 0.0f) ||
	    ((int64_t)source_x + source_width - 1 > INT32_MAX) ||
	    ((int64_t)source_y + source_height - 1 > INT32_MAX) ||
	    ((int64_t)destination_x + destination_width - 1 > INT32_MAX) ||
	    ((int64_t)destination_y + destination_height - 1 > INT32_MAX))
		goto done;
	source_rect.x1 = source_x;
	source_rect.y1 = source_y;
	source_rect.x2 = source_x + source_width - 1;
	source_rect.y2 = source_y + source_height - 1;
	destination_bounds.x1 = destination_x;
	destination_bounds.y1 = destination_y;
	destination_bounds.x2 = destination_x + destination_width - 1;
	destination_bounds.y2 = destination_y + destination_height - 1;
	angle = (double)camera_angle_degrees *
		(3.14159265358979323846 / 180.0);
	cosine = cos(angle);
	sine = sin(angle);
	centre_x = (double)destination_x + ((double)destination_width * 0.5);
	absolute_camera_x = (double)source_x + camera_x;
	absolute_camera_y = (double)source_y + camera_y;
	/*
		Divide both source numerators by destination y minus the horizon.
		This is the classic scanline Mode 7 projection expressed as one inverse
		projective matrix, so each output pixel remains an independent GPU job.
	*/
	inverse[0] = (float)(-sine * camera_height);
	inverse[1] = (float)absolute_camera_x;
	inverse[2] = (float)((sine * camera_height * centre_x) -
		(absolute_camera_x * horizon_y) +
		(cosine * camera_height * focal_length));
	inverse[3] = (float)(cosine * camera_height);
	inverse[4] = (float)absolute_camera_y;
	inverse[5] = (float)((-cosine * camera_height * centre_x) -
		(absolute_camera_y * horizon_y) +
		(sine * camera_height * focal_length));
	inverse[7] = 1.0f;
	inverse[8] = -horizon_y;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state != NULL)
		result = gpu_surface_transform_submit_locked(state,
			destination_pointer, source_pointer, &source_rect,
			&destination_bounds, inverse, mode, alpha, filter,
			FB_GFX3_TRANSFORM_WRAP_REPEAT);
	FB_GRAPHICS_UNLOCK();

done:
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

FBCALL int fb_Gfx3SurfacePresent(void *surface_pointer, int wait)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *surface;
	int result = FB_GFX3_INVALID;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state != NULL) &&
	    (fb_gfx3_gpu_surface_lookup_locked(surface_pointer, state->mode,
	     &surface) == FB_GFX3_OK))
		result = fb_gfx3_surface_present(surface, wait != 0);
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

FBCALL int fb_Gfx3DrawPoints(void *destination_pointer,
	const FB_GFX3_PUBLIC_POINT *public_points, int count)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *surface = NULL;
	FB_GFX3_POINT *points = NULL;
	FB_GFX3_RECT clip;
	size_t allocation_size;
	uint32_t bytes_per_pixel;
	int index;
	int result = FB_GFX3_INVALID;

	if ((count < 0) || ((count != 0) && (public_points == NULL)) ||
	    (fb_gfx3_size_multiply((size_t)count, sizeof(*points),
	     &allocation_size) != FB_GFX3_OK))
		goto done;
	if (count != 0) {
		points = (FB_GFX3_POINT *)malloc(allocation_size);
		if (points == NULL) {
			result = FB_GFX3_OUT_OF_MEMORY;
			goto done;
		}
	}
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state == NULL)
		goto unlock;
	if (destination_pointer == NULL) {
		if (state->work_page >= state->mode->page_count)
			goto unlock;
		surface = &state->mode->pages[state->work_page];
		clip = state->view;
	} else {
		if (fb_gfx3_gpu_surface_lookup_locked(destination_pointer,
		    state->mode, &surface) != FB_GFX3_OK)
			goto unlock;
		clip.x1 = 0;
		clip.y1 = 0;
		clip.x2 = (int32_t)surface->width - 1;
		clip.y2 = (int32_t)surface->height - 1;
	}
	bytes_per_pixel = gpu_surface_bytes_per_pixel(surface->depth);
	for (index = 0; index < count; index++) {
		uint32_t alpha = public_points[index].alpha;

		if (alpha > 255u)
			alpha = 255u;
		if ((alpha < 255u) && (surface->depth != 32u)) {
			result = FB_GFX3_UNSUPPORTED;
			goto unlock;
		}
		points[index].x = public_points[index].x;
		points[index].y = public_points[index].y;
		points[index].color = fb_gfx3_image_fix_color(bytes_per_pixel,
			public_points[index].color);
		points[index].flags = 0u;
		if (surface->depth == 32u) {
			points[index].color =
				(points[index].color & 0x00FFFFFFu) | (alpha << 24);
			if (alpha < 255u)
				points[index].flags =
					FB_GFX3_PRIMITIVE_ALPHA_BLEND;
		}
	}
	if (destination_pointer == NULL) {
		result = fb_gfx3_compat_points_logical(state, points,
			(uint32_t)count);
	} else {
		result = fb_gfx3_surface_points(surface, &clip, points,
			(uint32_t)count);
	}

unlock:
	FB_GRAPHICS_UNLOCK();
done:
	free(points);
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

/* ------------------------------------------------------------------------- */
/* Standard primitive target compatibility                                   */
/* ------------------------------------------------------------------------- */

static uint32_t gpu_surface_color(FB_GFX3_SURFACE *surface,
	FB_GFX3_DRAW_STATE *state, uint32_t color, uint32_t flags, int preset)
{
	if (flags & FB_GFX3_DEFAULT_COLOR_1)
		return preset ? state->background_color : state->foreground_color;
	return fb_gfx3_image_fix_color(
		gpu_surface_bytes_per_pixel(surface->depth), color);
}

static void gpu_surface_fix_relative(FB_GFX3_DRAW_STATE *state,
	uint32_t flags, float *x1, float *y1, float *x2, float *y2)
{
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

static int gpu_surface_coordinates(float x, float y, int *ix, int *iy)
{
	if (!isfinite(x) || !isfinite(y) || ((double)x < INT_MIN) ||
	    ((double)x > INT_MAX) || ((double)y < INT_MIN) ||
	    ((double)y > INT_MAX))
		return FB_GFX3_INVALID;
	*ix = CINT(x);
	*iy = CINT(y);
	return FB_GFX3_OK;
}

static void gpu_surface_full_clip(const FB_GFX3_SURFACE *surface,
	FB_GFX3_RECT *clip)
{
	clip->x1 = 0;
	clip->y1 = 0;
	clip->x2 = (int32_t)surface->width - 1;
	clip->y2 = (int32_t)surface->height - 1;
}

int fb_gfx3_gpu_surface_pset(FB_GFX3_SURFACE *surface,
	FB_GFX3_DRAW_STATE *state, float x, float y, uint32_t color,
	uint32_t flags, int preset)
{
	FB_GFX3_POINT point;
	FB_GFX3_RECT clip;
	int result;

	if ((surface == NULL) || (state == NULL))
		return FB_GFX3_INVALID;
	gpu_surface_fix_relative(state, flags, &x, &y, NULL, NULL);
	result = gpu_surface_coordinates(x, y, &point.x, &point.y);
	if (result != FB_GFX3_OK)
		return result;
	point.color = gpu_surface_color(surface, state, color, flags, preset);
	point.flags = fb_gfx3_compat_primitive_flags(state, surface->depth,
		point.color);
	gpu_surface_full_clip(surface, &clip);
	return fb_gfx3_surface_points(surface, &clip, &point, 1);
}

int fb_gfx3_gpu_surface_point(FB_GFX3_SURFACE *surface, float x, float y,
	uint32_t *color)
{
	int ix;
	int iy;
	int result;

	if ((surface == NULL) || (color == NULL))
		return FB_GFX3_INVALID;
	result = gpu_surface_coordinates(x, y, &ix, &iy);
	if (result != FB_GFX3_OK)
		return result;
	if ((ix < 0) || (iy < 0) || ((uint32_t)ix >= surface->width) ||
	    ((uint32_t)iy >= surface->height)) {
		*color = UINT32_MAX;
		return FB_GFX3_OK;
	}
	result = fb_gfx3_surface_read_pixel(surface, ix, iy, color);
	if ((result == FB_GFX3_OK) && (surface->depth == 16))
		*color = fb_gfx3_image_expand_color(2, *color);
	return result;
}

int fb_gfx3_gpu_surface_line(FB_GFX3_SURFACE *surface,
	FB_GFX3_DRAW_STATE *state, float x1, float y1, float x2, float y2,
	uint32_t color, int type, uint32_t style, uint32_t flags)
{
	FB_GFX3_RECT clip;
	int ix1;
	int iy1;
	int ix2 = 0;
	int iy2 = 0;
	int temporary;
	int result;

	if ((surface == NULL) || (state == NULL) ||
	    (type < FB_GFX3_LINE_TYPE_LINE) ||
	    (type > FB_GFX3_LINE_TYPE_FILLED_BOX))
		return FB_GFX3_INVALID;
	gpu_surface_fix_relative(state, flags, &x1, &y1, &x2, &y2);
	result = gpu_surface_coordinates(x1, y1, &ix1, &iy1);
	if (result == FB_GFX3_OK)
		result = gpu_surface_coordinates(x2, y2, &ix2, &iy2);
	if (result != FB_GFX3_OK)
		return result;
	color = gpu_surface_color(surface, state, color, flags, FALSE);
	gpu_surface_full_clip(surface, &clip);
	if (type == FB_GFX3_LINE_TYPE_LINE)
		return fb_gfx3_surface_line(surface, &clip, ix1, iy1, ix2, iy2,
			color, style & 0xFFFFu,
			fb_gfx3_compat_primitive_flags(state, surface->depth, color));
	if (ix2 < ix1) {
		temporary = ix1;
		ix1 = ix2;
		ix2 = temporary;
	}
	if (iy2 < iy1) {
		temporary = iy1;
		iy1 = iy2;
		iy2 = temporary;
	}
	return fb_gfx3_surface_rectangle(surface, &clip, ix1, iy1, ix2, iy2,
		color, style & 0xFFFFu,
		type == FB_GFX3_LINE_TYPE_FILLED_BOX,
		fb_gfx3_compat_primitive_flags(state, surface->depth, color));
}

static int gpu_surface_add_coordinate_offset(int coordinate, int offset)
{
	int64_t result = (int64_t)coordinate + offset;

	if (result < INT_MIN)
		return INT_MIN;
	if (result > INT_MAX)
		return INT_MAX;
	return (int)result;
}

static int gpu_surface_arc(FB_GFX3_SURFACE *surface,
	FB_GFX3_DRAW_STATE *state, int center_x, int center_y, float radius_x,
	float radius_y, uint32_t color, float start, float end)
{
	FB_GFX3_POINT *points = NULL;
	FB_GFX3_RECT clip;
	float increment;
	float span;
	float angle;
	float turns;
	int endpoint_x;
	int endpoint_y;
	int start_radial;
	int end_radial;
	uint32_t count;
	uint32_t i;
	size_t allocation_size;
	int result;

	(void)state;
	if (!(radius_x > 0.0f) || !(radius_y > 0.0f) ||
	    (radius_x > 32767.0f) || (radius_y > 32767.0f))
		return FB_GFX3_INVALID;
	start_radial = start < 0.0f;
	end_radial = end < 0.0f;
	if (start_radial)
		start = -start;
	if (end_radial)
		end = -end;
	span = end - start;
	if (span < 0.0f) {
		turns = ceilf((-span) / 6.28318530717958647692f);
		end += turns * 6.28318530717958647692f;
	}
	span = end - start;
	if (span > 6.28318530717958647692f) {
		turns = floorf(span / 6.28318530717958647692f);
		start += turns * 6.28318530717958647692f;
		if ((end - start) > 6.28318530717958647692f)
			start += 6.28318530717958647692f;
	}
	span = end - start;
	increment = 1.0f / (sqrtf(radius_x) * sqrtf(radius_y) * 1.5f);
	if (!isfinite(increment) || !(increment > 0.0f) || !isfinite(span) ||
	    (span < 0.0f) || ((double)span / increment + 1.5 > UINT32_MAX))
		return FB_GFX3_INVALID;
	count = (uint32_t)(span / increment + 0.5f) + 1u;
	if ((count == 0u) ||
	    (fb_gfx3_size_multiply(count, sizeof(points[0]),
	     &allocation_size) != FB_GFX3_OK) ||
	    (allocation_size == 0u) ||
	    (allocation_size > FB_GFX3_COMMAND_MAX_SIZE / 2u))
		return FB_GFX3_INVALID;
	points = (FB_GFX3_POINT *)calloc(1, allocation_size);
	if (points == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	for (i = 0; i < count; i++) {
		angle = start + ((float)i * increment);
		points[i].x = gpu_surface_add_coordinate_offset(center_x,
			CINT(cosf(angle) * radius_x));
		points[i].y = gpu_surface_add_coordinate_offset(center_y,
			-CINT(sinf(angle) * radius_y));
		points[i].color = color;
		points[i].flags = fb_gfx3_compat_primitive_flags(state,
			surface->depth, color);
	}
	gpu_surface_full_clip(surface, &clip);
	if (start_radial) {
		endpoint_x = gpu_surface_add_coordinate_offset(center_x,
			CINT(cosf(start) * radius_x));
		endpoint_y = gpu_surface_add_coordinate_offset(center_y,
			-CINT(sinf(start) * radius_y));
		result = fb_gfx3_surface_line(surface, &clip, center_x, center_y,
			endpoint_x, endpoint_y, color, 0xFFFFu,
			fb_gfx3_compat_primitive_flags(state, surface->depth, color));
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (end_radial) {
		endpoint_x = gpu_surface_add_coordinate_offset(center_x,
			CINT(cosf(end) * radius_x));
		endpoint_y = gpu_surface_add_coordinate_offset(center_y,
			-CINT(sinf(end) * radius_y));
		result = fb_gfx3_surface_line(surface, &clip, center_x, center_y,
			endpoint_x, endpoint_y, color, 0xFFFFu,
			fb_gfx3_compat_primitive_flags(state, surface->depth, color));
		if (result != FB_GFX3_OK)
			goto done;
	}
	result = fb_gfx3_surface_points(surface, &clip, points, count);

done:
	free(points);
	return result;
}

int fb_gfx3_gpu_surface_ellipse(FB_GFX3_SURFACE *surface,
	FB_GFX3_DRAW_STATE *state, float x, float y, float radius,
	uint32_t color, float aspect, float start, float end, int filled,
	uint32_t flags)
{
	FB_GFX3_RECT clip;
	float radius_x;
	float radius_y;
	int center_x;
	int center_y;
	int result;

	if ((surface == NULL) || (state == NULL) || !isfinite(radius) ||
	    !isfinite(aspect) || !isfinite(start) || !isfinite(end))
		return FB_GFX3_INVALID;
	if (!(radius > 0.0f))
		return FB_GFX3_OK;
	gpu_surface_fix_relative(state, flags, &x, &y, NULL, NULL);
	result = gpu_surface_coordinates(x, y, &center_x, &center_y);
	if (result != FB_GFX3_OK)
		return result;
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
	color = gpu_surface_color(surface, state, color, flags, FALSE);
	if ((start != 0.0f) || (end != 6.283186f))
		return gpu_surface_arc(surface, state, center_x, center_y, radius_x,
			radius_y, color, start, end);
	gpu_surface_full_clip(surface, &clip);
	return fb_gfx3_surface_ellipse(surface, &clip, center_x, center_y,
		radius_x, radius_y, color, filled,
		fb_gfx3_compat_primitive_flags(state, surface->depth, color));
}

/* end of gfx3_gpu_surface.c */
