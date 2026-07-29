/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_compat.c

    Purpose:

        Implement the first stateful compatibility layer above the typed
        gfxlib3 surface API.

    Responsibilities:

        - create and destroy logical GPU screen pages
        - preserve per-caller VIEW, WINDOW, page, color, and pen state
        - translate PSET, POINT, LINE, boxes, and full ellipses
        - implement page selection, page copy, PMAP, and POINTCOORD behavior

    This file intentionally does NOT contain:

        - public fb_Gfx* wrappers or runtime hook installation
        - arcs, PAINT, DRAW, text, palette, or FB.IMAGE handling
        - presentation or platform event processing
*/

#include "gfx3_compat.h"
#include "gfx3_console.h"
#include "gfx3_data.h"

#include <math.h>
#include <stdatomic.h>

/*
	A 131072-entry stream substantially reduces GPU submission overhead for a
	long PSET run without letting one compatibility state grow without bound.
	The original-order stream, layer-sorted submission stream, layer metadata,
	and coordinate hash tables use less than 7 MiB per active graphics state.
*/
#define FB_GFX3_PENDING_POINTS_CAPACITY 131072u
#define FB_GFX3_PENDING_POINT_KEY_CAPACITY 131072u

/*
	A single POINT/PSET pair is often an isolated query and is cheaper as two
	GPU commands. A second pair is strong evidence of a software alpha or image
	loop, where one full-page synchronization is dramatically cheaper than a
	readback for every remaining pixel.
*/
#define FB_GFX3_POINT_SHADOW_PAIR_THRESHOLD 2u

static _Atomic uint64_t next_mode_generation = 1;

/* ------------------------------------------------------------------------- */
/* Validation, color, and coordinate helpers                                 */
/* ------------------------------------------------------------------------- */

static uint32_t compat_color_mask(uint32_t depth)
{
	if (depth >= 32)
		return UINT32_MAX;
	return (1u << depth) - 1u;
}

static uint32_t compat_fix_color(uint32_t depth, uint32_t color)
{
	if (depth == 16) {
		uint32_t red = (color >> 16) & 0xFFu;
		uint32_t green = (color >> 8) & 0xFFu;
		uint32_t blue = color & 0xFFu;

		return (blue >> 3) | ((green << 3) & 0x07E0u) |
			((red << 8) & 0xF800u);
	}
	return color & compat_color_mask(depth);
}

static uint32_t compat_expand_16_bit_color(uint32_t color)
{
	return ((color & 0x001Fu) << 3) | ((color >> 2) & 0x7u) |
		((color & 0x07E0u) << 5) | ((color >> 1) & 0x300u) |
		((color & 0xF800u) << 8) | ((color << 3) & 0x70000u);
}

static FB_GFX3_MODE *compat_state_lock(FB_GFX3_DRAW_STATE *state)
{
	FB_GFX3_MODE *mode;

	if ((state == NULL) || (state->mode == NULL))
		return NULL;
	mode = state->mode;
	if (mode->mutex == NULL)
		return NULL;
	fb_MutexLock(mode->mutex);
	if (!mode->initialized || (state->generation != mode->generation) ||
	    (state->work_page >= mode->page_count)) {
		fb_MutexUnlock(mode->mutex);
		return NULL;
	}
	return mode;
}

/*
	Public primitive entry points already hold the runtime-wide
	FB_GRAPHICS_LOCK. Taking the mode mutex again for every pixel, line, or
	ellipse adds two uncontended lock operations without additional exclusion:
	mode teardown, page changes, and every other public graphics entry point take
	that outer lock first. Keep the mutex route for internal control callers, but
	use this narrow validator in compatibility drawing hot paths.
*/
static FB_GFX3_MODE *compat_state_graphics_locked(FB_GFX3_DRAW_STATE *state)
{
	FB_GFX3_MODE *mode;

	if ((state == NULL) || (state->mode == NULL))
		return NULL;
	mode = state->mode;
	if (!mode->initialized || (state->generation != mode->generation) ||
	    (state->work_page >= mode->page_count))
		return NULL;
	return mode;
}

/*
	The locked PSET shadow is a CPU mirror of the current GPU page.  It must use
	the same unusual gfxlib2 primitive-alpha equation as the renderer; copying a
	translucent source directly would make later POINT reads and uploads observe
	a different result from the GPU command path.
*/
static uint32_t compat_alpha_primitive_pixel(uint32_t source,
	uint32_t destination)
{
	uint32_t alpha = source >> 24;
	uint32_t source_red_blue = source & 0x00FF00FFu;
	uint32_t source_green = source & 0x0000FF00u;
	uint32_t destination_red_blue = destination & 0x00FF00FFu;
	uint32_t destination_green = destination & 0x0000FF00u;

	source_red_blue = ((source_red_blue - destination_red_blue) * alpha) >> 8;
	source_green = ((source_green - destination_green) * alpha) >> 8;
	return ((destination_red_blue + source_red_blue) & 0x00FF00FFu) |
		((destination_green + source_green) & 0x0000FF00u) |
		(source & 0xFF000000u);
}

static int compat_commit_locked_shadow(FB_GFX3_DRAW_STATE *state,
	FB_GFX3_MODE *mode);
static int compat_mirror_shadow_rectangle(FB_GFX3_DRAW_STATE *state,
	FB_GFX3_MODE *mode, int x1, int y1, int x2, int y2, uint32_t color,
	uint32_t style, int filled, uint32_t flags);
static void compat_write_shadow_primitive_pixel(FB_GFX3_MODE *mode,
	uint32_t page, int x, int y, uint32_t color, uint32_t flags);

/*
	Discard one completed pending-point stream in constant time. Hash entries
	are tagged with a generation, so clearing the complete coordinate table
	after every frame would waste memory bandwidth.
*/
static void compat_clear_pending_points_locked(FB_GFX3_DRAW_STATE *state)
{
	state->pending_points_count = 0;
	state->pending_point_max_layer = 0;
	state->pending_point_generation++;
	if (state->pending_point_generation == 0u) {
		memset(state->pending_point_generations, 0,
			(size_t)state->pending_point_key_capacity *
				sizeof(state->pending_point_generations[0]));
		state->pending_point_generation = 1u;
	}
}

/*
	The renderer can process thousands of points in one compute dispatch, but a
	BASIC PSET statement arrives one pixel at a time.  Keep a short caller-local
	batch until a later operation requires its writes to be visible in command
	order.  The caller owns mode->mutex.
*/
static int compat_flush_pending_points_locked(FB_GFX3_DRAW_STATE *state,
	FB_GFX3_MODE *mode)
{
	const FB_GFX3_POINT *submission;
	uint32_t layer;
	uint32_t layer_count;
	uint32_t layer_end;
	uint32_t layer_start;
	uint32_t point_index;
	uint32_t running_offset;
	uint32_t submitted_layers = 0;
	int mirror_shadow;
	int result;

	if ((state->pending_points == NULL) ||
	    (state->pending_points_count == 0))
		return FB_GFX3_OK;
	if ((state->pending_points_page >= mode->page_count) ||
	    (state->pending_points_count > state->pending_points_capacity) ||
	    (state->pending_point_submission == NULL) ||
	    (state->pending_point_layers == NULL) ||
	    (state->pending_point_layer_offsets == NULL) ||
	    (state->pending_point_max_layer >= state->pending_points_count))
		return FB_GFX3_INVALID;
	mirror_shadow = ((mode->depth == 16u) || (mode->depth == 32u)) &&
		(mode->shadow_pages != NULL) &&
		(mode->shadow_valid != NULL) &&
		(mode->shadow_pages[state->pending_points_page] != NULL) &&
		mode->shadow_valid[state->pending_points_page] &&
		(mode->shadow_pitch != 0u);

	/*
		Points in one renderer packet are intentionally independent GPU jobs.
		A downscaled WINDOW can map adjacent logical alpha points onto the same
		physical pixel, however. Stable counting-sort by per-coordinate
		occurrence number turns that stream into a handful of ordered packets:
		every packet remains massively parallel, while successive packets
		preserve gfxlib2's destination-dependent alpha equation exactly.
	*/
	layer_count = state->pending_point_max_layer + 1u;
	submission = state->pending_points;
	if (layer_count > 1u) {
		memset(state->pending_point_layer_offsets, 0,
			(size_t)layer_count *
				sizeof(state->pending_point_layer_offsets[0]));
		for (point_index = 0u;
		     point_index < state->pending_points_count;
		     point_index++) {
			layer = state->pending_point_layers[point_index];
			if (layer >= layer_count)
				return FB_GFX3_INVALID;
			state->pending_point_layer_offsets[layer]++;
		}
		running_offset = 0u;
		for (layer = 0u; layer < layer_count; layer++) {
			uint32_t points_in_layer =
				state->pending_point_layer_offsets[layer];

			state->pending_point_layer_offsets[layer] = running_offset;
			running_offset += points_in_layer;
		}
		if (running_offset != state->pending_points_count)
			return FB_GFX3_INVALID;
		for (point_index = 0u;
		     point_index < state->pending_points_count;
		     point_index++) {
			layer = state->pending_point_layers[point_index];
			state->pending_point_submission[
				state->pending_point_layer_offsets[layer]++] =
					state->pending_points[point_index];
		}
		submission = state->pending_point_submission;
	}

	result = FB_GFX3_OK;
	layer_start = 0u;
	for (layer = 0u; layer < layer_count; layer++) {
		layer_end = (layer_count == 1u) ?
			state->pending_points_count :
			state->pending_point_layer_offsets[layer];
		if ((layer_end <= layer_start) ||
		    (layer_end > state->pending_points_count)) {
			result = FB_GFX3_INVALID;
			break;
		}
		result = fb_gfx3_surface_points(
			&mode->pages[state->pending_points_page],
			&state->pending_points_clip, submission + layer_start,
			layer_end - layer_start);
		if (result != FB_GFX3_OK)
			break;
		submitted_layers++;
		layer_start = layer_end;
	}
	if (result == FB_GFX3_OK) {
		fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_TRACE,
			"PSET batch submitted: page %u, points %u, layers %u, mirror %d",
			state->pending_points_page, state->pending_points_count,
			layer_count, mirror_shadow);
		/*
			An existing shadow can follow a GPU PSET batch at negligible CPU
			cost. This is important for old programs which mix opaque font
			pixels with POINT/PSET alpha pixels: discarding the shadow here
			would force the next POINT to download the complete page.

			The renderer still owns and performs the actual drawing. The small
			CPU writes only preserve a coherent copy which was already present.
		*/
		if (mirror_shadow) {
			for (point_index = 0u;
			     point_index < state->pending_points_count;
			     point_index++) {
				const FB_GFX3_POINT *point =
					&state->pending_points[point_index];

				if ((point->x < state->pending_points_clip.x1) ||
				    (point->x > state->pending_points_clip.x2) ||
				    (point->y < state->pending_points_clip.y1) ||
				    (point->y > state->pending_points_clip.y2) ||
				    (point->x < 0) || (point->y < 0) ||
				    ((uint32_t)point->x >= mode->width) ||
				    ((uint32_t)point->y >= mode->height))
					continue;
				compat_write_shadow_primitive_pixel(mode,
					state->pending_points_page, point->x, point->y,
					point->color, point->flags);
			}
		} else if (mode->shadow_valid != NULL) {
			mode->shadow_valid[state->pending_points_page] = FALSE;
		}
		compat_clear_pending_points_locked(state);
	} else if (submitted_layers != 0u) {
		/*
			A later packet can fail after earlier layers entered the renderer
			queue. Retrying the complete stream would apply those alpha writes
			twice, so discard it and invalidate any CPU mirror.
		*/
		if (mode->shadow_valid != NULL)
			mode->shadow_valid[state->pending_points_page] = FALSE;
		compat_clear_pending_points_locked(state);
	}
	return result;
}

/*
	Points in one compute dispatch run in parallel. Opaque PSET writes have no
	destination read, so repeated opaque writes to one coordinate can retain only
	their last value until the next ordering boundary. A mixed or alpha sequence
	must retain every write because each alpha operation observes its predecessor.
	Those writes receive successive layer numbers and are separated into ordered
	GPU packets when the stream is flushed.

	The hash stores a one-based point index alongside each coordinate key. This
	avoids an O(n) point-array search when a tight BASIC PSET loop revisits a
	pixel. A full table remains a bounded condition: it flushes before appending
	more work rather than relying on an unbounded probe.
*/
static int compat_queue_pending_point_locked(FB_GFX3_DRAW_STATE *state,
	FB_GFX3_MODE *mode, const FB_GFX3_POINT *point)
{
	uint64_t key;
	uint32_t point_layer = 0u;
	uint32_t point_index;
	uint32_t slot;
	int result;

	if ((state == NULL) || (mode == NULL) || (point == NULL) ||
	    (state->pending_points == NULL) ||
	    (state->pending_point_submission == NULL) ||
	    (state->pending_point_layers == NULL) ||
	    (state->pending_point_layer_offsets == NULL) ||
	    (state->pending_points_capacity == 0) ||
	    (state->pending_point_keys == NULL) ||
	    (state->pending_point_indices == NULL) ||
	    (state->pending_point_generations == NULL) ||
	    (state->pending_point_generation == 0u) ||
	    (state->pending_point_key_capacity == 0))
		return FB_GFX3_UNSUPPORTED;
	if ((state->pending_points_count != 0) &&
	    ((state->pending_points_page != state->work_page) ||
	     (memcmp(&state->pending_points_clip, &state->view,
	      sizeof(state->view)) != 0))) {
		result = compat_flush_pending_points_locked(state, mode);
		if (result != FB_GFX3_OK)
			return result;
	}
	key = (((uint64_t)(uint32_t)point->x << 32) | (uint32_t)point->y) + 1u;
	if ((state->pending_points_count >= state->pending_points_capacity) ||
	    (state->pending_points_count >= state->pending_point_key_capacity)) {
		result = compat_flush_pending_points_locked(state, mode);
		if (result != FB_GFX3_OK)
			return result;
	}
	slot = ((uint32_t)key ^ (uint32_t)(key >> 32) * 2654435761u) &
		(state->pending_point_key_capacity - 1u);
	while ((state->pending_point_generations[slot] ==
	        state->pending_point_generation) &&
	       (state->pending_point_keys[slot] != key))
		slot = (slot + 1u) & (state->pending_point_key_capacity - 1u);
	if ((state->pending_point_generations[slot] ==
	     state->pending_point_generation) &&
	    (state->pending_point_keys[slot] == key)) {
		point_index = state->pending_point_indices[slot];
		if ((point_index == 0u) ||
		    (point_index > state->pending_points_count))
			return FB_GFX3_INVALID;
		/*
			Opaque writes commute with every other opaque write except the
			previous write to this exact coordinate. Replacing that record keeps
			the last PSET result without creating a parallel write race.
		*/
		if ((point->flags == 0u) &&
		    (state->pending_points[point_index - 1u].flags == 0u)) {
			state->pending_points[point_index - 1u] = *point;
			return FB_GFX3_OK;
		}
		point_layer = state->pending_point_layers[point_index - 1u] + 1u;
		if (point_layer == 0u)
			return FB_GFX3_INVALID;
	} else if ((state->pending_points_count == state->pending_points_capacity) ||
	           (state->pending_point_generations[slot] ==
	            state->pending_point_generation)) {
		result = compat_flush_pending_points_locked(state, mode);
		if (result != FB_GFX3_OK)
			return result;
		slot = ((uint32_t)key ^ (uint32_t)(key >> 32) * 2654435761u) &
			(state->pending_point_key_capacity - 1u);
	}
	if (state->pending_points_count == 0) {
		state->pending_points_page = state->work_page;
		state->pending_points_clip = state->view;
	}
	state->pending_points[state->pending_points_count] = *point;
	state->pending_point_layers[state->pending_points_count] = point_layer;
	state->pending_points_count++;
	if (point_layer > state->pending_point_max_layer)
		state->pending_point_max_layer = point_layer;
	state->pending_point_keys[slot] = key;
	state->pending_point_indices[slot] = state->pending_points_count;
	state->pending_point_generations[slot] =
		state->pending_point_generation;
	return FB_GFX3_OK;
}

static void compat_invalidate_work_shadow(FB_GFX3_MODE *mode,
	const FB_GFX3_DRAW_STATE *state, const char *reason);

/*
	Some older games express every logical pixel as a small opaque filled box.
	Submitting a separate rectangle command for each box makes the render thread
	spend more time dispatching work than drawing it.  Reuse the PSET batch for
	those boxes by expanding only small, solid, non-alpha rectangles to points.

	This deliberately does not cover alpha paths.  A SCREENLOCK which has
	exposed or modified CPU pixels reaches the authoritative-shadow path before
	this helper. A lock used only to group ordinary drawing remains GPU-owned
	and can use the same point batch as an unlocked frame.

	The caller owns mode->mutex.
*/
static int compat_queue_small_opaque_rectangle_locked(
	FB_GFX3_DRAW_STATE *state, FB_GFX3_MODE *mode, int x1, int y1,
	int x2, int y2, uint32_t color, uint32_t flags)
{
	FB_GFX3_POINT points[64];
	uint32_t point_flags;
	uint32_t count = 0;
	int x;
	int y;
	int result;

	if (((flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0) ||
	    (state->pending_points == NULL) ||
	    (state->pending_points_capacity == 0))
		return FB_GFX3_UNSUPPORTED;
	if ((x2 < x1) || (y2 < y1) ||
	    ((uint32_t)(x2 - x1 + 1) > 8u) ||
	    ((uint32_t)(y2 - y1 + 1) > 8u))
		return FB_GFX3_UNSUPPORTED;

	point_flags = flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			points[count].x = x;
			points[count].y = y;
			points[count].color = color;
			points[count].flags = point_flags;
			count++;
		}
	}
	if ((count == 0) || (count > state->pending_points_capacity))
		return FB_GFX3_UNSUPPORTED;
	for (x = 0; x < (int)count; x++) {
		result = compat_queue_pending_point_locked(state, mode, &points[x]);
		if (result != FB_GFX3_OK)
			return result;
	}
	/*
		The point batch remains the GPU rendering path. If a POINT loop has
		already created a coherent shadow, mirror these at-most 64 pixels now so
		the next software-alpha glyph does not download the complete page.
	*/
	result = compat_mirror_shadow_rectangle(state, mode, x1, y1,
		x2, y2, color, 0xFFFFu, TRUE, flags);
	if (result != FB_GFX3_OK)
		compat_invalidate_work_shadow(mode, state,
			"small opaque rectangle batch");
	return FB_GFX3_OK;
}

int fb_gfx3_compat_flush_points(FB_GFX3_DRAW_STATE *state)
{
	FB_GFX3_MODE *mode = compat_state_lock(state);
	int result;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	result = compat_flush_pending_points_locked(state, mode);
	fb_MutexUnlock(mode->mutex);
	return result;
}

int fb_gfx3_compat_flush_points_graphics_locked(FB_GFX3_DRAW_STATE *state)
{
	FB_GFX3_MODE *mode;

	/*
		The public graphics entry points serialize mode lifetime and page-state
		changes with FB_GRAPHICS_LOCK.  PSET's own fast path uses the same
		validation rule, allowing a following PUT or GET to flush its local
		point stream without an otherwise redundant mode-mutex round trip.
	*/
	mode = compat_state_graphics_locked(state);
	if (mode == NULL)
		return FB_GFX3_INVALID;
	return compat_flush_pending_points_locked(state, mode);
}

int fb_gfx3_compat_commit_shadow(FB_GFX3_DRAW_STATE *state)
{
	FB_GFX3_MODE *mode = compat_state_lock(state);
	int result;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	result = compat_commit_locked_shadow(state, mode);
	fb_MutexUnlock(mode->mutex);
	return result;
}

static FB_GFX3_SURFACE *compat_work_surface(FB_GFX3_MODE *mode,
	const FB_GFX3_DRAW_STATE *state)
{
	return &mode->pages[state->work_page];
}

static void compat_invalidate_work_shadow(FB_GFX3_MODE *mode,
	const FB_GFX3_DRAW_STATE *state, const char *reason)
{
	if ((mode->shadow_valid != NULL) && (state->work_page < mode->page_count)) {
		mode->shadow_valid[state->work_page] = FALSE;
		fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_TRACE,
			"CPU shadow invalidated: page %u, reason %s",
			state->work_page, (reason != NULL) ? reason : "unspecified");
	}
}

/*
	POINT must ordinarily wait for GPU visibility. Keep only one exact cached
	coordinate per page, and invalidate it before submitting any operation that
	could cover that coordinate. A rectangle outside the coordinate therefore
	cannot turn a later POINT into an unnecessary full GPU readback.

	The helpers are deliberately conservative: malformed or overflowed bounds
	discard the cache rather than attempting to infer coverage.
*/
static void compat_invalidate_point_cache_rect_locked(FB_GFX3_MODE *mode,
	uint32_t page, int x1, int y1, int x2, int y2)
{
	FB_GFX3_POINT_CACHE *cache;

	if ((mode == NULL) || (mode->point_cache == NULL) ||
	    (page >= mode->page_count))
		return;
	cache = &mode->point_cache[page];
	if ((x1 > x2) || (y1 > y2)) {
		cache->valid = FALSE;
		cache->read_modify_pairs = 0;
		return;
	}
	if (cache->valid && (cache->x >= x1) && (cache->x <= x2) &&
	    (cache->y >= y1) && (cache->y <= y2)) {
		cache->valid = FALSE;
		cache->read_modify_pairs = 0;
	}
}

static void compat_invalidate_work_point_cache_locked(FB_GFX3_MODE *mode,
	const FB_GFX3_DRAW_STATE *state, int x1, int y1, int x2, int y2)
{
	if ((state == NULL) || (mode == NULL))
		return;
	compat_invalidate_point_cache_rect_locked(mode, state->work_page,
		x1, y1, x2, y2);
}

void fb_gfx3_compat_invalidate_point_cache_rect_graphics_locked(
	FB_GFX3_DRAW_STATE *state, int x1, int y1, int x2, int y2)
{
	FB_GFX3_MODE *mode = compat_state_graphics_locked(state);

	if (mode != NULL)
		compat_invalidate_work_point_cache_locked(mode, state, x1, y1, x2,
			y2);
}

void fb_gfx3_compat_invalidate_point_cache_graphics_locked(
	FB_GFX3_DRAW_STATE *state)
{
	FB_GFX3_MODE *mode = compat_state_graphics_locked(state);

	if ((mode != NULL) && (state->work_page < mode->page_count) &&
	    (mode->point_cache != NULL)) {
		mode->point_cache[state->work_page].valid = FALSE;
		mode->point_cache[state->work_page].read_modify_pairs = 0;
	}
}

/*
	POINT/PSET read-modify-write loops are a common software-rendering pattern.
	Issuing a GPU readback for every glyph or alpha-blended pixel would serialize
	the renderer thousands of times per frame. Keep that pattern in a page shadow
	and submit one upload at the next graphics ordering boundary.

	The public SCREENPTR ABI only promises directly-addressable true-colour
	pixels. Keep this fast path equally narrow; indexed modes continue through the
	regular renderer commands until their packed storage is implemented. The
	caller owns either mode->mutex or the runtime-wide graphics lock.
*/
static int compat_prepare_truecolor_shadow_storage(FB_GFX3_DRAW_STATE *state,
	FB_GFX3_MODE *mode, uint32_t *prepared_page, size_t *prepared_size)
{
	uint32_t page;
	uint32_t bytes_per_pixel;
	uint64_t row_bytes;
	size_t allocation_size;

	if ((state == NULL) || (mode == NULL) ||
	    ((mode->depth != 16) && (mode->depth != 32)) ||
	    (mode->width == 0u) || (mode->height == 0u) ||
	    (mode->shadow_pages == NULL) || (mode->shadow_valid == NULL) ||
	    (mode->shadow_dirty == NULL))
		return FB_GFX3_UNSUPPORTED;
	page = state->work_page;
	bytes_per_pixel = (mode->depth == 16) ? sizeof(uint16_t) :
		sizeof(uint32_t);
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
	if (prepared_page != NULL)
		*prepared_page = page;
	if (prepared_size != NULL)
		*prepared_size = allocation_size;
	return FB_GFX3_OK;
}

static void compat_clear_shadow_dirty(FB_GFX3_MODE *mode, uint32_t page)
{
	if ((mode == NULL) || (page >= mode->page_count))
		return;
	if (mode->shadow_dirty != NULL)
		mode->shadow_dirty[page] = FALSE;
	if (mode->shadow_dirty_first_line != NULL)
		mode->shadow_dirty_first_line[page] = UINT32_MAX;
	if (mode->shadow_dirty_last_line != NULL)
		mode->shadow_dirty_last_line[page] = 0u;
}

static void compat_mark_shadow_dirty(FB_GFX3_MODE *mode, uint32_t page,
	uint32_t first_line, uint32_t last_line)
{
	if ((mode == NULL) || (page >= mode->page_count) ||
	    (first_line > last_line) || (last_line >= mode->height) ||
	    (mode->shadow_dirty == NULL))
		return;
	if (!mode->shadow_dirty[page]) {
		if (mode->shadow_dirty_first_line != NULL)
			mode->shadow_dirty_first_line[page] = first_line;
		if (mode->shadow_dirty_last_line != NULL)
			mode->shadow_dirty_last_line[page] = last_line;
	} else {
		if ((mode->shadow_dirty_first_line != NULL) &&
		    (first_line < mode->shadow_dirty_first_line[page]))
			mode->shadow_dirty_first_line[page] = first_line;
		if ((mode->shadow_dirty_last_line != NULL) &&
		    (last_line > mode->shadow_dirty_last_line[page]))
			mode->shadow_dirty_last_line[page] = last_line;
	}
	mode->shadow_dirty[page] = TRUE;
}

static int compat_ensure_truecolor_shadow(FB_GFX3_DRAW_STATE *state,
	FB_GFX3_MODE *mode, int require_screen_lock)
{
	uint32_t page;
	int result;

	if (require_screen_lock && ((mode->access_lock_count == 0) ||
	    (mode->depth != 32)))
		return FB_GFX3_UNSUPPORTED;
	result = compat_prepare_truecolor_shadow_storage(state, mode, &page, NULL);
	if (result != FB_GFX3_OK)
		return result;
	if (mode->shadow_valid[page])
		return FB_GFX3_OK;
	result = fb_gfx3_surface_download(&mode->pages[page], 0, 0,
		mode->width, mode->height, mode->shadow_pitch,
		mode->shadow_pages[page]);
	if (result == FB_GFX3_OK)
		mode->shadow_valid[page] = TRUE;
	return result;
}

int fb_gfx3_compat_replace_shadow_after_full_clear_graphics_locked(
	FB_GFX3_DRAW_STATE *state, uint32_t color)
{
	FB_GFX3_MODE *mode = compat_state_graphics_locked(state);
	uint32_t page;
	size_t pixel_count;
	size_t index;
	int result;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	page = state->work_page;
	if (page >= mode->page_count)
		return FB_GFX3_INVALID;
	/*
		The GPU clear supersedes every cached POINT result even when no shadow
		has ever been allocated. Keep this before the no-shadow fast return.
	*/
	if (mode->point_cache != NULL) {
		mode->point_cache[page].valid = FALSE;
		mode->point_cache[page].read_modify_pairs = 0u;
	}
	/*
		Do not allocate a CPU mirror for ordinary GPU-only CLS calls. A live
		SCREENLOCK may immediately use POINT/PSET as a software rasterizer, while
		an already allocated page may still be referenced by SCREENPTR.
	*/
	if ((mode->access_lock_count == 0u) &&
	    ((mode->shadow_pages == NULL) ||
	     (mode->shadow_pages[page] == NULL))) {
		if (mode->shadow_valid != NULL)
			mode->shadow_valid[page] = FALSE;
		compat_clear_shadow_dirty(mode, page);
		return FB_GFX3_OK;
	}
	result = compat_prepare_truecolor_shadow_storage(state, mode, &page, NULL);
	if (result != FB_GFX3_OK) {
		/*
			The GPU clear has already superseded the old page. Never leave a
			stale dirty marker which could upload the pre-clear pixels later.
		*/
		if (mode->shadow_valid != NULL)
			mode->shadow_valid[page] = FALSE;
		compat_clear_shadow_dirty(mode, page);
		return result;
	}
	if (fb_gfx3_size_multiply(mode->width, mode->height,
	    &pixel_count) != FB_GFX3_OK) {
		mode->shadow_valid[page] = FALSE;
		compat_clear_shadow_dirty(mode, page);
		return FB_GFX3_INVALID;
	}
	if (mode->depth == 16) {
		uint16_t *pixels = (uint16_t *)mode->shadow_pages[page];
		uint16_t packed_color = (uint16_t)color;

		for (index = 0u; index < pixel_count; ++index)
			pixels[index] = packed_color;
	} else {
		uint32_t *pixels = (uint32_t *)mode->shadow_pages[page];

		for (index = 0u; index < pixel_count; ++index)
			pixels[index] = color;
	}
	mode->shadow_valid[page] = TRUE;
	/*
		A dirty marker under SCREENLOCK may represent a SCREENPTR which remains
		writable after CLS. Preserve it until SCREENUNLOCK. Without a live pointer,
		the full clear has made every older CPU edit irrelevant.
	*/
	if (mode->access_lock_count == 0u)
		compat_clear_shadow_dirty(mode, page);
	return FB_GFX3_OK;
}

static int compat_ensure_locked_truecolor_shadow(FB_GFX3_DRAW_STATE *state,
	FB_GFX3_MODE *mode)
{
	return compat_ensure_truecolor_shadow(state, mode, TRUE);
}

static void compat_order_int_coordinates(int *x1, int *y1, int *x2, int *y2);

static void compat_store_shadow_pixel(FB_GFX3_MODE *mode, uint32_t page,
	int x, int y, uint32_t color)
{
	unsigned char *row = mode->shadow_pages[page] +
		(uint32_t)y * mode->shadow_pitch;

	if (mode->depth == 16) {
		uint16_t packed_color = (uint16_t)color;

		memcpy(row + (uint32_t)x * sizeof(packed_color), &packed_color,
			sizeof(packed_color));
	} else {
		memcpy(row + (uint32_t)x * sizeof(color), &color, sizeof(color));
	}
}

/*
	A GPU rectangle can be reflected into an already valid CPU shadow without a
	readback. The GPU remains the renderer; this small coherency update prevents
	the next POINT loop from downloading an entire page merely to learn the same
	box. Alpha rectangles repeat only their blend calculation in the existing
	shadow and remain shader-rendered on the device.

	The caller owns mode->mutex.
*/
static int compat_mirror_shadow_rectangle(FB_GFX3_DRAW_STATE *state,
	FB_GFX3_MODE *mode, int x1, int y1, int x2, int y2, uint32_t color,
	uint32_t style, int filled, uint32_t flags)
{
	uint64_t width;
	uint64_t height;
	uint64_t phase;
	uint32_t page;
	int clipped_x1;
	int clipped_y1;
	int clipped_x2;
	int clipped_y2;
	int x;
	int y;

	if ((state == NULL) || (mode == NULL) ||
	    ((mode->depth != 16) && (mode->depth != 32)) ||
	    (((flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) &&
	     (mode->depth != 32)))
		return FB_GFX3_UNSUPPORTED;
	page = state->work_page;
	if ((page >= mode->page_count) || (mode->shadow_pages == NULL) ||
	    (mode->shadow_valid == NULL) || !mode->shadow_valid[page] ||
	    (mode->shadow_pages[page] == NULL) || (mode->shadow_pitch == 0u))
		return FB_GFX3_UNSUPPORTED;
	compat_order_int_coordinates(&x1, &y1, &x2, &y2);
	width = (uint64_t)((int64_t)x2 - (int64_t)x1) + 1u;
	height = (uint64_t)((int64_t)y2 - (int64_t)y1) + 1u;
	clipped_x1 = MAX(x1, state->view.x1);
	clipped_y1 = MAX(y1, state->view.y1);
	clipped_x2 = MIN(x2, state->view.x2);
	clipped_y2 = MIN(y2, state->view.y2);
	if ((clipped_x1 > clipped_x2) || (clipped_y1 > clipped_y2))
		return FB_GFX3_OK;
	if (filled) {
		for (y = clipped_y1; y <= clipped_y2; y++) {
			for (x = clipped_x1; x <= clipped_x2; x++)
				compat_write_shadow_primitive_pixel(mode, page, x, y,
					color, flags);
		}
		return FB_GFX3_OK;
	}
	style &= 0xFFFFu;
	if ((y2 >= clipped_y1) && (y2 <= clipped_y2)) {
		for (x = clipped_x1; x <= clipped_x2; x++) {
			phase = (uint64_t)((int64_t)x - (int64_t)x1);
			if ((style & (0x8000u >> (phase & 15u))) != 0u)
				compat_write_shadow_primitive_pixel(mode, page, x, y2,
					color, flags);
		}
	}
	if ((y1 >= clipped_y1) && (y1 <= clipped_y2)) {
		for (x = clipped_x1; x <= clipped_x2; x++) {
			phase = width +
				(uint64_t)((int64_t)x - (int64_t)x1);
			if ((style & (0x8000u >> (phase & 15u))) != 0u)
				compat_write_shadow_primitive_pixel(mode, page, x, y1,
					color, flags);
		}
	}
	if ((x2 >= clipped_x1) && (x2 <= clipped_x2)) {
		for (y = clipped_y1; y <= clipped_y2; y++) {
			phase = (width * 2u) +
				(uint64_t)((int64_t)y - (int64_t)y1);
			if ((style & (0x8000u >> (phase & 15u))) != 0u)
				compat_write_shadow_primitive_pixel(mode, page, x2, y,
					color, flags);
		}
	}
	if ((x1 >= clipped_x1) && (x1 <= clipped_x2)) {
		for (y = clipped_y1; y <= clipped_y2; y++) {
			phase = (width * 2u) + height +
				(uint64_t)((int64_t)y - (int64_t)y1);
			if ((style & (0x8000u >> (phase & 15u))) != 0u)
				compat_write_shadow_primitive_pixel(mode, page, x1, y,
					color, flags);
		}
	}
	return FB_GFX3_OK;
}

/*
	A dirty shadow inside SCREENLOCK is the current source of truth. This occurs
	after SCREENPTR writes or a POINT/PSET read-modify-write sequence. Uploading
	that shadow before every following primitive would alternate page ownership
	between the CPU and GPU, which is substantially slower than either renderer.

	Opaque primitives which do not depend on their old destination can instead
	be rasterized into the already authoritative shadow. SCREENUNLOCK then
	uploads the accumulated result once. GPU-only locks never enter this path.
*/
static int compat_shadow_is_authoritative(const FB_GFX3_DRAW_STATE *state,
	const FB_GFX3_MODE *mode)
{
	uint32_t page;

	if ((state == NULL) || (mode == NULL) ||
	    (mode->access_lock_count == 0u))
		return FALSE;
	page = state->work_page;
	return (page < mode->page_count) &&
		(mode->shadow_pages != NULL) &&
		(mode->shadow_pages[page] != NULL) &&
		(mode->shadow_valid != NULL) &&
		mode->shadow_valid[page] &&
		(mode->shadow_dirty != NULL) &&
		mode->shadow_dirty[page] &&
		(mode->shadow_pitch != 0u);
}

/*
	A POINT/PSET loop can make a shadow dirty without a public SCREENLOCK.
	Until that shadow is uploaded it is the newest version of the page, so a
	following PSET must join it instead of forcing an upload followed by another
	GPU-to-CPU download. Public SCREENPTR writes use the same ownership rule.
*/
static int compat_shadow_has_pending_writes(
	const FB_GFX3_DRAW_STATE *state, const FB_GFX3_MODE *mode)
{
	uint32_t page;

	if ((state == NULL) || (mode == NULL))
		return FALSE;
	page = state->work_page;
	return (page < mode->page_count) &&
		(mode->shadow_pages != NULL) &&
		(mode->shadow_pages[page] != NULL) &&
		(mode->shadow_valid != NULL) &&
		mode->shadow_valid[page] &&
		(mode->shadow_dirty != NULL) &&
		mode->shadow_dirty[page] &&
		(mode->shadow_pitch != 0u);
}

static void compat_write_shadow_primitive_pixel(FB_GFX3_MODE *mode,
	uint32_t page, int x, int y, uint32_t color, uint32_t flags)
{
	if ((flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) {
		unsigned char *address;
		uint32_t destination;

		/*
			Alpha primitives are only defined for the 32-bit compatibility
			surface. The caller rejects this path for other depths.
		*/
		address = mode->shadow_pages[page] +
			(uint32_t)y * mode->shadow_pitch +
			(uint32_t)x * sizeof(uint32_t);
		memcpy(&destination, address, sizeof(destination));
		color = compat_alpha_primitive_pixel(color, destination);
		memcpy(address, &color, sizeof(color));
		return;
	}
	compat_store_shadow_pixel(mode, page, x, y, color);
}

static int compat_render_shadow_line(
	FB_GFX3_DRAW_STATE *state, FB_GFX3_MODE *mode, int x1, int y1,
	int x2, int y2, uint32_t color, uint32_t style, uint32_t flags,
	int mark_dirty)
{
	int64_t difference_x;
	int64_t difference_y;
	int64_t direction_x;
	int64_t direction_y;
	int64_t point_count;
	int64_t index;
	int first_changed_y = INT_MAX;
	int last_changed_y = INT_MIN;
	uint32_t page;

	if (mark_dirty) {
		if (!compat_shadow_is_authoritative(state, mode))
			return FB_GFX3_UNSUPPORTED;
	} else if ((state == NULL) || (mode == NULL) ||
	    (state->work_page >= mode->page_count) ||
	    (mode->shadow_pages == NULL) ||
	    (mode->shadow_pages[state->work_page] == NULL) ||
	    (mode->shadow_valid == NULL) ||
	    !mode->shadow_valid[state->work_page] ||
	    (mode->shadow_pitch == 0u)) {
		return FB_GFX3_UNSUPPORTED;
	}
	if (((flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) &&
	    (mode->depth != 32))
		return FB_GFX3_UNSUPPORTED;
	page = state->work_page;
	difference_x = llabs((int64_t)x2 - (int64_t)x1);
	difference_y = llabs((int64_t)y2 - (int64_t)y1);
	direction_x = (x2 < x1) ? -1 : 1;
	direction_y = (y2 < y1) ? -1 : 1;
	point_count = MAX(difference_x, difference_y) + 1;
	style &= 0xFFFFu;

	for (index = 0; index < point_count; ++index) {
		int64_t position_x;
		int64_t position_y;

		if (difference_x >= difference_y) {
			position_x = (int64_t)x1 + direction_x * index;
			position_y = y1;
			if (difference_x != 0) {
				position_y += direction_y *
					((difference_y * index +
					  difference_x / 2) / difference_x);
			}
		} else {
			position_y = (int64_t)y1 + direction_y * index;
			position_x = (int64_t)x1 + direction_x *
				((difference_x * index +
				  difference_y / 2) / difference_y);
		}
		if ((position_x < state->view.x1) ||
		    (position_y < state->view.y1) ||
		    (position_x > state->view.x2) ||
		    (position_y > state->view.y2) ||
		    ((style & (0x8000u >> ((uint64_t)index & 15u))) == 0u))
			continue;
		compat_write_shadow_primitive_pixel(mode, page, (int)position_x,
			(int)position_y, color, flags);
		if (position_y < first_changed_y)
			first_changed_y = (int)position_y;
		if (position_y > last_changed_y)
			last_changed_y = (int)position_y;
	}
	if (mark_dirty && (first_changed_y <= last_changed_y))
		compat_mark_shadow_dirty(mode, page, (uint32_t)first_changed_y,
			(uint32_t)last_changed_y);
	return FB_GFX3_OK;
}

static void compat_render_shadow_ellipse_scanline(
	FB_GFX3_DRAW_STATE *state, FB_GFX3_MODE *mode, uint32_t page,
	int y, int x1, int x2, uint32_t color, int filled, uint32_t flags,
	int *first_changed_y, int *last_changed_y)
{
	int first;
	int last;
	int x;

	if ((y < state->view.y1) || (y > state->view.y2))
		return;
	if (filled) {
		first = MAX(x1, state->view.x1);
		last = MIN(x2, state->view.x2);
		for (x = first; x <= last; ++x)
			compat_write_shadow_primitive_pixel(mode, page, x, y, color,
				flags);
		if (first > last)
			return;
	} else {
		int wrote_pixel = FALSE;

		if ((x1 >= state->view.x1) && (x1 <= state->view.x2)) {
			compat_write_shadow_primitive_pixel(mode, page, x1, y, color,
				flags);
			wrote_pixel = TRUE;
		}
		if ((x2 >= state->view.x1) && (x2 <= state->view.x2)) {
			/*
				The GPU midpoint path writes both endpoints even when they
				coincide. This matters for alpha primitives.
			*/
			compat_write_shadow_primitive_pixel(mode, page, x2, y, color,
				flags);
			wrote_pixel = TRUE;
		}
		if (!wrote_pixel)
			return;
	}
	if (y < *first_changed_y)
		*first_changed_y = y;
	if (y > *last_changed_y)
		*last_changed_y = y;
}

static int compat_render_authoritative_shadow_ellipse(
	FB_GFX3_DRAW_STATE *state, FB_GFX3_MODE *mode, int center_x,
	int center_y, float radius_x, float radius_y, uint32_t color,
	int filled, uint32_t flags)
{
	double aq;
	double bq;
	double dx;
	double dy;
	double r;
	double rx;
	double ry;
	int x1;
	int x2;
	int y1;
	int y2;
	int d;
	int first_changed_y = INT_MAX;
	int last_changed_y = INT_MIN;
	uint32_t page;

	if (!compat_shadow_is_authoritative(state, mode))
		return FB_GFX3_UNSUPPORTED;
	if (((flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) &&
	    (mode->depth != 32))
		return FB_GFX3_UNSUPPORTED;
	page = state->work_page;
	x1 = (int)((float)center_x - radius_x);
	x2 = (int)((float)center_x + radius_x);
	y1 = center_y;
	y2 = center_y;
	if (radius_y == 0.0f) {
		compat_render_shadow_ellipse_scanline(state, mode, page, y1, x1,
			x2, color, TRUE, flags, &first_changed_y,
			&last_changed_y);
		goto done;
	}
	compat_render_shadow_ellipse_scanline(state, mode, page, y1, x1, x2,
		color, filled, flags, &first_changed_y, &last_changed_y);
	/*
		Keep the float multiplication before conversion to double. This mirrors
		the shader and gfxlib2 midpoint intermediates at large radii.
	*/
	aq = trunc((double)(radius_x * radius_x));
	bq = trunc((double)(radius_y * radius_y));
	dx = aq * 2.0;
	dy = bq * 2.0;
	r = trunc((double)(radius_x * (float)bq));
	rx = r * 2.0;
	ry = 0.0;
	d = (int)radius_x;
	while (d > 0) {
		if (r > 0.0) {
			y1++;
			y2--;
			ry += dx;
			r -= ry;
		}
		if (r <= 0.0) {
			d--;
			x1++;
			x2--;
			rx -= dy;
			r += rx;
		}
		compat_render_shadow_ellipse_scanline(state, mode, page, y1, x1,
			x2, color, filled, flags, &first_changed_y,
			&last_changed_y);
		compat_render_shadow_ellipse_scanline(state, mode, page, y2, x1,
			x2, color, filled, flags, &first_changed_y,
			&last_changed_y);
	}

done:
	if (first_changed_y <= last_changed_y)
		compat_mark_shadow_dirty(mode, page, (uint32_t)first_changed_y,
			(uint32_t)last_changed_y);
	return FB_GFX3_OK;
}

static int compat_write_truecolor_shadow_point(FB_GFX3_DRAW_STATE *state,
	FB_GFX3_MODE *mode, FB_GFX3_POINT *point)
{
	uint32_t offset;
	uint32_t bytes_per_pixel;

	if ((state == NULL) || (mode == NULL) || (point == NULL) ||
	    ((mode->depth != 16) && (mode->depth != 32)) ||
	    (state->work_page >= mode->page_count) ||
	    (mode->shadow_pages == NULL) ||
	    (mode->shadow_pages[state->work_page] == NULL) ||
	    (mode->shadow_valid == NULL) ||
	    !mode->shadow_valid[state->work_page] ||
	    (mode->shadow_dirty == NULL) || (mode->shadow_pitch == 0))
		return FB_GFX3_INVALID;
	bytes_per_pixel = (mode->depth == 16) ? sizeof(uint16_t) :
		sizeof(uint32_t);
	offset = (uint32_t)point->y * mode->shadow_pitch +
		(uint32_t)point->x * bytes_per_pixel;
	if (mode->depth == 16) {
		uint16_t packed_color = (uint16_t)point->color;

		memcpy(mode->shadow_pages[state->work_page] + offset, &packed_color,
			sizeof(packed_color));
	} else {
		uint32_t destination;

		memcpy(&destination, mode->shadow_pages[state->work_page] + offset,
			sizeof(destination));
		if ((point->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0)
			point->color = compat_alpha_primitive_pixel(point->color,
				destination);
		memcpy(mode->shadow_pages[state->work_page] + offset, &point->color,
			sizeof(point->color));
	}
	compat_mark_shadow_dirty(mode, state->work_page, (uint32_t)point->y,
		(uint32_t)point->y);
	return FB_GFX3_OK;
}

/*
	Return the shadow to GPU ownership before a primitive that follows CPU
	pixel writes.  This preserves BASIC command order for mixed SCREENLOCK
	frames such as PSET, LINE, PSET without making normal primitive rendering
	fall back to the CPU.

	The caller owns mode->mutex.
*/
static int compat_commit_locked_shadow(FB_GFX3_DRAW_STATE *state,
	FB_GFX3_MODE *mode)
{
	uint32_t page = state->work_page;
	uint32_t first_line = 0u;
	uint32_t last_line;
	uint32_t height;
	const unsigned char *pixels;
	int result;

	if (((mode->depth != 16) && (mode->depth != 32)) ||
	    (page >= mode->page_count) ||
	    (mode->shadow_dirty == NULL) ||
	    !mode->shadow_dirty[page])
		return FB_GFX3_OK;
	if ((mode->shadow_pages == NULL) || (mode->shadow_pages[page] == NULL) ||
	    (mode->shadow_pitch == 0))
		return FB_GFX3_INVALID;
	last_line = mode->height - 1u;
	if ((mode->shadow_dirty_first_line != NULL) &&
	    (mode->shadow_dirty_last_line != NULL) &&
	    (mode->shadow_dirty_first_line[page] <=
	     mode->shadow_dirty_last_line[page]) &&
	    (mode->shadow_dirty_last_line[page] < mode->height)) {
		first_line = mode->shadow_dirty_first_line[page];
		last_line = mode->shadow_dirty_last_line[page];
	}
	height = last_line - first_line + 1u;
	pixels = mode->shadow_pages[page] +
		(size_t)first_line * mode->shadow_pitch;
	fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_TRACE,
		"SCREENLOCK shadow upload begin: page %u, rows %u-%u", page,
		first_line, last_line);
	result = fb_gfx3_surface_upload(&mode->pages[page], 0, (int)first_line,
		mode->width, height, mode->shadow_pitch, pixels);
	if (result != FB_GFX3_OK)
		return result;
	fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_TRACE,
		"SCREENLOCK shadow upload queued: page %u", page);
	compat_clear_shadow_dirty(mode, page);
	mode->shadow_valid[page] = TRUE;
	return FB_GFX3_OK;
}

static void compat_fix_relative(FB_GFX3_DRAW_STATE *state, uint32_t flags,
	float *x1, float *y1, float *x2, float *y2)
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

static int compat_translate(const FB_GFX3_DRAW_STATE *state, float x,
	float y, int *translated_x, int *translated_y)
{
	int view_width;
	int view_height;

	if ((translated_x == NULL) || (translated_y == NULL))
		return FB_GFX3_INVALID;
	view_width = state->view.x2 - state->view.x1 + 1;
	view_height = state->view.y2 - state->view.y1 + 1;
	if (state->flags & FB_GFX3_WINDOW_ACTIVE) {
		if ((state->window_width == 0.0f) ||
		    (state->window_height == 0.0f))
			return FB_GFX3_INVALID;
		x = ((x - state->window_x) * (float)(view_width - 1)) /
			state->window_width;
		y = ((y - state->window_y) * (float)(view_height - 1)) /
			state->window_height;
	}
	if (!isfinite(x) || !isfinite(y) ||
	    ((double)x < (double)INT_MIN + 1.0) ||
	    ((double)x > (double)INT_MAX - 1.0) ||
	    ((double)y < (double)INT_MIN + 1.0) ||
	    ((double)y > (double)INT_MAX - 1.0))
		return FB_GFX3_INVALID;

	*translated_x = CINT(x);
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

static int compat_inside_view(const FB_GFX3_DRAW_STATE *state, int x, int y)
{
	return (x >= state->view.x1) && (x <= state->view.x2) &&
		(y >= state->view.y1) && (y <= state->view.y2);
}

static void compat_order_int_coordinates(int *x1, int *y1, int *x2, int *y2)
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

static int compat_add_coordinate_offset(int coordinate, int offset)
{
	int64_t result = (int64_t)coordinate + offset;

	if (result < INT_MIN)
		return INT_MIN;
	if (result > INT_MAX)
		return INT_MAX;
	return (int)result;
}

/* ------------------------------------------------------------------------- */
/* Mode and caller-state lifecycle                                           */
/* ------------------------------------------------------------------------- */

int fb_gfx3_mode_init(FB_GFX3_MODE *mode,
	const FB_GFX3_CONTEXT_CONFIG *config, uint32_t clear_color)
{
	FB_GFX3_CONTEXT_CONFIG context_config;
	size_t allocation_size;
	uint32_t usage;
	const unsigned char *default_palette;
	uint32_t created_pages = 0;
	uint32_t console_font_height;
	uint32_t i;
	int result;

	if ((mode == NULL) || (config == NULL) || (config->backend == NULL) ||
	    (config->width == 0) || (config->height == 0) ||
	    (config->width > INT_MAX) || (config->height > INT_MAX) ||
	    (config->page_count == 0) || (config->page_count > 255))
		return FB_GFX3_INVALID;
	console_font_height = (config->console_font_height == 0u) ? 8u :
		config->console_font_height;
	if ((console_font_height != 8u) && (console_font_height != 14u) &&
	    (console_font_height != 16u))
		return FB_GFX3_INVALID;
	if (fb_gfx3_size_multiply(config->page_count, sizeof(mode->pages[0]),
	    &allocation_size) != FB_GFX3_OK)
		return FB_GFX3_INVALID;

	memset(mode, 0, sizeof(*mode));
	mode->mutex = fb_MutexCreate();
	if (mode->mutex == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	result = fb_gfx3_input_init(&mode->input, config->width,
		config->height);
	if (result != FB_GFX3_OK) {
		fb_MutexDestroy(mode->mutex);
		mode->mutex = NULL;
		return result;
	}
	mode->pages = (FB_GFX3_SURFACE *)calloc(1, allocation_size);
	if (mode->pages == NULL) {
		fb_gfx3_input_destroy(&mode->input);
		fb_MutexDestroy(mode->mutex);
		mode->mutex = NULL;
		return FB_GFX3_OUT_OF_MEMORY;
	}
	mode->shadow_pages = (unsigned char **)calloc(config->page_count,
		sizeof(mode->shadow_pages[0]));
	mode->shadow_snapshots = (unsigned char **)calloc(config->page_count,
		sizeof(mode->shadow_snapshots[0]));
	mode->shadow_valid = (unsigned char *)calloc(config->page_count,
		sizeof(mode->shadow_valid[0]));
	mode->shadow_dirty = (unsigned char *)calloc(config->page_count,
		sizeof(mode->shadow_dirty[0]));
	mode->shadow_dirty_first_line = (uint32_t *)malloc(
		config->page_count * sizeof(mode->shadow_dirty_first_line[0]));
	mode->shadow_dirty_last_line = (uint32_t *)calloc(config->page_count,
		sizeof(mode->shadow_dirty_last_line[0]));
	mode->shadow_snapshot_active = (unsigned char *)calloc(config->page_count,
		sizeof(mode->shadow_snapshot_active[0]));
	mode->point_cache = (FB_GFX3_POINT_CACHE *)calloc(config->page_count,
		sizeof(mode->point_cache[0]));
	if ((mode->shadow_pages == NULL) || (mode->shadow_snapshots == NULL) ||
	    (mode->shadow_valid == NULL) || (mode->shadow_dirty == NULL) ||
	    (mode->shadow_dirty_first_line == NULL) ||
	    (mode->shadow_dirty_last_line == NULL) ||
	    (mode->shadow_snapshot_active == NULL) || (mode->point_cache == NULL)) {
		free(mode->point_cache);
		mode->point_cache = NULL;
		free(mode->shadow_snapshot_active);
		free(mode->shadow_dirty_last_line);
		mode->shadow_dirty_last_line = NULL;
		free(mode->shadow_dirty_first_line);
		mode->shadow_dirty_first_line = NULL;
		free(mode->shadow_dirty);
		free(mode->shadow_valid);
		free((void *)mode->shadow_snapshots);
		free((void *)mode->shadow_pages);
		free(mode->pages);
		mode->pages = NULL;
		fb_gfx3_input_destroy(&mode->input);
		fb_MutexDestroy(mode->mutex);
		mode->mutex = NULL;
		return FB_GFX3_OUT_OF_MEMORY;
	}
	for (i = 0u; i < config->page_count; i++)
		mode->shadow_dirty_first_line[i] = UINT32_MAX;

	context_config = *config;
	context_config.platform = &mode->input;
	result = fb_gfx3_context_init(&mode->context, &context_config);
	if (result != FB_GFX3_OK)
		goto fail;
	usage = FB_GFX3_SURFACE_RENDER_TARGET | FB_GFX3_SURFACE_SAMPLED |
		FB_GFX3_SURFACE_TRANSFER_SOURCE |
		FB_GFX3_SURFACE_TRANSFER_DESTINATION;
	for (i = 0; i < config->page_count; i++) {
		result = fb_gfx3_surface_create(&mode->context, &mode->pages[i],
			config->width, config->height, config->depth, usage,
			clear_color);
		if (result != FB_GFX3_OK) {
			fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_ERROR,
				"%s page %u creation failed: %d", config->backend->name,
				(unsigned int)i, result);
			goto fail_surfaces;
		}
		created_pages++;
	}

	mode->generation = atomic_fetch_add_explicit(&next_mode_generation, 1,
		memory_order_relaxed);
	if (mode->generation == 0)
		mode->generation = atomic_fetch_add_explicit(&next_mode_generation, 1,
			memory_order_relaxed);
	mode->resize_serial = 1u;
	mode->width = config->width;
	mode->height = config->height;
	mode->depth = config->depth;
	mode->page_count = config->page_count;
	mode->console_font_width = 8u;
	mode->console_font_height = console_font_height;
	mode->console_rows = 0u;
	mode->visible_page = 0;
	mode->vga_palette_shift = 2u;
	mode->standard_mode = -1;
	/*
		GFX_NULL is the historical all-bits-set value. gfxlib2 treats it as a
		driver selector, not as every SCREEN option being enabled at once.
	*/
	mode->alpha_primitives = (config->flags != UINT32_MAX) &&
		((config->flags & FB_GFX3_MODE_ALPHA_PRIMITIVES) != 0);
	mode->resizable = (config->flags != UINT32_MAX) &&
		((config->flags & FB_GFX3_MODE_RESIZABLE) != 0);
	default_palette = fb_gfx3_data_palette_256();
	for (i = 0; i < 256; i++) {
		if (default_palette != NULL) {
			mode->palette[i] = default_palette[i * 3u] |
				((uint32_t)default_palette[i * 3u + 1u] << 8) |
				((uint32_t)default_palette[i * 3u + 2u] << 16);
		} else {
			mode->palette[i] = i | (i << 8) | (i << 16);
		}
	}
	memcpy(mode->default_palette, mode->palette,
		sizeof(mode->default_palette));
	memcpy(mode->standard_colors, mode->palette,
		sizeof(mode->standard_colors));
	mode->standard_foreground_color = 15u;
	result = fb_gfx3_context_set_palette(&mode->context, mode->palette);
	if (result != FB_GFX3_OK) {
		fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_ERROR,
			"%s initial palette failed: %d", config->backend->name,
			result);
		goto fail_surfaces;
	}
	result = fb_gfx3_surface_set_visible(&mode->pages[0]);
	if (result != FB_GFX3_OK) {
		fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_ERROR,
			"%s initial visible page failed: %d", config->backend->name,
			result);
		goto fail_surfaces;
	}
	/*
		A windowed GPU driver may defer swap-chain creation and pipeline setup
		until its first present.  Paying that one-time cost during SCREENRES
		keeps the first user drawing command from appearing spuriously slow and
		makes the API match gfxlib2's fully ready screen-return contract.
	*/
	result = fb_gfx3_surface_present(&mode->pages[0], TRUE);
	if (result != FB_GFX3_OK) {
		fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_ERROR,
			"%s initial presentation failed: %d", config->backend->name,
			result);
		goto fail_surfaces;
	}
	mode->initialized = TRUE;
	fb_gfx3_input_set_resizable(&mode->input, mode->resizable);
	return FB_GFX3_OK;

fail_surfaces:
	while (created_pages > 0) {
		created_pages--;
		fb_gfx3_surface_destroy(&mode->pages[created_pages]);
	}
	fb_gfx3_context_shutdown(&mode->context);
fail:
	free(mode->point_cache);
	mode->point_cache = NULL;
	free(mode->paint_queue);
	mode->paint_queue = NULL;
	free(mode->paint_visited);
	mode->paint_visited = NULL;
	mode->paint_scratch_capacity = 0;
	free(mode->shadow_snapshot_active);
	mode->shadow_snapshot_active = NULL;
	free(mode->shadow_dirty_last_line);
	mode->shadow_dirty_last_line = NULL;
	free(mode->shadow_dirty_first_line);
	mode->shadow_dirty_first_line = NULL;
	free(mode->shadow_dirty);
	mode->shadow_dirty = NULL;
	free(mode->shadow_valid);
	mode->shadow_valid = NULL;
	free((void *)mode->shadow_snapshots);
	mode->shadow_snapshots = NULL;
	free((void *)mode->shadow_pages);
	mode->shadow_pages = NULL;
	free(mode->pages);
	mode->pages = NULL;
	fb_gfx3_input_destroy(&mode->input);
	fb_MutexDestroy(mode->mutex);
	mode->mutex = NULL;
	return result;
}

int fb_gfx3_mode_shutdown(FB_GFX3_MODE *mode)
{
	FBMUTEX *mutex;
	uint32_t i;
	int first_result = FB_GFX3_OK;
	int result;

	if ((mode == NULL) || (mode->mutex == NULL))
		return FB_GFX3_INVALID;
	mutex = mode->mutex;
	fb_MutexLock(mutex);
	if (!mode->initialized) {
		fb_MutexUnlock(mutex);
		return FB_GFX3_INVALID;
	}
	mode->initialized = FALSE;
	for (i = mode->page_count; i > 0; i--) {
		result = fb_gfx3_surface_destroy(&mode->pages[i - 1]);
		if ((first_result == FB_GFX3_OK) && (result != FB_GFX3_OK))
			first_result = result;
	}
	for (i = 0; i < mode->page_count; i++)
		free(mode->shadow_pages[i]);
	for (i = 0; i < mode->page_count; i++)
		free(mode->shadow_snapshots[i]);
	free(mode->paint_queue);
	mode->paint_queue = NULL;
	free(mode->paint_visited);
	mode->paint_visited = NULL;
	mode->paint_scratch_capacity = 0;
	result = fb_gfx3_context_shutdown(&mode->context);
	if ((first_result == FB_GFX3_OK) && (result != FB_GFX3_OK))
		first_result = result;
	fb_gfx3_input_destroy(&mode->input);
	free(mode->pages);
	mode->pages = NULL;
	free(mode->point_cache);
	mode->point_cache = NULL;
	free(mode->shadow_dirty_last_line);
	mode->shadow_dirty_last_line = NULL;
	free(mode->shadow_dirty_first_line);
	mode->shadow_dirty_first_line = NULL;
	free(mode->shadow_dirty);
	mode->shadow_dirty = NULL;
	free(mode->shadow_valid);
	mode->shadow_valid = NULL;
	free(mode->shadow_snapshot_active);
	mode->shadow_snapshot_active = NULL;
	free((void *)mode->shadow_snapshots);
	mode->shadow_snapshots = NULL;
	free((void *)mode->shadow_pages);
	mode->shadow_pages = NULL;
	mode->page_count = 0;
	mode->mutex = NULL;
	fb_MutexUnlock(mutex);
	fb_MutexDestroy(mutex);
	return first_result;
}

/* ------------------------------------------------------------------------- */
/* Dynamic logical framebuffer resizing                                      */
/* ------------------------------------------------------------------------- */

/* The caller owns mode->mutex. */
static int compat_sync_resize_locked(FB_GFX3_DRAW_STATE *state,
	FB_GFX3_MODE *mode)
{
	int result;

	if ((state == NULL) || (mode == NULL) ||
	    (state->generation != mode->generation))
		return FB_GFX3_INVALID;
	if (state->resize_serial == mode->resize_serial)
		return FB_GFX3_OK;
	result = compat_flush_pending_points_locked(state, mode);
	if (result != FB_GFX3_OK)
		return result;
	if ((state->flags & FB_GFX3_VIEWPORT_SET) == 0u) {
		state->view.x1 = 0;
		state->view.y1 = 0;
		state->view.x2 = (int32_t)mode->width - 1;
		state->view.y2 = (int32_t)mode->height - 1;
	} else {
		state->view.x1 = MAX(state->view.x1, 0);
		state->view.y1 = MAX(state->view.y1, 0);
		state->view.x2 = MIN(state->view.x2, (int32_t)mode->width - 1);
		state->view.y2 = MIN(state->view.y2, (int32_t)mode->height - 1);
		if ((state->view.x2 < state->view.x1) ||
		    (state->view.y2 < state->view.y1)) {
			state->flags &= ~(uint32_t)FB_GFX3_VIEWPORT_SET;
			state->view.x1 = 0;
			state->view.y1 = 0;
			state->view.x2 = (int32_t)mode->width - 1;
			state->view.y2 = (int32_t)mode->height - 1;
		}
	}
	state->resize_serial = mode->resize_serial;
	return FB_GFX3_OK;
}

int fb_gfx3_draw_state_sync_resize(FB_GFX3_DRAW_STATE *state)
{
	FB_GFX3_MODE *mode;
	int result;

	if ((state == NULL) || (state->mode == NULL) ||
	    (state->mode->mutex == NULL))
		return FB_GFX3_INVALID;
	mode = state->mode;
	fb_MutexLock(mode->mutex);
	result = mode->initialized ? compat_sync_resize_locked(state, mode) :
		FB_GFX3_CLOSED;
	fb_MutexUnlock(mode->mutex);
	return result;
}

int fb_gfx3_mode_resize(FB_GFX3_MODE *mode, FB_GFX3_DRAW_STATE *state,
	uint32_t width, uint32_t height)
{
	FB_GFX3_SURFACE *new_pages = NULL;
	FB_GFX3_SURFACE *old_pages;
	FB_GFX3_RECT clip;
	FB_GFX3_RECT source_rect;
	uint32_t copy_width;
	uint32_t copy_height;
	uint32_t usage;
	uint32_t created_pages = 0u;
	uint32_t page;
	uint32_t page_count;
	uint32_t visible_page;
	int destroy_result;
	int result = FB_GFX3_OK;

	if ((mode == NULL) || (state == NULL) || (mode->mutex == NULL) ||
	    (width == 0u) || (height == 0u) || (width > INT_MAX) ||
	    (height > INT_MAX))
		return FB_GFX3_INVALID;
	fb_MutexLock(mode->mutex);
	if (!mode->initialized || !mode->resizable ||
	    (mode->page_count == 0u) || (mode->pages == NULL) ||
	    (mode->visible_page >= mode->page_count) ||
	    (state->mode != mode) || (state->generation != mode->generation)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	/* SCREENLOCK exposes page memory whose address must remain stable. */
	if (mode->access_lock_count != 0u)
		goto done;
	if ((width < mode->console_font_width) ||
	    (height < mode->console_font_height)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	if ((width == mode->width) && (height == mode->height))
		goto complete;

	page_count = mode->page_count;
	visible_page = mode->visible_page;
	result = compat_flush_pending_points_locked(state, mode);
	if ((result != FB_GFX3_OK) || (mode->page_count != page_count) ||
	    (mode->visible_page != visible_page)) {
		if (result == FB_GFX3_OK)
			result = FB_GFX3_INVALID;
		goto done;
	}
	/* A page modified through SCREENLOCK must reach the GPU before it is copied. */
	for (page = 0u; page < page_count; ++page) {
		if ((mode->shadow_dirty == NULL) || !mode->shadow_dirty[page])
			continue;
		if ((mode->shadow_pages == NULL) ||
		    (mode->shadow_pages[page] == NULL) ||
		    (mode->shadow_pitch == 0u)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		result = fb_gfx3_surface_upload(&mode->pages[page], 0, 0,
			mode->width, mode->height, mode->shadow_pitch,
			mode->shadow_pages[page]);
		if (result != FB_GFX3_OK)
			goto done;
		compat_clear_shadow_dirty(mode, page);
	}

	new_pages = (FB_GFX3_SURFACE *)calloc(page_count,
		sizeof(new_pages[0]));
	if (new_pages == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	usage = FB_GFX3_SURFACE_RENDER_TARGET | FB_GFX3_SURFACE_SAMPLED |
		FB_GFX3_SURFACE_TRANSFER_SOURCE |
		FB_GFX3_SURFACE_TRANSFER_DESTINATION;
	for (page = 0u; page < page_count; ++page) {
		result = fb_gfx3_surface_create(&mode->context, &new_pages[page],
			width, height, mode->depth, usage,
			(mode->depth == 32u) ? 0xFF000000u : 0u);
		if (result != FB_GFX3_OK)
			goto fail_new_pages;
		created_pages++;
	}

	copy_width = MIN(width, mode->width);
	copy_height = MIN(height, mode->height);
	clip.x1 = 0;
	clip.y1 = 0;
	clip.x2 = (int32_t)width - 1;
	clip.y2 = (int32_t)height - 1;
	source_rect.x1 = 0;
	source_rect.y1 = 0;
	source_rect.x2 = (int32_t)copy_width - 1;
	source_rect.y2 = (int32_t)copy_height - 1;
	for (page = 0u; page < page_count; ++page) {
		result = fb_gfx3_surface_blit(&new_pages[page], &clip,
			&mode->pages[page], &source_rect, 0, 0,
			FB_GFX3_BLIT_PSET, 255u);
		if (result != FB_GFX3_OK)
			goto fail_new_pages;
	}
	result = fb_gfx3_surface_set_visible(&new_pages[visible_page]);
	if (result != FB_GFX3_OK)
		goto fail_new_pages;
	result = fb_gfx3_surface_present(&new_pages[visible_page], TRUE);
	if (result != FB_GFX3_OK)
		goto fail_new_pages;

	old_pages = mode->pages;
	mode->pages = new_pages;
	new_pages = NULL;
	mode->width = width;
	mode->height = height;
	mode->shadow_pitch = 0u;
	mode->resize_serial++;
	if (mode->resize_serial == 0u)
		mode->resize_serial = 1u;
	for (page = 0u; page < page_count; ++page) {
		free(mode->shadow_pages[page]);
		mode->shadow_pages[page] = NULL;
		free(mode->shadow_snapshots[page]);
		mode->shadow_snapshots[page] = NULL;
		mode->shadow_valid[page] = FALSE;
		compat_clear_shadow_dirty(mode, page);
		mode->shadow_snapshot_active[page] = FALSE;
		mode->point_cache[page].valid = FALSE;
	}
	free(mode->paint_queue);
	mode->paint_queue = NULL;
	free(mode->paint_visited);
	mode->paint_visited = NULL;
	mode->paint_scratch_capacity = 0u;
	result = fb_gfx3_console_resize_locked(mode, state->foreground_color,
		state->background_color);
	if (result != FB_GFX3_OK) {
		fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_WARNING,
			"console metadata resize failed after framebuffer resize: %d",
			result);
		/* Graphics pages remain authoritative even if console metadata is lost. */
	}
	result = compat_sync_resize_locked(state, mode);
	for (page = 0u; page < page_count; ++page) {
		destroy_result = fb_gfx3_surface_destroy(&old_pages[page]);
		if (destroy_result != FB_GFX3_OK)
			fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_WARNING,
				"old resized page %u destruction failed: %d", page,
				destroy_result);
	}
	free(old_pages);

complete:
	fb_MutexUnlock(mode->mutex);
	fb_gfx3_input_complete_resize(&mode->input, width, height);
	return result;

fail_new_pages:
	while (created_pages > 0u) {
		created_pages--;
		fb_gfx3_surface_destroy(&new_pages[created_pages]);
	}
	free(new_pages);
done:
	fb_MutexUnlock(mode->mutex);
	return result;
}

int fb_gfx3_draw_state_init(FB_GFX3_MODE *mode,
	FB_GFX3_DRAW_STATE *state)
{
	uint32_t mask;

	if ((mode == NULL) || (state == NULL) || (mode->mutex == NULL))
		return FB_GFX3_INVALID;
	fb_MutexLock(mode->mutex);
	if (!mode->initialized) {
		fb_MutexUnlock(mode->mutex);
		return FB_GFX3_INVALID;
	}
	memset(state, 0, sizeof(*state));
	state->mode = mode;
	state->generation = mode->generation;
	state->resize_serial = mode->resize_serial;
	state->view.x2 = (int32_t)mode->width - 1;
	state->view.y2 = (int32_t)mode->height - 1;
	mask = compat_color_mask(mode->depth);
	state->foreground_color = ((mode->depth > 4) && (mode->depth <= 8)) ?
		15u : mask;
	state->background_color = 0xFF000000u & mask;
	state->draw_scale = 1.0f;
	state->pending_points = (FB_GFX3_POINT *)malloc(
		FB_GFX3_PENDING_POINTS_CAPACITY * sizeof(*state->pending_points));
	state->pending_points_capacity = FB_GFX3_PENDING_POINTS_CAPACITY;
	state->pending_point_submission = (FB_GFX3_POINT *)malloc(
		FB_GFX3_PENDING_POINTS_CAPACITY *
			sizeof(*state->pending_point_submission));
	state->pending_point_layers = (uint32_t *)malloc(
		FB_GFX3_PENDING_POINTS_CAPACITY *
			sizeof(*state->pending_point_layers));
	state->pending_point_layer_offsets = (uint32_t *)malloc(
		FB_GFX3_PENDING_POINTS_CAPACITY *
			sizeof(*state->pending_point_layer_offsets));
	state->pending_point_keys = (uint64_t *)calloc(
		FB_GFX3_PENDING_POINT_KEY_CAPACITY,
		sizeof(*state->pending_point_keys));
	state->pending_point_indices = (uint32_t *)calloc(
		FB_GFX3_PENDING_POINT_KEY_CAPACITY,
		sizeof(*state->pending_point_indices));
	state->pending_point_generations = (uint32_t *)calloc(
		FB_GFX3_PENDING_POINT_KEY_CAPACITY,
		sizeof(*state->pending_point_generations));
	if ((state->pending_points == NULL) ||
	    (state->pending_point_submission == NULL) ||
	    (state->pending_point_layers == NULL) ||
	    (state->pending_point_layer_offsets == NULL) ||
	    (state->pending_point_keys == NULL) ||
	    (state->pending_point_indices == NULL) ||
	    (state->pending_point_generations == NULL)) {
		fb_gfx3_draw_state_destroy(state);
		fb_MutexUnlock(mode->mutex);
		return FB_GFX3_OUT_OF_MEMORY;
	}
	state->pending_point_key_capacity = FB_GFX3_PENDING_POINT_KEY_CAPACITY;
	state->pending_point_generation = 1u;
	fb_MutexUnlock(mode->mutex);
	return FB_GFX3_OK;
}

void fb_gfx3_draw_state_destroy(FB_GFX3_DRAW_STATE *state)
{
	if (state == NULL)
		return;
	free(state->pending_points);
	free(state->pending_point_submission);
	free(state->pending_point_keys);
	free(state->pending_point_indices);
	free(state->pending_point_generations);
	free(state->pending_point_layers);
	free(state->pending_point_layer_offsets);
	state->pending_points = NULL;
	state->pending_point_submission = NULL;
	state->pending_point_keys = NULL;
	state->pending_point_indices = NULL;
	state->pending_point_generations = NULL;
	state->pending_point_layers = NULL;
	state->pending_point_layer_offsets = NULL;
	state->pending_points_count = 0;
	state->pending_points_capacity = 0;
	state->pending_point_key_capacity = 0;
	state->pending_point_generation = 0;
	state->pending_point_max_layer = 0;
}

/* ------------------------------------------------------------------------- */
/* Logical screen pages                                                      */
/* ------------------------------------------------------------------------- */

int fb_gfx3_page_set(FB_GFX3_DRAW_STATE *state, int work_page,
	int visible_page, int *previous_pages)
{
	FB_GFX3_MODE *mode = compat_state_lock(state);
	int previous;
	int result;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	result = compat_flush_pending_points_locked(state, mode);
	if (result != FB_GFX3_OK) {
		fb_MutexUnlock(mode->mutex);
		return result;
	}
	result = compat_commit_locked_shadow(state, mode);
	if (result != FB_GFX3_OK) {
		fb_MutexUnlock(mode->mutex);
		return result;
	}
	previous = (int)state->work_page | ((int)mode->visible_page << 8);
	if ((work_page < 0) && (visible_page < 0)) {
		work_page = 0;
		visible_page = 0;
	}
	if ((work_page >= 0) && ((uint32_t)work_page < mode->page_count))
		state->work_page = (uint32_t)work_page;
	if ((visible_page >= 0) && ((uint32_t)visible_page < mode->page_count) &&
	    ((uint32_t)visible_page != mode->visible_page)) {
		result = fb_gfx3_surface_set_visible(&mode->pages[visible_page]);
		if (result == FB_GFX3_OK) {
			mode->visible_page = (uint32_t)visible_page;
			/*
				SCREENSET is a visible frame boundary.  Keep it asynchronous for the
				BASIC thread, but retain an explicit PRESENT packet so a busy producer
				cannot merge several complete frames into one long backend drain before
				the first swap.  SCREENSYNC remains the explicit GPU wait.
			*/
			result = fb_gfx3_surface_present(&mode->pages[visible_page], FALSE);
			if (result == FB_GFX3_OK)
				result = fb_gfx3_context_submit_pending(&mode->context);
		}
	} else {
		result = FB_GFX3_OK;
	}
	if (previous_pages != NULL)
		*previous_pages = previous;
	fb_MutexUnlock(mode->mutex);
	return result;
}

int fb_gfx3_page_copy(FB_GFX3_DRAW_STATE *state, int from_page,
	int to_page)
{
	FB_GFX3_MODE *mode = compat_state_lock(state);
	uint32_t source_page;
	uint32_t destination_page;
	int result;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	result = compat_flush_pending_points_locked(state, mode);
	if (result != FB_GFX3_OK) {
		fb_MutexUnlock(mode->mutex);
		return result;
	}
	result = compat_commit_locked_shadow(state, mode);
	if (result != FB_GFX3_OK) {
		fb_MutexUnlock(mode->mutex);
		return result;
	}
	if (from_page < 0)
		source_page = state->work_page;
	else if ((uint32_t)from_page < mode->page_count)
		source_page = (uint32_t)from_page;
	else {
		fb_MutexUnlock(mode->mutex);
		return FB_GFX3_INVALID;
	}
	if (to_page < 0)
		destination_page = mode->visible_page;
	else if ((uint32_t)to_page < mode->page_count)
		destination_page = (uint32_t)to_page;
	else {
		fb_MutexUnlock(mode->mutex);
		return FB_GFX3_INVALID;
	}
	if (source_page == destination_page) {
		fb_MutexUnlock(mode->mutex);
		return FB_GFX3_OK;
	}
	fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_TRACE,
		"SCREENCOPY begin: page %u to page %u", source_page,
		destination_page);
	result = fb_gfx3_surface_blit(&mode->pages[destination_page],
		&state->view, &mode->pages[source_page], &state->view,
		state->view.x1, state->view.y1, FB_GFX3_BLIT_PSET, 255);
	if (result == FB_GFX3_OK) {
		result = fb_gfx3_console_page_copy_locked(mode, source_page,
			destination_page);
		if (mode->shadow_valid != NULL)
			mode->shadow_valid[destination_page] = FALSE;
		compat_invalidate_point_cache_rect_locked(mode, destination_page,
			state->view.x1, state->view.y1, state->view.x2,
			state->view.y2);
		/*
			A page copy is a logical frame-production boundary even when its target
			is currently offscreen. Send the pending batch to the renderer now so
			GPU work overlaps the next BASIC frame. If the destination is visible,
			append an asynchronous PRESENT packet. The renderer uses that packet as
			a drain boundary, which preserves within-frame batching without delaying
			the first visible frame behind later work from the same busy producer.
		*/
		if ((result == FB_GFX3_OK) &&
		    (destination_page == mode->visible_page))
			result = fb_gfx3_surface_present(
				&mode->pages[destination_page], FALSE);
		if (result == FB_GFX3_OK)
			result = fb_gfx3_context_submit_pending(&mode->context);
	}
	fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_TRACE,
		"SCREENCOPY submitted: page %u to page %u, result %d",
		source_page, destination_page, result);
	fb_MutexUnlock(mode->mutex);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Pixel-oriented compatibility operations                                  */
/* ------------------------------------------------------------------------- */

uint32_t fb_gfx3_compat_primitive_flags(const FB_GFX3_DRAW_STATE *state,
	uint32_t target_depth, uint32_t color)
{
	if ((state == NULL) || (state->mode == NULL) ||
	    !state->mode->alpha_primitives || (target_depth != 32) ||
	    ((color >> 24) == 255u))
		return 0;
	return FB_GFX3_PRIMITIVE_ALPHA_BLEND;
}

int fb_gfx3_compat_resolve_point(FB_GFX3_DRAW_STATE *state, float *x,
	float *y, uint32_t flags, int *translated_x, int *translated_y)
{
	FB_GFX3_MODE *mode = compat_state_lock(state);
	int result;

	if ((mode == NULL) || (x == NULL) || (y == NULL) ||
	    (translated_x == NULL) || (translated_y == NULL)) {
		if (mode != NULL)
			fb_MutexUnlock(mode->mutex);
		return FB_GFX3_INVALID;
	}
	compat_fix_relative(state, flags, x, y, NULL, NULL);
	result = compat_translate(state, *x, *y, translated_x, translated_y);
	fb_MutexUnlock(mode->mutex);
	return result;
}

/*
	A generated mask should cross the FreeBASIC-to-runtime boundary once, while
	retaining the same VIEW and WINDOW mapping as ordinary PSET statements.
	The pending-point layer queue keeps repeated mapped coordinates ordered and
	leaves clipping to the renderer.
*/
int fb_gfx3_compat_points_logical(FB_GFX3_DRAW_STATE *state,
	const FB_GFX3_POINT *points, uint32_t count)
{
	FB_GFX3_MODE *mode = compat_state_lock(state);
	FB_GFX3_POINT translated;
	uint32_t point_index;
	int result;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	if ((count != 0u) && (points == NULL)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	result = compat_commit_locked_shadow(state, mode);
	if (result != FB_GFX3_OK)
		goto done;
	compat_invalidate_work_point_cache_locked(mode, state, state->view.x1,
		state->view.y1, state->view.x2, state->view.y2);
	for (point_index = 0u; point_index < count; point_index++) {
		translated = points[point_index];
		result = compat_translate(state, (float)points[point_index].x,
			(float)points[point_index].y, &translated.x, &translated.y);
		if (result != FB_GFX3_OK)
			goto done;
		result = compat_queue_pending_point_locked(state, mode, &translated);
		if (result != FB_GFX3_OK)
			goto done;
	}
	/*
		A point-array call is not an observable completion boundary. Retain
		adjacent masks in the same ordered point stream so a user interface can
		draw many strings without allocating and dispatching one renderer packet
		per string. Capacity, a different primitive, page presentation, and
		every CPU readback still flush through the normal ordering paths.
	*/
	result = FB_GFX3_OK;

done:
	fb_MutexUnlock(mode->mutex);
	return result;
}

int fb_gfx3_compat_points_absolute(FB_GFX3_DRAW_STATE *state,
	const FB_GFX3_POINT *points, uint32_t count)
{
	FB_GFX3_MODE *mode = compat_state_lock(state);
	uint32_t point_index;
	int mirror_shadow;
	int result;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	result = compat_flush_pending_points_locked(state, mode);
	if (result != FB_GFX3_OK) {
		fb_MutexUnlock(mode->mutex);
		return result;
	}
	if (count == 0) {
		fb_MutexUnlock(mode->mutex);
		return FB_GFX3_OK;
	}
	if (points == NULL) {
		fb_MutexUnlock(mode->mutex);
		return FB_GFX3_INVALID;
	}
	result = compat_commit_locked_shadow(state, mode);
	if (result != FB_GFX3_OK) {
		fb_MutexUnlock(mode->mutex);
		return result;
	}
	mirror_shadow = ((mode->depth == 16u) || (mode->depth == 32u)) &&
		(mode->shadow_pages != NULL) &&
		(mode->shadow_valid != NULL) &&
		(mode->shadow_pages[state->work_page] != NULL) &&
		mode->shadow_valid[state->work_page] &&
		(mode->shadow_pitch != 0u);
	result = fb_gfx3_surface_points(compat_work_surface(mode, state),
		&state->view, points, count);
	if (result == FB_GFX3_OK) {
		/*
			GLES renders the built-in font as an absolute GPU point packet.
			Keep an existing shadow coherent with that packet so a following
			legacy POINT/PSET alpha loop does not download the entire page.
			No shadow is allocated for applications which remain GPU-only.
		*/
		if (mirror_shadow) {
			for (point_index = 0u; point_index < count; point_index++) {
				const FB_GFX3_POINT *point = &points[point_index];

				if ((point->x < state->view.x1) ||
				    (point->x > state->view.x2) ||
				    (point->y < state->view.y1) ||
				    (point->y > state->view.y2) ||
				    (point->x < 0) || (point->y < 0) ||
				    ((uint32_t)point->x >= mode->width) ||
				    ((uint32_t)point->y >= mode->height))
					continue;
				compat_write_shadow_primitive_pixel(mode,
					state->work_page, point->x, point->y,
					point->color, point->flags);
			}
		} else {
			compat_invalidate_work_shadow(mode, state, "absolute points");
		}
		/* Text and arc callers can supply sparse point lists; invalidate the
		   one cached coordinate conservatively instead of scanning that list. */
		compat_invalidate_work_point_cache_locked(mode, state, state->view.x1,
			state->view.y1, state->view.x2, state->view.y2);
	}
	fb_MutexUnlock(mode->mutex);
	return result;
}

int fb_gfx3_compat_rectangles_absolute(FB_GFX3_DRAW_STATE *state,
	const FB_GFX3_RECTANGLE_COMMAND *rectangles, uint32_t count)
{
	FB_GFX3_MODE *mode = compat_state_lock(state);
	int result;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	result = compat_flush_pending_points_locked(state, mode);
	if (result != FB_GFX3_OK)
		goto done;
	if ((count != 0u) && (rectangles == NULL)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	if (count == 0u) {
		result = FB_GFX3_OK;
		goto done;
	}
	result = compat_commit_locked_shadow(state, mode);
	if (result != FB_GFX3_OK)
		goto done;
	result = fb_gfx3_surface_rectangles(compat_work_surface(mode, state),
		rectangles, count);
	if (result == FB_GFX3_OK) {
		compat_invalidate_work_shadow(mode, state, "absolute rectangles");
		compat_invalidate_work_point_cache_locked(mode, state, state->view.x1,
			state->view.y1, state->view.x2, state->view.y2);
	}

done:
	fb_MutexUnlock(mode->mutex);
	return result;
}

int fb_gfx3_compat_glyphs_absolute(FB_GFX3_DRAW_STATE *state,
	const FB_GFX3_GLYPH *glyphs, uint32_t count)
{
	FB_GFX3_MODE *mode = compat_state_lock(state);
	int result;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	result = compat_flush_pending_points_locked(state, mode);
	if (result != FB_GFX3_OK)
		goto done;
	if ((count != 0u) && (glyphs == NULL)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	if (count == 0u) {
		result = FB_GFX3_OK;
		goto done;
	}
	result = compat_commit_locked_shadow(state, mode);
	if (result != FB_GFX3_OK)
		goto done;
	result = fb_gfx3_surface_glyphs(compat_work_surface(mode, state),
		&state->view, glyphs, count);
	if (result == FB_GFX3_OK) {
		compat_invalidate_work_shadow(mode, state, "absolute glyphs");
		compat_invalidate_work_point_cache_locked(mode, state, state->view.x1,
			state->view.y1, state->view.x2, state->view.y2);
	}

done:
	fb_MutexUnlock(mode->mutex);
	return result;
}

int fb_gfx3_compat_pset(FB_GFX3_DRAW_STATE *state, float x, float y,
	uint32_t color, uint32_t flags, int preset)
{
	FB_GFX3_MODE *mode = compat_state_graphics_locked(state);
	FB_GFX3_POINT_CACHE *cache;
	FB_GFX3_POINT point;
	uint32_t pair_count = 0;
	int paired_point = FALSE;
	int result;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	if (flags & FB_GFX3_DEFAULT_COLOR_1)
		color = preset ? state->background_color : state->foreground_color;
	else
		color = compat_fix_color(mode->depth, color);

	if (((flags & FB_GFX3_COORDINATE_MASK) == FB_GFX3_COORDINATE_AA) &&
	    ((state->flags & FB_GFX3_WINDOW_ACTIVE) == 0)) {
		if (!isfinite(x) || !isfinite(y) ||
		    ((double)x < (double)INT_MIN + 1.0) ||
		    ((double)x > (double)INT_MAX - 1.0) ||
		    ((double)y < (double)INT_MIN + 1.0) ||
		    ((double)y > (double)INT_MAX - 1.0))
			return FB_GFX3_INVALID;
		state->last_x = x;
		state->last_y = y;
		point.x = CINT(x);
		point.y = CINT(y);
		if ((state->flags & FB_GFX3_VIEW_SCREEN) == 0) {
			point.x += state->view.x1;
			point.y += state->view.y1;
		}
	} else {
		compat_fix_relative(state, flags, &x, &y, NULL, NULL);
		result = compat_translate(state, x, y, &point.x, &point.y);
		if (result != FB_GFX3_OK)
			return result;
	}
	if (!compat_inside_view(state, point.x, point.y))
		return FB_GFX3_OK;
	point.color = color;
	point.flags = fb_gfx3_compat_primitive_flags(state, mode->depth, color);
	cache = (mode->point_cache != NULL) ?
		&mode->point_cache[state->work_page] : NULL;
	if ((cache != NULL) && cache->valid &&
	    (cache->x == point.x) && (cache->y == point.y)) {
		paired_point = TRUE;
		pair_count = cache->read_modify_pairs;
	}
	compat_invalidate_work_point_cache_locked(mode, state, point.x, point.y,
		point.x, point.y);
	/*
		SCREENLOCK is also widely used as a command-grouping hint. It must not
		turn the first ordinary PSET into a full-page download. CPU rasterization
		is required only after SCREENPTR made the shadow writable, or when this
		PSET immediately follows POINT at the same coordinate and can cheaply
		complete that legacy read-modify-write pair in the valid shadow.
	*/
	if (compat_shadow_has_pending_writes(state, mode)) {
		return compat_write_truecolor_shadow_point(state, mode, &point);
	}
	if (paired_point && ((mode->depth == 16) || (mode->depth == 32))) {
		if (pair_count < UINT32_MAX)
			pair_count++;
		if ((mode->shadow_valid != NULL) &&
		    mode->shadow_valid[state->work_page]) {
			return compat_write_truecolor_shadow_point(state, mode, &point);
		}
		if (pair_count >= FB_GFX3_POINT_SHADOW_PAIR_THRESHOLD) {
			result = compat_flush_pending_points_locked(state, mode);
			if (result != FB_GFX3_OK)
				return result;
			result = compat_ensure_truecolor_shadow(state, mode, FALSE);
			if (result != FB_GFX3_OK)
				return result;
			return compat_write_truecolor_shadow_point(state, mode, &point);
		}
		cache->read_modify_pairs = pair_count;
	}
	result = compat_commit_locked_shadow(state, mode);
	if (result != FB_GFX3_OK)
		return result;
	if ((state->pending_points == NULL) ||
	    (state->pending_points_capacity == 0) ||
	    (state->pending_point_keys == NULL) ||
	    (state->pending_point_indices == NULL) ||
	    (state->pending_point_generations == NULL) ||
	    (state->pending_point_generation == 0u) ||
	    (state->pending_point_key_capacity == 0)) {
		result = fb_gfx3_surface_points(compat_work_surface(mode, state),
			&state->view, &point, 1);
		if (result == FB_GFX3_OK)
			compat_invalidate_work_shadow(mode, state, "single point");
		return result;
	}
	return compat_queue_pending_point_locked(state, mode, &point);
}

int fb_gfx3_compat_point(FB_GFX3_DRAW_STATE *state, float x, float y,
	uint32_t *color)
{
	FB_GFX3_MODE *mode = compat_state_graphics_locked(state);
	int translated_x;
	int translated_y;
	int result;

	if ((mode == NULL) || (color == NULL))
		return FB_GFX3_INVALID;
	result = compat_flush_pending_points_locked(state, mode);
	if (result != FB_GFX3_OK)
		return result;
	result = compat_translate(state, x, y, &translated_x, &translated_y);
	if (result != FB_GFX3_OK)
		return result;
	if (!compat_inside_view(state, translated_x, translated_y)) {
		*color = UINT32_MAX;
		return FB_GFX3_OK;
	}
	if ((mode->access_lock_count == 0u) && (mode->point_cache != NULL) &&
	    mode->point_cache[state->work_page].valid &&
	    (mode->point_cache[state->work_page].x == translated_x) &&
	    (mode->point_cache[state->work_page].y == translated_y)) {
		*color = mode->point_cache[state->work_page].color;
		return FB_GFX3_OK;
	}
	if (((mode->depth == 16) || (mode->depth == 32)) &&
	    (mode->shadow_pages != NULL) &&
	    (mode->shadow_valid != NULL) &&
	    (mode->shadow_pages[state->work_page] != NULL) &&
	    mode->shadow_valid[state->work_page]) {
		uint32_t bytes_per_pixel = (mode->depth == 16) ?
			sizeof(uint16_t) : sizeof(uint32_t);
		size_t offset = (size_t)translated_y * mode->shadow_pitch +
			(size_t)translated_x * bytes_per_pixel;

		if (mode->depth == 16) {
			uint16_t packed_color;

			memcpy(&packed_color,
				mode->shadow_pages[state->work_page] + offset,
				sizeof(packed_color));
			*color = compat_expand_16_bit_color(packed_color);
		} else {
			memcpy(color, mode->shadow_pages[state->work_page] + offset,
				sizeof(*color));
		}
		if (mode->point_cache != NULL) {
			mode->point_cache[state->work_page].x = translated_x;
			mode->point_cache[state->work_page].y = translated_y;
			mode->point_cache[state->work_page].color = *color;
			mode->point_cache[state->work_page].valid = TRUE;
		}
		return FB_GFX3_OK;
	}
	result = compat_ensure_locked_truecolor_shadow(state, mode);
	if (result == FB_GFX3_OK) {
		size_t offset = (size_t)translated_y * mode->shadow_pitch +
			(size_t)translated_x * sizeof(uint32_t);

		memcpy(color, mode->shadow_pages[state->work_page] + offset,
			sizeof(*color));
		if (mode->point_cache != NULL) {
			mode->point_cache[state->work_page].x = translated_x;
			mode->point_cache[state->work_page].y = translated_y;
			mode->point_cache[state->work_page].color = *color;
			mode->point_cache[state->work_page].valid = TRUE;
		}
		return FB_GFX3_OK;
	}
	if (result != FB_GFX3_UNSUPPORTED)
		return result;
	/* POINT is an ordered GPU readback and must observe a completed SCREENPTR edit. */
	result = compat_commit_locked_shadow(state, mode);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_surface_read_pixel(compat_work_surface(mode, state),
		translated_x, translated_y, color);
	if ((result == FB_GFX3_OK) && (mode->depth == 16))
		*color = compat_expand_16_bit_color(*color);
	if ((result == FB_GFX3_OK) && (mode->access_lock_count == 0u) &&
	    (mode->point_cache != NULL)) {
		mode->point_cache[state->work_page].x = translated_x;
		mode->point_cache[state->work_page].y = translated_y;
		mode->point_cache[state->work_page].color = *color;
		mode->point_cache[state->work_page].valid = TRUE;
	}
	return result;
}

int fb_gfx3_compat_line(FB_GFX3_DRAW_STATE *state, float x1, float y1,
	float x2, float y2, uint32_t color, int type, uint32_t style,
	uint32_t flags)
{
	FB_GFX3_MODE *mode = compat_state_graphics_locked(state);
	uint32_t primitive_flags;
	int translated_x1;
	int translated_y1;
	int translated_x2 = 0;
	int translated_y2 = 0;
	int result;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	if ((type < FB_GFX3_LINE_TYPE_LINE) ||
	    (type > FB_GFX3_LINE_TYPE_FILLED_BOX))
		return FB_GFX3_INVALID;
	if (flags & FB_GFX3_DEFAULT_COLOR_1)
		color = state->foreground_color;
	else
		color = compat_fix_color(mode->depth, color);
	compat_fix_relative(state, flags, &x1, &y1, &x2, &y2);
	result = compat_translate(state, x1, y1, &translated_x1,
		&translated_y1);
	if (result == FB_GFX3_OK)
		result = compat_translate(state, x2, y2, &translated_x2,
			&translated_y2);
	if (result != FB_GFX3_OK)
		return result;
	compat_invalidate_work_point_cache_locked(mode, state,
		(translated_x1 < translated_x2) ? translated_x1 : translated_x2,
		(translated_y1 < translated_y2) ? translated_y1 : translated_y2,
		(translated_x1 > translated_x2) ? translated_x1 : translated_x2,
		(translated_y1 > translated_y2) ? translated_y1 : translated_y2);
	primitive_flags = fb_gfx3_compat_primitive_flags(state, mode->depth,
		color);
	if (compat_shadow_is_authoritative(state, mode)) {
		if (type == FB_GFX3_LINE_TYPE_LINE) {
			result = compat_render_shadow_line(state, mode,
				translated_x1, translated_y1, translated_x2,
				translated_y2, color, style, primitive_flags, TRUE);
		} else {
			compat_order_int_coordinates(&translated_x1, &translated_y1,
				&translated_x2, &translated_y2);
			result = compat_mirror_shadow_rectangle(state, mode,
				translated_x1, translated_y1, translated_x2,
				translated_y2, color, style,
				type == FB_GFX3_LINE_TYPE_FILLED_BOX,
				primitive_flags);
			if (result == FB_GFX3_OK) {
				int first_y = MAX(translated_y1, state->view.y1);
				int last_y = MIN(translated_y2, state->view.y2);

				if (first_y <= last_y)
					compat_mark_shadow_dirty(mode,
						state->work_page,
						(uint32_t)first_y,
						(uint32_t)last_y);
			}
		}
		if (result == FB_GFX3_OK)
			return FB_GFX3_OK;
		if (result != FB_GFX3_UNSUPPORTED)
			return result;
	}
	if ((type == FB_GFX3_LINE_TYPE_FILLED_BOX) &&
	    ((style & 0xFFFFu) == 0xFFFFu)) {
		result = compat_queue_small_opaque_rectangle_locked(state, mode,
			translated_x1, translated_y1, translated_x2, translated_y2,
			color, primitive_flags);
		if (result == FB_GFX3_OK)
			return FB_GFX3_OK;
		if (result != FB_GFX3_UNSUPPORTED)
			return result;
	}
	result = compat_flush_pending_points_locked(state, mode);
	if (result != FB_GFX3_OK)
		return result;
	result = compat_commit_locked_shadow(state, mode);
	if (result != FB_GFX3_OK)
		return result;

	if (type == FB_GFX3_LINE_TYPE_LINE) {
		result = fb_gfx3_surface_line_graphics_locked(
			compat_work_surface(mode, state),
			&state->view, translated_x1, translated_y1, translated_x2,
			translated_y2, color, style & 0xFFFFu, primitive_flags);
	} else {
		compat_order_int_coordinates(&translated_x1, &translated_y1,
			&translated_x2, &translated_y2);
		result = fb_gfx3_surface_rectangle_graphics_locked(
			compat_work_surface(mode, state), &state->view,
			translated_x1, translated_y1, translated_x2,
			translated_y2, color, style & 0xFFFFu,
			type == FB_GFX3_LINE_TYPE_FILLED_BOX, primitive_flags);
	}
	if (result == FB_GFX3_OK) {
		int mirror_result;

		if (type == FB_GFX3_LINE_TYPE_LINE) {
			mirror_result = compat_render_shadow_line(state, mode,
				translated_x1, translated_y1, translated_x2,
				translated_y2, color, style, primitive_flags, FALSE);
		} else {
			mirror_result = compat_mirror_shadow_rectangle(state,
				mode, translated_x1, translated_y1, translated_x2,
				translated_y2, color, style,
				type == FB_GFX3_LINE_TYPE_FILLED_BOX,
				primitive_flags);
		}
		if (mirror_result != FB_GFX3_OK)
			compat_invalidate_work_shadow(mode, state, "line or rectangle");
	}
	return result;
}

int fb_gfx3_compat_ellipse(FB_GFX3_DRAW_STATE *state, float x, float y,
	float radius, uint32_t color, float aspect, int filled,
	uint32_t flags)
{
	FB_GFX3_MODE *mode = compat_state_graphics_locked(state);
	float radius_x;
	float radius_y;
	uint32_t primitive_flags;
	int center_x;
	int center_y;
	int view_width;
	int result;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	result = compat_flush_pending_points_locked(state, mode);
	if (result != FB_GFX3_OK)
		return result;
	if (!(radius > 0.0f))
		return FB_GFX3_OK;
	if (flags & FB_GFX3_DEFAULT_COLOR_1)
		color = state->foreground_color;
	else
		color = compat_fix_color(mode->depth, color);
	compat_fix_relative(state, flags, &x, &y, NULL, NULL);
	result = compat_translate(state, x, y, &center_x, &center_y);
	if (result != FB_GFX3_OK)
		return result;
	if (state->flags & FB_GFX3_WINDOW_ACTIVE) {
		view_width = state->view.x2 - state->view.x1 + 1;
		radius *= (float)view_width / state->window_width;
	}
	if (aspect == 0.0f)
		aspect = 1.0f;
	if (aspect > 1.0f) {
		radius_x = radius / aspect;
		radius_y = radius;
	} else {
		radius_x = radius;
		radius_y = radius * aspect;
	}
	primitive_flags = fb_gfx3_compat_primitive_flags(state, mode->depth,
		color);
	result = compat_render_authoritative_shadow_ellipse(state, mode,
		center_x, center_y, radius_x, radius_y, color, filled,
		primitive_flags);
	if (result == FB_GFX3_OK)
		return FB_GFX3_OK;
	if (result != FB_GFX3_UNSUPPORTED)
		return result;
	result = compat_commit_locked_shadow(state, mode);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_surface_ellipse_graphics_locked(
		compat_work_surface(mode, state), &state->view, center_x,
		center_y, radius_x, radius_y, color, filled, primitive_flags);
	if (result == FB_GFX3_OK) {
		compat_invalidate_work_shadow(mode, state, "ellipse");
		compat_invalidate_work_point_cache_locked(mode, state, state->view.x1,
			state->view.y1, state->view.x2, state->view.y2);
	}
	return result;
}

int fb_gfx3_compat_arc(FB_GFX3_DRAW_STATE *state, float x, float y,
	float radius, uint32_t color, float aspect, float start, float end,
	uint32_t flags)
{
	FB_GFX3_MODE *mode = compat_state_lock(state);
	FB_GFX3_POINT *points = NULL;
	FB_GFX3_SURFACE *surface;
	float radius_x;
	float radius_y;
	float increment;
	float span;
	float angle;
	float turns;
	int center_x;
	int center_y;
	int endpoint_x;
	int endpoint_y;
	int start_radial;
	int end_radial;
	int view_width;
	uint32_t count;
	uint32_t i;
	size_t allocation_size;
	int result;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	if (!(radius > 0.0f)) {
		fb_MutexUnlock(mode->mutex);
		return FB_GFX3_OK;
	}
	if (!isfinite(radius) || !isfinite(aspect) ||
	    !isfinite(start) || !isfinite(end)) {
		fb_MutexUnlock(mode->mutex);
		return FB_GFX3_INVALID;
	}
	if (flags & FB_GFX3_DEFAULT_COLOR_1)
		color = state->foreground_color;
	else
		color = compat_fix_color(mode->depth, color);
	compat_fix_relative(state, flags, &x, &y, NULL, NULL);
	result = compat_translate(state, x, y, &center_x, &center_y);
	if (result != FB_GFX3_OK)
		goto done;
	if (state->flags & FB_GFX3_WINDOW_ACTIVE) {
		view_width = state->view.x2 - state->view.x1 + 1;
		radius *= (float)view_width / state->window_width;
	}
	if (aspect == 0.0f)
		aspect = 1.0f;
	if (aspect <= 0.0f) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	if (aspect > 1.0f) {
		radius_x = radius / aspect;
		radius_y = radius;
	} else {
		radius_x = radius;
		radius_y = radius * aspect;
	}
	if (!(radius_x > 0.0f) || !(radius_y > 0.0f) ||
	    (radius_x > 32767.0f) || (radius_y > 32767.0f)) {
		result = FB_GFX3_INVALID;
		goto done;
	}

	start_radial = start < 0.0f;
	end_radial = end < 0.0f;
	result = compat_flush_pending_points_locked(state, mode);
	if (result != FB_GFX3_OK) {
		fb_MutexUnlock(mode->mutex);
		return result;
	}
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
	increment = 1.0f /
		(sqrtf(radius_x) * sqrtf(radius_y) * 1.5f);
	if (!isfinite(increment) || !(increment > 0.0f) ||
	    !isfinite(span) || (span < 0.0f) ||
	    ((double)span / increment + 1.5 > UINT32_MAX)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	count = (uint32_t)(span / increment + 0.5f) + 1;
	if ((count == 0u) ||
	    (fb_gfx3_size_multiply(count, sizeof(points[0]),
	     &allocation_size) != FB_GFX3_OK) ||
	    (allocation_size == 0u) ||
	    (allocation_size > FB_GFX3_COMMAND_MAX_SIZE /
	     2)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	result = compat_commit_locked_shadow(state, mode);
	if (result != FB_GFX3_OK)
		goto done;
	points = (FB_GFX3_POINT *)calloc(1, allocation_size);
	if (points == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	for (i = 0; i < count; i++) {
		angle = start + ((float)i * increment);
		points[i].x = compat_add_coordinate_offset(center_x,
			CINT(cosf(angle) * radius_x));
		points[i].y = compat_add_coordinate_offset(center_y,
			-CINT(sinf(angle) * radius_y));
		points[i].color = color;
		points[i].flags = fb_gfx3_compat_primitive_flags(state,
			mode->depth, color);
	}

	surface = compat_work_surface(mode, state);
	if (start_radial) {
		endpoint_x = compat_add_coordinate_offset(center_x,
			CINT(cosf(start) * radius_x));
		endpoint_y = compat_add_coordinate_offset(center_y,
			-CINT(sinf(start) * radius_y));
		result = fb_gfx3_surface_line(surface, &state->view, center_x,
			center_y, endpoint_x, endpoint_y, color, 0xFFFFu,
			fb_gfx3_compat_primitive_flags(state, mode->depth, color));
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (end_radial) {
		endpoint_x = compat_add_coordinate_offset(center_x,
			CINT(cosf(end) * radius_x));
		endpoint_y = compat_add_coordinate_offset(center_y,
			-CINT(sinf(end) * radius_y));
		result = fb_gfx3_surface_line(surface, &state->view, center_x,
			center_y, endpoint_x, endpoint_y, color, 0xFFFFu,
			fb_gfx3_compat_primitive_flags(state, mode->depth, color));
		if (result != FB_GFX3_OK)
			goto done;
	}
	result = fb_gfx3_surface_points(surface, &state->view, points, count);
	if (result == FB_GFX3_OK) {
		compat_invalidate_work_shadow(mode, state, "arc");
		compat_invalidate_work_point_cache_locked(mode, state, state->view.x1,
			state->view.y1, state->view.x2, state->view.y2);
	}

done:
	free(points);
	fb_MutexUnlock(mode->mutex);
	return result;
}

/* ------------------------------------------------------------------------- */
/* VIEW, WINDOW, PMAP, and POINTCOORD                                        */
/* ------------------------------------------------------------------------- */

int fb_gfx3_compat_view(FB_GFX3_DRAW_STATE *state, int x1, int y1,
	int x2, int y2, uint32_t fill_color, uint32_t border_color,
	uint32_t flags)
{
	FB_GFX3_MODE *mode = compat_state_lock(state);
	FB_GFX3_RECT full_view;
	FB_GFX3_RECT new_view;
	FB_GFX3_SURFACE *surface;
	int border_x1;
	int border_y1;
	int border_x2;
	int border_y2;
	int result = FB_GFX3_OK;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	result = compat_flush_pending_points_locked(state, mode);
	if (result != FB_GFX3_OK) {
		fb_MutexUnlock(mode->mutex);
		return result;
	}
	full_view.x1 = 0;
	full_view.y1 = 0;
	full_view.x2 = (int32_t)mode->width - 1;
	full_view.y2 = (int32_t)mode->height - 1;
	if ((x1 == INT16_MIN) && (y1 == INT16_MIN) &&
	    (x2 == INT16_MIN) && (y2 == INT16_MIN)) {
		state->flags &= ~(uint32_t)FB_GFX3_VIEWPORT_SET;
		state->view = full_view;
		fb_MutexUnlock(mode->mutex);
		return FB_GFX3_OK;
	}
	result = compat_commit_locked_shadow(state, mode);
	if (result != FB_GFX3_OK) {
		fb_MutexUnlock(mode->mutex);
		return result;
	}

	compat_order_int_coordinates(&x1, &y1, &x2, &y2);
	if ((x1 >= (int)mode->width) || (y1 >= (int)mode->height) ||
	    (x2 < 0) || (y2 < 0)) {
		fb_MutexUnlock(mode->mutex);
		return FB_GFX3_OK;
	}
	surface = compat_work_surface(mode, state);
	state->flags |= FB_GFX3_VIEWPORT_SET;
	if (flags & 1u)
		state->flags |= FB_GFX3_VIEW_SCREEN;
	else
		state->flags &= ~(uint32_t)FB_GFX3_VIEW_SCREEN;
	if ((flags & FB_GFX3_DEFAULT_COLOR_2) == 0) {
		border_color = compat_fix_color(mode->depth, border_color);
		border_x1 = (x1 == INT_MIN) ? INT_MIN : x1 - 1;
		border_y1 = (y1 == INT_MIN) ? INT_MIN : y1 - 1;
		border_x2 = (x2 == INT_MAX) ? INT_MAX : x2 + 1;
		border_y2 = (y2 == INT_MAX) ? INT_MAX : y2 + 1;
		result = fb_gfx3_surface_rectangle(surface, &full_view,
			border_x1, border_y1, border_x2, border_y2,
			border_color, 0xFFFFu,
			FALSE, fb_gfx3_compat_primitive_flags(state, mode->depth,
				border_color));
	}
	new_view.x1 = (x1 < 0) ? 0 : x1;
	new_view.y1 = (y1 < 0) ? 0 : y1;
	new_view.x2 = (x2 >= (int)mode->width) ?
		(int32_t)mode->width - 1 : x2;
	new_view.y2 = (y2 >= (int)mode->height) ?
		(int32_t)mode->height - 1 : y2;
	state->view = new_view;
	if ((result == FB_GFX3_OK) &&
	    ((flags & FB_GFX3_DEFAULT_COLOR_1) == 0)) {
		fill_color = compat_fix_color(mode->depth, fill_color);
		result = fb_gfx3_surface_clear(surface, &new_view, fill_color,
			fb_gfx3_compat_primitive_flags(state, mode->depth,
				fill_color));
	}
	if (result == FB_GFX3_OK)
		compat_invalidate_work_shadow(mode, state, "view clear");
	if (result == FB_GFX3_OK)
		compat_invalidate_work_point_cache_locked(mode, state, full_view.x1,
			full_view.y1, full_view.x2, full_view.y2);
	fb_MutexUnlock(mode->mutex);
	return result;
}

static int compat_window_unlocked(FB_GFX3_DRAW_STATE *state,
	FB_GFX3_MODE *mode, float x1, float y1, float x2, float y2, int screen)
{
	float temporary;
	int result;

	result = compat_flush_pending_points_locked(state, mode);
	if (result != FB_GFX3_OK)
		return result;
	if ((x1 == 0.0f) && (y1 == 0.0f) && (x2 == 0.0f) && (y2 == 0.0f)) {
		state->flags &= ~(uint32_t)(FB_GFX3_WINDOW_ACTIVE |
			FB_GFX3_WINDOW_SCREEN);
		return FB_GFX3_OK;
	}
	if (x2 < x1) {
		temporary = x1;
		x1 = x2;
		x2 = temporary;
	}
	if (y2 < y1) {
		temporary = y1;
		y1 = y2;
		y2 = temporary;
	}
	if ((x2 == x1) || (y2 == y1))
		return FB_GFX3_INVALID;
	state->window_x = x1;
	state->window_y = y1;
	state->window_width = x2 - x1;
	state->window_height = y2 - y1;
	state->flags |= FB_GFX3_WINDOW_ACTIVE;
	if (screen)
		state->flags |= FB_GFX3_WINDOW_SCREEN;
	else
		state->flags &= ~(uint32_t)FB_GFX3_WINDOW_SCREEN;
	return FB_GFX3_OK;
}

int fb_gfx3_compat_window(FB_GFX3_DRAW_STATE *state, float x1, float y1,
	float x2, float y2, int screen)
{
	FB_GFX3_MODE *mode = compat_state_lock(state);
	int result;

	if (mode == NULL)
		return FB_GFX3_INVALID;
	result = compat_window_unlocked(state, mode, x1, y1, x2, y2, screen);
	fb_MutexUnlock(mode->mutex);
	return result;
}

int fb_gfx3_compat_window_graphics_locked(FB_GFX3_DRAW_STATE *state,
	float x1, float y1, float x2, float y2, int screen)
{
	FB_GFX3_MODE *mode = compat_state_graphics_locked(state);

	if (mode == NULL)
		return FB_GFX3_INVALID;
	return compat_window_unlocked(state, mode, x1, y1, x2, y2, screen);
}

static float compat_pmap_unlocked(const FB_GFX3_DRAW_STATE *state,
	float coordinate, int function)
{
	float view_width = (float)(state->view.x2 - state->view.x1 + 1);
	float view_height = (float)(state->view.y2 - state->view.y1 + 1);

	switch (function) {
	case 0:
		if (state->flags & FB_GFX3_WINDOW_ACTIVE)
			coordinate = ((coordinate - state->window_x) * view_width) /
				state->window_width;
		break;
	case 1:
		if (state->flags & FB_GFX3_WINDOW_ACTIVE) {
			coordinate = ((coordinate - state->window_y) * view_height) /
				state->window_height;
			if ((state->flags & FB_GFX3_WINDOW_SCREEN) == 0)
				coordinate = view_height - 1.0f - coordinate;
		}
		break;
	case 2:
		if (state->flags & FB_GFX3_WINDOW_ACTIVE)
			coordinate = ((coordinate * state->window_width) /
				view_width) + state->window_x;
		break;
	case 3:
		if (state->flags & FB_GFX3_WINDOW_ACTIVE) {
			if ((state->flags & FB_GFX3_WINDOW_SCREEN) == 0)
				coordinate = view_height - 1.0f - coordinate;
			coordinate = ((coordinate * state->window_height) /
				view_height) + state->window_y;
		}
		break;
	default:
		coordinate = 0.0f;
		break;
	}
	return coordinate;
}

int fb_gfx3_compat_pmap(FB_GFX3_DRAW_STATE *state, float coordinate,
	int function, float *result)
{
	FB_GFX3_MODE *mode = compat_state_lock(state);

	if ((mode == NULL) || (result == NULL)) {
		if (mode != NULL)
			fb_MutexUnlock(mode->mutex);
		return FB_GFX3_INVALID;
	}
	*result = compat_pmap_unlocked(state, coordinate, function);
	fb_MutexUnlock(mode->mutex);
	return FB_GFX3_OK;
}

int fb_gfx3_compat_pmap_graphics_locked(FB_GFX3_DRAW_STATE *state,
	float coordinate, int function, float *result)
{
	if ((compat_state_graphics_locked(state) == NULL) || (result == NULL))
		return FB_GFX3_INVALID;
	*result = compat_pmap_unlocked(state, coordinate, function);
	return FB_GFX3_OK;
}

int fb_gfx3_compat_cursor(FB_GFX3_DRAW_STATE *state, int function,
	float *result)
{
	FB_GFX3_MODE *mode = compat_state_lock(state);

	if ((mode == NULL) || (result == NULL)) {
		if (mode != NULL)
			fb_MutexUnlock(mode->mutex);
		return FB_GFX3_INVALID;
	}
	switch (function) {
	case 0:
		*result = compat_pmap_unlocked(state, state->last_x, 0);
		break;
	case 1:
		*result = compat_pmap_unlocked(state, state->last_y, 1);
		break;
	case 2:
		*result = state->last_x;
		break;
	case 3:
		*result = state->last_y;
		break;
	default:
		*result = 0.0f;
		break;
	}
	fb_MutexUnlock(mode->mutex);
	return FB_GFX3_OK;
}

int fb_gfx3_compat_cursor_graphics_locked(FB_GFX3_DRAW_STATE *state,
	int function, float *result)
{
	if ((compat_state_graphics_locked(state) == NULL) || (result == NULL))
		return FB_GFX3_INVALID;
	switch (function) {
	case 0:
		*result = compat_pmap_unlocked(state, state->last_x, 0);
		break;
	case 1:
		*result = compat_pmap_unlocked(state, state->last_y, 1);
		break;
	case 2:
		*result = state->last_x;
		break;
	case 3:
		*result = state->last_y;
		break;
	default:
		*result = 0.0f;
		break;
	}
	return FB_GFX3_OK;
}

/* end of gfx3_compat.c */
