/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_context.c

    Purpose:

        Implement the typed internal surface API above the command queue.

    Responsibilities:

        - initialize and shut down a renderer with centralized logging
        - build checked, self-contained command payloads
        - queue write-only work without waiting
        - wait only for creation, destruction, readback, and explicit flushes

    This file intentionally does NOT contain:

        - FreeBASIC runtime entry points
        - coordinate translation or current drawing state
        - rendering algorithms or backend calls
*/

#include "gfx3_context.h"

#include <math.h>
#include <stdlib.h>

/* ------------------------------------------------------------------------- */
/* Command submission                                                        */
/* ------------------------------------------------------------------------- */

static int surface_is_valid(const FB_GFX3_SURFACE *surface);

static void context_mark_visible_modified(FB_GFX3_CONTEXT *context,
	FB_GFX3_HANDLE target)
{
	if ((context == NULL) || (target == 0))
		return;
	if (atomic_load(&context->visible_surface_handle) != target)
		return;
	atomic_fetch_add(&context->visible_content_revision, 1u);
}

/*
	These command types change pixels which can contribute to the next
	presentation. Recording the mutation while submission_mutex is still held
	keeps the revision in the same producer order as the command itself.
*/
static void context_mark_command_visible_modified(FB_GFX3_CONTEXT *context,
	const FB_GFX3_COMMAND *command)
{
	FB_GFX3_HANDLE target;

	if ((context == NULL) || (command == NULL))
		return;
	target = command->target;
	switch (command->type) {
	case FB_GFX3_COMMAND_SURFACE_UPLOAD:
	case FB_GFX3_COMMAND_CLEAR:
	case FB_GFX3_COMMAND_POINTS:
	case FB_GFX3_COMMAND_LINE:
	case FB_GFX3_COMMAND_LINES:
	case FB_GFX3_COMMAND_RECTANGLE:
	case FB_GFX3_COMMAND_RECTANGLES:
	case FB_GFX3_COMMAND_ELLIPSE:
	case FB_GFX3_COMMAND_PAINT:
	case FB_GFX3_COMMAND_BLIT:
	case FB_GFX3_COMMAND_BLITS:
	case FB_GFX3_COMMAND_TRANSFORM_BLIT:
	case FB_GFX3_COMMAND_GLYPHS:
		break;
	case FB_GFX3_COMMAND_PALETTE:
	case FB_GFX3_COMMAND_INTEROP_CALLBACK:
		target = atomic_load(&context->visible_surface_handle);
		break;
	default:
		return;
	}
	context_mark_visible_modified(context, target);
}

static int context_submit_commands_locked(FB_GFX3_CONTEXT *context)
{
	int result;
	size_t index;

	if ((context == NULL) || (context->submission_mutex == NULL))
		return FB_GFX3_INVALID;
	if (context->pending_command_count == 0)
		return FB_GFX3_OK;
	result = fb_gfx3_renderer_submit_many(&context->renderer,
		context->pending_commands, context->pending_command_count, NULL);
	if (result != FB_GFX3_OK) {
		for (index = 0; index < context->pending_command_count; index++)
			fb_gfx3_command_destroy(context->pending_commands[index]);
	}
	context->pending_command_count = 0;
	return result;
}

/*
	Polygon fillers often issue one horizontal LINE per span. Preserve those
	operations as one ordered payload so the BASIC thread performs no per-line
	allocation and the renderer can upload the complete shader batch directly.
	The caller owns submission_mutex or the runtime-wide graphics lock.
*/
static int context_flush_lines_locked(FB_GFX3_CONTEXT *context)
{
	FB_GFX3_LINES_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	size_t line_bytes;
	size_t payload_size;
	int result;

	if ((context == NULL) || (context->submission_mutex == NULL))
		return FB_GFX3_INVALID;
	if (context->pending_line_count == 0u)
		return FB_GFX3_OK;
	if ((context->pending_line_target == 0) ||
	    (context->pending_line_count > FB_GFX3_CONTEXT_PENDING_LINE_LIMIT) ||
	    (fb_gfx3_size_multiply(context->pending_line_count,
	     sizeof(context->pending_lines[0]), &line_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_LINES_COMMAND, line), line_bytes,
	     &payload_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	if (context->pending_command_count == FB_GFX3_CONTEXT_PENDING_COMMAND_LIMIT) {
		result = context_submit_commands_locked(context);
		if (result != FB_GFX3_OK)
			return result;
	}
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_LINES, payload_size);
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = context->pending_line_target;
	payload = (FB_GFX3_LINES_COMMAND *)command->payload;
	payload->count = (uint32_t)context->pending_line_count;
	memset(payload->reserved, 0, sizeof(payload->reserved));
	memcpy(payload->line, context->pending_lines, line_bytes);
	context->pending_commands[context->pending_command_count++] = command;
	context->pending_line_count = 0u;
	context->pending_line_target = 0;
	return FB_GFX3_OK;
}

/*
	A uniform-image PUT can become an opaque rectangle. Vulkan also accepts
	outline boxes in the same ordered packet. Holding adjacent rectangles here
	avoids allocator and queue traffic for every public LINE ... B/BF operation,
	while a subsequent non-rectangle command explicitly materializes the packet
	before its own submission. The caller owns submission_mutex.
*/
static int context_flush_rectangles_locked(FB_GFX3_CONTEXT *context)
{
	FB_GFX3_RECTANGLES_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	size_t rectangle_bytes;
	size_t payload_size;
	int result;

	if ((context == NULL) || (context->submission_mutex == NULL))
		return FB_GFX3_INVALID;
	if (context->pending_rectangle_count == 0u)
		return FB_GFX3_OK;
	if ((context->pending_rectangle_target == 0) ||
	    (context->pending_rectangle_count >
	     FB_GFX3_CONTEXT_PENDING_RECTANGLE_LIMIT) ||
	    (fb_gfx3_size_multiply(context->pending_rectangle_count,
	     sizeof(context->pending_rectangles[0]), &rectangle_bytes) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_RECTANGLES_COMMAND, rectangle),
	     rectangle_bytes, &payload_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	if (context->pending_command_count == FB_GFX3_CONTEXT_PENDING_COMMAND_LIMIT) {
		result = context_submit_commands_locked(context);
		if (result != FB_GFX3_OK)
			return result;
	}
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_RECTANGLES, payload_size);
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = context->pending_rectangle_target;
	payload = (FB_GFX3_RECTANGLES_COMMAND *)command->payload;
	payload->count = (uint32_t)context->pending_rectangle_count;
	memset(payload->reserved, 0, sizeof(payload->reserved));
	memcpy(payload->rectangle, context->pending_rectangles, rectangle_bytes);
	context->pending_commands[context->pending_command_count++] = command;
	context->pending_rectangle_count = 0u;
	context->pending_rectangle_target = 0;
	return FB_GFX3_OK;
}

/*
	The desktop renderer's raster BLIT packet accepts the ordinary opaque and
	logic PUT modes. Blend-family modes retain their existing tile path, which
	already combines commands after they have entered the renderer queue.
*/
static int context_blit_is_packet_compatible(uint32_t mode)
{
	switch (mode) {
	case FB_GFX3_BLIT_TRANS:
	case FB_GFX3_BLIT_PSET:
	case FB_GFX3_BLIT_PRESET:
	case FB_GFX3_BLIT_AND:
	case FB_GFX3_BLIT_OR:
	case FB_GFX3_BLIT_XOR:
		return TRUE;
	default:
		return FALSE;
	}
}

static int context_blit_packet_supported(const FB_GFX3_CONTEXT *context,
	uint32_t mode)
{
	uint32_t features;

	if (context == NULL)
		return FALSE;
	features = context->renderer.backend.caps.features;
	if ((features & FB_GFX3_FEATURE_PACKED_BLITS) == 0u)
		return FALSE;
	if ((features & FB_GFX3_FEATURE_COMPUTE) != 0u)
		return context_blit_is_packet_compatible(mode);
	/* ES 3.0 can batch the destination-independent raster operations. */
	return mode <= FB_GFX3_BLIT_PRESET;
}

static int context_heterogeneous_blit_packet_supported(
	const FB_GFX3_CONTEXT *context)
{
	if (context == NULL)
		return FALSE;
	return (context->renderer.backend.caps.features &
		FB_GFX3_FEATURE_HETEROGENEOUS_BLITS) != 0u;
}

static int context_flush_blits_locked(FB_GFX3_CONTEXT *context)
{
	FB_GFX3_BLITS_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	size_t blit_bytes;
	size_t payload_size;
	int result;

	if ((context == NULL) || (context->submission_mutex == NULL))
		return FB_GFX3_INVALID;
	if (context->pending_blit_count == 0u)
		return FB_GFX3_OK;
	if ((context->pending_blit_target == 0) ||
	    (context->pending_blits[0].source == 0) ||
	    (context->pending_blit_count > FB_GFX3_CONTEXT_PENDING_BLIT_LIMIT) ||
	    (fb_gfx3_size_multiply(context->pending_blit_count,
	     sizeof(context->pending_blits[0]), &blit_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_BLITS_COMMAND, blit), blit_bytes,
	     &payload_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	if (context->pending_command_count == FB_GFX3_CONTEXT_PENDING_COMMAND_LIMIT) {
		result = context_submit_commands_locked(context);
		if (result != FB_GFX3_OK)
			return result;
	}
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_BLITS, payload_size);
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = context->pending_blit_target;
	payload = (FB_GFX3_BLITS_COMMAND *)command->payload;
	payload->source = context->pending_blits[0].source;
	payload->mode = context->pending_blits[0].mode;
	payload->alpha = context->pending_blits[0].alpha;
	payload->count = (uint32_t)context->pending_blit_count;
	memset(payload->reserved, 0, sizeof(payload->reserved));
	memcpy(payload->blit, context->pending_blits, blit_bytes);
	context->pending_commands[context->pending_command_count++] = command;
	context->pending_blit_count = 0u;
	context->pending_blit_target = 0;
	context->pending_blit_source = 0;
	context->pending_blit_mode = 0u;
	context->pending_blit_alpha = 0u;
	return FB_GFX3_OK;
}

static int context_flush_glyphs_locked(FB_GFX3_CONTEXT *context)
{
	FB_GFX3_GLYPHS_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	size_t glyph_bytes;
	size_t payload_size;
	int result;

	if ((context == NULL) || (context->submission_mutex == NULL))
		return FB_GFX3_INVALID;
	if (context->pending_glyph_count == 0u)
		return FB_GFX3_OK;
	if ((context->pending_glyph_target == 0) ||
	    (context->pending_glyph_count > FB_GFX3_CONTEXT_PENDING_GLYPH_LIMIT) ||
	    (fb_gfx3_size_multiply(context->pending_glyph_count,
	     sizeof(context->pending_glyphs[0]), &glyph_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_GLYPHS_COMMAND, glyph), glyph_bytes,
	     &payload_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	if (context->pending_command_count == FB_GFX3_CONTEXT_PENDING_COMMAND_LIMIT) {
		result = context_submit_commands_locked(context);
		if (result != FB_GFX3_OK)
			return result;
	}
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_GLYPHS, payload_size);
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = context->pending_glyph_target;
	payload = (FB_GFX3_GLYPHS_COMMAND *)command->payload;
	payload->clip = context->pending_glyph_clip;
	payload->count = (uint32_t)context->pending_glyph_count;
	memset(payload->reserved, 0, sizeof(payload->reserved));
	memcpy(payload->glyph, context->pending_glyphs, glyph_bytes);
	context->pending_commands[context->pending_command_count++] = command;
	context->pending_glyph_count = 0u;
	context->pending_glyph_target = 0;
	memset(&context->pending_glyph_clip, 0,
		sizeof(context->pending_glyph_clip));
	return FB_GFX3_OK;
}

static int context_flush_staged_locked(FB_GFX3_CONTEXT *context)
{
	int result;

	result = context_flush_lines_locked(context);
	if (result != FB_GFX3_OK)
		return result;
	result = context_flush_glyphs_locked(context);
	if (result != FB_GFX3_OK)
		return result;
	result = context_flush_rectangles_locked(context);
	if (result != FB_GFX3_OK)
		return result;
	return context_flush_blits_locked(context);
}

static int context_submit_pending_locked(FB_GFX3_CONTEXT *context)
{
	int result;

	if ((context == NULL) || (context->submission_mutex == NULL))
		return FB_GFX3_INVALID;
	result = context_flush_staged_locked(context);
	if (result != FB_GFX3_OK)
		return result;
	result = context_submit_commands_locked(context);
	if (result == FB_GFX3_OK)
		atomic_store(&context->pending_submission_work, FALSE);
	return result;
}

static int context_submit_async_locked(FB_GFX3_CONTEXT *context,
	FB_GFX3_COMMAND *command)
{
	int result;

	if ((context == NULL) || (command == NULL))
		return FB_GFX3_INVALID;
	result = context_flush_staged_locked(context);
	if ((result == FB_GFX3_OK) && (context->pending_command_count ==
	    FB_GFX3_CONTEXT_PENDING_COMMAND_LIMIT))
		result = context_submit_pending_locked(context);
	if (result != FB_GFX3_OK)
		return result;
	context->pending_commands[context->pending_command_count++] = command;
	context_mark_command_visible_modified(context, command);
	atomic_store(&context->pending_submission_work, TRUE);
	return FB_GFX3_OK;
}

/*
	The caller provides producer serialization. Public compatibility entry points
	may use FB_GRAPHICS_LOCK instead of submission_mutex, because that outer
	runtime lock serializes every mode and page mutation reaching this context.
*/
static int context_stage_rectangle_locked(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style, int filled, uint32_t flags)
{
	FB_GFX3_CONTEXT *context;
	FB_GFX3_RECTANGLE_COMMAND *rectangle;
	int result;

	if (!surface_is_valid(surface) || (clip == NULL))
		return FB_GFX3_INVALID;
	context = surface->context;
	if ((context == NULL) || (context->submission_mutex == NULL))
		return FB_GFX3_INVALID;
	if (context->pending_line_count != 0u) {
		result = context_flush_lines_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (context->pending_glyph_count != 0u) {
		result = context_flush_glyphs_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (context->pending_blit_count != 0u) {
		result = context_flush_blits_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	if ((context->pending_rectangle_count != 0u) &&
	    (context->pending_rectangle_target != surface->handle)) {
		result = context_flush_rectangles_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (context->pending_rectangle_count ==
	    FB_GFX3_CONTEXT_PENDING_RECTANGLE_LIMIT) {
		result = context_flush_rectangles_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	rectangle = &context->pending_rectangles[context->pending_rectangle_count];
	rectangle->clip = *clip;
	rectangle->x1 = x1;
	rectangle->y1 = y1;
	rectangle->x2 = x2;
	rectangle->y2 = y2;
	rectangle->color = color;
	rectangle->style = style & 0xFFFFu;
	rectangle->filled = (filled != 0);
	rectangle->flags = flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	context->pending_rectangle_target = surface->handle;
	context->pending_rectangle_count++;
	context_mark_visible_modified(context, surface->handle);
	atomic_store(&context->pending_submission_work, TRUE);
	result = FB_GFX3_OK;

done:
	return result;
}

static int context_stage_rectangle(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style, int filled, uint32_t flags)
{
	FB_GFX3_CONTEXT *context;
	int result;

	if ((surface == NULL) || (surface->context == NULL) ||
	    (surface->context->submission_mutex == NULL))
		return FB_GFX3_INVALID;
	context = surface->context;
	fb_MutexLock(context->submission_mutex);
	result = context_stage_rectangle_locked(surface, clip, x1, y1, x2, y2,
		color, style, filled, flags);
	fb_MutexUnlock(context->submission_mutex);
	return result;
}

/*
	Built-in bitmap text naturally decomposes into short horizontal spans.  Copy
	a whole span list while holding submission_mutex once, then let the existing
	RECTANGLES packet move the coverage work to the GPU.  All items are checked
	before any are staged so invalid input cannot leave a partial draw behind.
*/
static int context_stage_rectangles(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECTANGLE_COMMAND *rectangles, uint32_t count)
{
	FB_GFX3_CONTEXT *context;
	size_t source_index = 0u;
	uint32_t index;
	int result = FB_GFX3_OK;

	if (!surface_is_valid(surface) ||
	    ((count != 0u) && (rectangles == NULL)))
		return FB_GFX3_INVALID;
	if ((surface->usage & FB_GFX3_SURFACE_RENDER_TARGET) == 0u)
		return FB_GFX3_UNSUPPORTED;
	if (count == 0u)
		return FB_GFX3_OK;
	context = surface->context;
	if ((context == NULL) || (context->submission_mutex == NULL) ||
	    ((context->renderer.backend.caps.features &
	      FB_GFX3_FEATURE_COMPUTE) == 0u))
		return FB_GFX3_UNSUPPORTED;
	for (index = 0u; index < count; ++index) {
		const FB_GFX3_RECTANGLE_COMMAND *rectangle = &rectangles[index];

		if ((rectangle->clip.x1 > rectangle->clip.x2) ||
		    (rectangle->clip.y1 > rectangle->clip.y2) ||
		    (rectangle->x1 > rectangle->x2) ||
		    (rectangle->y1 > rectangle->y2) ||
		    ((rectangle->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u))
			return FB_GFX3_INVALID;
	}

	fb_MutexLock(context->submission_mutex);
	if (context->pending_line_count != 0u) {
		result = context_flush_lines_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (context->pending_glyph_count != 0u) {
		result = context_flush_glyphs_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (context->pending_blit_count != 0u) {
		result = context_flush_blits_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	if ((context->pending_rectangle_count != 0u) &&
	    (context->pending_rectangle_target != surface->handle)) {
		result = context_flush_rectangles_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	while (source_index < count) {
		size_t available;
		size_t copy_count;

		if (context->pending_rectangle_count ==
		    FB_GFX3_CONTEXT_PENDING_RECTANGLE_LIMIT) {
			result = context_flush_rectangles_locked(context);
			if (result != FB_GFX3_OK)
				goto done;
		}
		available = FB_GFX3_CONTEXT_PENDING_RECTANGLE_LIMIT -
			context->pending_rectangle_count;
		copy_count = (size_t)count - source_index;
		if (copy_count > available)
			copy_count = available;
		memcpy(context->pending_rectangles + context->pending_rectangle_count,
			rectangles + source_index,
			copy_count * sizeof(rectangles[0]));
		context->pending_rectangle_target = surface->handle;
		context->pending_rectangle_count += copy_count;
		source_index += copy_count;
	}
	context_mark_visible_modified(context, surface->handle);
	atomic_store(&context->pending_submission_work, TRUE);

done:
	fb_MutexUnlock(context->submission_mutex);
	return result;
}

static int context_stage_glyphs(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, const FB_GFX3_GLYPH *glyphs, uint32_t count)
{
	FB_GFX3_CONTEXT *context;
	size_t source_index = 0u;
	uint32_t index;
	int result = FB_GFX3_OK;

	if (!surface_is_valid(surface) || (clip == NULL) ||
	    ((count != 0u) && (glyphs == NULL)))
		return FB_GFX3_INVALID;
	if ((surface->usage & FB_GFX3_SURFACE_RENDER_TARGET) == 0u)
		return FB_GFX3_UNSUPPORTED;
	if (count == 0u)
		return FB_GFX3_OK;
	for (index = 0u; index < count; ++index) {
		if ((glyphs[index].width == 0u) || (glyphs[index].width > 8u) ||
		    (glyphs[index].height == 0u) || (glyphs[index].height > 16u) ||
		    ((glyphs[index].flags &
		      ~(uint32_t)FB_GFX3_GLYPH_BACKGROUND) != 0u))
			return FB_GFX3_INVALID;
	}
	context = surface->context;
	if ((context == NULL) || (context->submission_mutex == NULL))
		return FB_GFX3_INVALID;

	fb_MutexLock(context->submission_mutex);
	if ((context->pending_line_count != 0u) ||
	    (context->pending_rectangle_count != 0u) ||
	    (context->pending_blit_count != 0u)) {
		result = context_flush_staged_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	if ((context->pending_glyph_count != 0u) &&
	    ((context->pending_glyph_target != surface->handle) ||
	     (memcmp(&context->pending_glyph_clip, clip, sizeof(*clip)) != 0))) {
		result = context_flush_glyphs_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	while (source_index < count) {
		size_t available;
		size_t copy_count;

		if (context->pending_glyph_count ==
		    FB_GFX3_CONTEXT_PENDING_GLYPH_LIMIT) {
			result = context_flush_glyphs_locked(context);
			if (result != FB_GFX3_OK)
				goto done;
		}
		available = FB_GFX3_CONTEXT_PENDING_GLYPH_LIMIT -
			context->pending_glyph_count;
		copy_count = (size_t)count - source_index;
		if (copy_count > available)
			copy_count = available;
		if (context->pending_glyph_count == 0u) {
			context->pending_glyph_target = surface->handle;
			context->pending_glyph_clip = *clip;
		}
		memcpy(context->pending_glyphs + context->pending_glyph_count,
			glyphs + source_index, copy_count * sizeof(glyphs[0]));
		context->pending_glyph_count += copy_count;
		source_index += copy_count;
	}
	context_mark_visible_modified(context, surface->handle);
	atomic_store(&context->pending_submission_work, TRUE);

done:
	fb_MutexUnlock(context->submission_mutex);
	return result;
}

/* See context_stage_rectangle_locked for the outer-lock contract. */
static int context_stage_blit_locked(FB_GFX3_SURFACE *destination,
	const FB_GFX3_RECT *clip, const FB_GFX3_SURFACE *source,
	const FB_GFX3_RECT *source_rect, int destination_x, int destination_y,
	uint32_t mode, uint32_t alpha)
{
	FB_GFX3_CONTEXT *context;
	FB_GFX3_BLIT_COMMAND *blit;
	size_t pending_limit;
	int result;

	context = destination->context;
	pending_limit = context->renderer.backend.caps.max_packed_blits;
	if ((pending_limit == 0u) ||
	    (pending_limit > FB_GFX3_CONTEXT_PENDING_BLIT_LIMIT))
		pending_limit = FB_GFX3_CONTEXT_PENDING_BLIT_LIMIT;
	if (context->pending_line_count != 0u) {
		result = context_flush_lines_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (context->pending_glyph_count != 0u) {
		result = context_flush_glyphs_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (context->pending_rectangle_count != 0u) {
		result = context_flush_rectangles_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	if ((context->pending_blit_count != 0u) &&
	    ((context->pending_blit_target != destination->handle) ||
	     (!context_heterogeneous_blit_packet_supported(context) &&
	      ((context->pending_blit_source != source->handle) ||
	       (context->pending_blit_mode != mode) ||
	       (context->pending_blit_alpha != alpha))))) {
		result = context_flush_blits_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (context->pending_blit_count == pending_limit) {
		result = context_flush_blits_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
		/*
			A full BLITS command is already a backend-sized GPU batch. Submit it
			immediately so the renderer and GPU can consume this packet while the
			BASIC thread fills the next one. Retaining full packets in the local
			pending-command array made POINT observe GPU work only after the whole
			sprite stream had finished, which defeated renderer-thread offloading.
		*/
		result = context_submit_commands_locked(context);
		if (result != FB_GFX3_OK)
			goto done;
	}
	blit = &context->pending_blits[context->pending_blit_count];
	blit->source = source->handle;
	blit->clip = *clip;
	blit->source_rect = *source_rect;
	blit->destination_x = destination_x;
	blit->destination_y = destination_y;
	blit->mode = mode;
	blit->alpha = alpha;
	memset(blit->reserved, 0, sizeof(blit->reserved));
	context->pending_blit_target = destination->handle;
	if (context->pending_blit_count == 0u) {
		context->pending_blit_source = source->handle;
		context->pending_blit_mode = mode;
		context->pending_blit_alpha = alpha;
	}
	context->pending_blit_count++;
	context_mark_visible_modified(context, destination->handle);
	atomic_store(&context->pending_submission_work, TRUE);
	result = FB_GFX3_OK;

done:
	return result;
}

static int context_stage_blit(FB_GFX3_SURFACE *destination,
	const FB_GFX3_RECT *clip, const FB_GFX3_SURFACE *source,
	const FB_GFX3_RECT *source_rect, int destination_x, int destination_y,
	uint32_t mode, uint32_t alpha)
{
	FB_GFX3_CONTEXT *context;
	int result;

	if ((destination == NULL) || (destination->context == NULL) ||
	    (destination->context->submission_mutex == NULL))
		return FB_GFX3_INVALID;
	context = destination->context;
	fb_MutexLock(context->submission_mutex);
	result = context_stage_blit_locked(destination, clip, source, source_rect,
		destination_x, destination_y, mode, alpha);
	fb_MutexUnlock(context->submission_mutex);
	return result;
}

static int context_submit(FB_GFX3_CONTEXT *context, FB_GFX3_COMMAND *command,
	int wait, uint64_t *value)
{
	FB_GFX3_COMPLETION *completion;
	uint32_t command_type;
	int result;

	if ((context == NULL) || !context->renderer_initialized ||
	    (context->submission_mutex == NULL) || (command == NULL) ||
	    ((value != NULL) && !wait)) {
		fb_gfx3_command_destroy(command);
		return FB_GFX3_INVALID;
	}
	command_type = command->type;
	if (!wait) {
		fb_MutexLock(context->submission_mutex);
		result = context_submit_async_locked(context, command);
		if (result == FB_GFX3_OK) {
			command = NULL;
		}
		fb_MutexUnlock(context->submission_mutex);
		if (result != FB_GFX3_OK) {
			fb_gfx3_log_write(&context->logger, FB_GFX3_LOG_ERROR,
				"command %u submission failed: %d", command_type,
				result);
			fb_gfx3_command_destroy(command);
		}
		return result;
	}

	/*
		The command crosses to the renderer thread. Keep its completion on the
		heap so the object lifetime is independent of this producer's stack,
		then release it only after the renderer has signalled the wait.
	*/
	completion = (FB_GFX3_COMPLETION *)malloc(sizeof(*completion));
	if (completion == NULL) {
		fb_gfx3_command_destroy(command);
		return FB_GFX3_OUT_OF_MEMORY;
	}
	result = fb_gfx3_completion_init(completion);
	if (result != FB_GFX3_OK) {
		fb_gfx3_command_destroy(command);
		free(completion);
		return result;
	}
	command->completion = completion;
	fb_MutexLock(context->submission_mutex);
	result = context_submit_pending_locked(context);
	if (result == FB_GFX3_OK) {
		context_mark_command_visible_modified(context, command);
		result = fb_gfx3_renderer_submit(&context->renderer, command, NULL);
	}
	fb_MutexUnlock(context->submission_mutex);
	if (result != FB_GFX3_OK) {
		fb_gfx3_log_write(&context->logger, FB_GFX3_LOG_ERROR,
			"synchronous command %u submission failed: %d",
			command_type, result);
		fb_gfx3_command_destroy(command);
		fb_gfx3_completion_destroy(completion);
		free(completion);
		return result;
	}

	result = fb_gfx3_completion_wait(completion, NULL);
	if ((result != FB_GFX3_OK) &&
	    !((command_type == FB_GFX3_COMMAND_INTEROP_CALLBACK) &&
	      (result == FB_GFX3_UNSUPPORTED)))
		fb_gfx3_log_write(&context->logger, FB_GFX3_LOG_ERROR,
			"synchronous command %u completion failed: %d",
			command_type, result);
	if ((result == FB_GFX3_OK) && (value != NULL))
		result = fb_gfx3_completion_get_value(completion, 0, value);
	fb_gfx3_completion_destroy(completion);
	free(completion);
	return result;
}

static int surface_is_valid(const FB_GFX3_SURFACE *surface)
{
	return (surface != NULL) && (surface->context != NULL) &&
		surface->context->renderer_initialized && (surface->handle != 0) &&
		(surface->width != 0) && (surface->height != 0);
}

static int surface_require_usage(const FB_GFX3_SURFACE *surface,
	uint32_t required_usage)
{
	if (!surface_is_valid(surface))
		return FB_GFX3_INVALID;
	if ((surface->usage & required_usage) != required_usage)
		return FB_GFX3_UNSUPPORTED;
	return FB_GFX3_OK;
}

static uint32_t surface_bytes_per_pixel(uint32_t depth)
{
	switch (depth) {
	case 1:
	case 2:
	case 4:
	case 8:
		return 1;
	case 16:
		return 2;
	case 32:
		return 4;
	default:
		return 0;
	}
}

/* ------------------------------------------------------------------------- */
/* Context lifecycle                                                         */
/* ------------------------------------------------------------------------- */

int fb_gfx3_context_init(FB_GFX3_CONTEXT *context,
	const FB_GFX3_CONTEXT_CONFIG *config)
{
	FB_GFX3_RENDERER_CONFIG renderer_config;
	int log_level;
	int result;

	if ((context == NULL) || (config == NULL) || (config->backend == NULL))
		return FB_GFX3_INVALID;
	memset(context, 0, sizeof(*context));
	atomic_init(&context->visible_surface_handle, 0);
	atomic_init(&context->visible_content_revision, 0u);
	atomic_init(&context->queued_present_revision, 0u);
	atomic_init(&context->pending_submission_work, FALSE);
	result = fb_gfx3_log_init(&context->logger);
	if (result != FB_GFX3_OK)
		return result;
	context->logger_initialized = TRUE;
	log_level = config->log_level;
	if ((log_level < FB_GFX3_LOG_ERROR) ||
	    (log_level > FB_GFX3_LOG_TRACE))
		log_level = FB_GFX3_LOG_WARNING;
	result = fb_gfx3_log_set(&context->logger, log_level,
		config->log_callback, config->log_user_data);
	if (result != FB_GFX3_OK)
		goto fail_logger;
	context->submission_mutex = fb_MutexCreate();
	if (context->submission_mutex == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto fail_logger;
	}
	result = fb_gfx3_completion_init(&context->input_poll_completion);
	if (result != FB_GFX3_OK)
		goto fail_submission_mutex;
	context->input_poll_completion_initialized = TRUE;
	context->input_poll_command =
		fb_gfx3_command_create(FB_GFX3_COMMAND_INPUT_POLL, 0);
	if (context->input_poll_command == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto fail_input_poll_completion;
	}
	context->input_poll_command->flags = FB_GFX3_COMMAND_REUSABLE;
	context->input_poll_command->completion =
		&context->input_poll_completion;

	memset(&renderer_config, 0, sizeof(renderer_config));
	renderer_config.backend = config->backend;
	renderer_config.backend_config.platform = config->platform;
	renderer_config.backend_config.logger = &context->logger;
	renderer_config.backend_config.title = config->title;
	renderer_config.backend_config.width = config->width;
	renderer_config.backend_config.height = config->height;
	renderer_config.backend_config.depth = config->depth;
	renderer_config.backend_config.page_count = config->page_count;
	renderer_config.backend_config.flags = config->flags;
	renderer_config.queue_capacity = config->queue_capacity;
	renderer_config.resource_capacity = config->resource_capacity;
	renderer_config.idle_poll_milliseconds =
		config->idle_poll_milliseconds;
	result = fb_gfx3_renderer_init(&context->renderer, &renderer_config);
	if (result != FB_GFX3_OK)
		goto fail_input_poll_command;
	context->renderer_initialized = TRUE;
	return FB_GFX3_OK;

fail_input_poll_command:
	context->input_poll_command->flags &=
		~(uint32_t)FB_GFX3_COMMAND_REUSABLE;
	fb_gfx3_command_destroy(context->input_poll_command);
	context->input_poll_command = NULL;
fail_input_poll_completion:
	fb_gfx3_completion_destroy(&context->input_poll_completion);
	context->input_poll_completion_initialized = FALSE;
fail_submission_mutex:
	fb_MutexDestroy(context->submission_mutex);
	context->submission_mutex = NULL;
fail_logger:
	fb_gfx3_log_destroy(&context->logger);
	memset(context, 0, sizeof(*context));
	return result;
}

int fb_gfx3_context_shutdown(FB_GFX3_CONTEXT *context)
{
	int destroy_result;
	int result = FB_GFX3_OK;
	uint32_t index;

	if (context == NULL)
		return FB_GFX3_INVALID;
	if (context->renderer_initialized) {
		fb_MutexLock(context->submission_mutex);
		destroy_result = context_submit_pending_locked(context);
		fb_MutexUnlock(context->submission_mutex);
		if (destroy_result != FB_GFX3_OK)
			result = destroy_result;
		for (index = 0; index < FB_GFX3_IMAGE_CACHE_CAPACITY; index++) {
			FB_GFX3_IMAGE_CACHE_ENTRY *entry =
				&context->image_cache[index];

			if ((entry->surface.handle != 0) && !entry->uses_atlas) {
				destroy_result = fb_gfx3_surface_destroy(&entry->surface);
				if ((result == FB_GFX3_OK) &&
				    (destroy_result != FB_GFX3_OK))
					result = destroy_result;
			}
			free(entry->snapshot);
			entry->snapshot = NULL;
			entry->snapshot_size = 0;
		}
		if (context->image_cache_atlas.handle != 0) {
			destroy_result = fb_gfx3_surface_destroy(
				&context->image_cache_atlas);
			if ((result == FB_GFX3_OK) && (destroy_result != FB_GFX3_OK))
				result = destroy_result;
		}
		/*
			The image upload surface is context-owned rather than application
				owned. Retire it before stopping the renderer so its GPU resource
				can be released on the render thread after every queued blit.
		*/
		if (context->image_upload_surface.handle != 0) {
			destroy_result = fb_gfx3_surface_destroy(
				&context->image_upload_surface);
			if (destroy_result != FB_GFX3_OK)
				result = destroy_result;
		}
		destroy_result = fb_gfx3_renderer_shutdown(&context->renderer);
		if (result == FB_GFX3_OK)
			result = destroy_result;
		context->renderer_initialized = FALSE;
	}
	if (context->input_poll_command != NULL) {
		context->input_poll_command->flags &=
			~(uint32_t)FB_GFX3_COMMAND_REUSABLE;
		fb_gfx3_command_destroy(context->input_poll_command);
		context->input_poll_command = NULL;
	}
	if (context->input_poll_completion_initialized) {
		fb_gfx3_completion_destroy(&context->input_poll_completion);
		context->input_poll_completion_initialized = FALSE;
	}
	if (context->submission_mutex != NULL) {
		fb_MutexDestroy(context->submission_mutex);
		context->submission_mutex = NULL;
	}
	if (context->logger_initialized) {
		fb_gfx3_log_destroy(&context->logger);
		context->logger_initialized = FALSE;
	}
	memset(context, 0, sizeof(*context));
	return result;
}

int fb_gfx3_context_flush(FB_GFX3_CONTEXT *context)
{
	FB_GFX3_COMMAND *command;

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_BARRIER, 0);
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	return context_submit(context, command, TRUE, NULL);
}

int fb_gfx3_context_submit_pending(FB_GFX3_CONTEXT *context)
{
	int result;

	if ((context == NULL) || !context->renderer_initialized ||
	    (context->submission_mutex == NULL))
		return FB_GFX3_INVALID;
	/*
		A logical frame boundary must make queued work visible to the renderer
		without turning into a GPU wait. The submission mutex protects the staged
		packet arrays; the renderer queue takes ownership before this call returns.
	*/
	fb_MutexLock(context->submission_mutex);
	result = context_submit_pending_locked(context);
	fb_MutexUnlock(context->submission_mutex);
	return result;
}

int fb_gfx3_context_poll_platform(FB_GFX3_CONTEXT *context)
{
	int result;

	if ((context == NULL) || !context->renderer_initialized ||
	    (context->submission_mutex == NULL) ||
	    (context->input_poll_command == NULL) ||
	    !context->input_poll_completion_initialized)
		return FB_GFX3_INVALID;
	/*
		The completion means that the window owner pumped its native queue. This
		input-only command deliberately skips backend housekeeping and any wait for
		preceding GPU work. The periodic PLATFORM_POLL still owns resize, overlay,
		and completed-submission maintenance.

		FB_GRAPHICS_LOCK serializes public SCREENEVENT calls. Reusing the control
		record here is therefore safe, and avoids allocating a command, mutex, and
		condition variable for every empty game-loop poll.
	*/
	fb_MutexLock(context->submission_mutex);
	result = context_submit_pending_locked(context);
	if (result == FB_GFX3_OK)
		result = fb_gfx3_completion_reset(&context->input_poll_completion);
	if (result == FB_GFX3_OK)
		result = fb_gfx3_renderer_submit(&context->renderer,
			context->input_poll_command, NULL);
	fb_MutexUnlock(context->submission_mutex);
	if (result != FB_GFX3_OK)
		return result;
	return fb_gfx3_completion_wait(&context->input_poll_completion, NULL);
}

int fb_gfx3_context_set_palette(FB_GFX3_CONTEXT *context,
	const uint32_t *palette)
{
	FB_GFX3_PALETTE_COMMAND *payload;
	FB_GFX3_COMMAND *command;

	if ((context == NULL) || !context->renderer_initialized ||
	    (palette == NULL))
		return FB_GFX3_INVALID;
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_PALETTE,
		sizeof(*payload));
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	payload = (FB_GFX3_PALETTE_COMMAND *)command->payload;
	memcpy(payload->color, palette, sizeof(payload->color));
	return context_submit(context, command, FALSE, NULL);
}

int fb_gfx3_context_set_window_title(FB_GFX3_CONTEXT *context,
	const char *title, size_t length)
{
	FB_GFX3_WINDOW_TITLE_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	size_t payload_size;
	size_t title_size;

	if ((context == NULL) || !context->renderer_initialized ||
	    (title == NULL) || (length > UINT32_MAX) ||
	    (fb_gfx3_size_add(length, 1u, &title_size) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(sizeof(*payload), title_size,
	     &payload_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_WINDOW_TITLE,
		payload_size);
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	payload = (FB_GFX3_WINDOW_TITLE_COMMAND *)command->payload;
	payload->length = (uint32_t)length;
	payload->reserved = 0;
	memcpy(payload->title, title, length);
	payload->title[length] = '\0';
	return context_submit(context, command, FALSE, NULL);
}

int fb_gfx3_context_run_interop_callback(FB_GFX3_CONTEXT *context,
	FB_GFX3_INTEROP_CALLBACK callback, void *user_data)
{
	FB_GFX3_INTEROP_CALLBACK_COMMAND *payload;
	FB_GFX3_COMMAND *command;

	if ((context == NULL) || !context->renderer_initialized ||
	    (callback == NULL))
		return FB_GFX3_INVALID;
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_INTEROP_CALLBACK,
		sizeof(*payload));
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	payload = (FB_GFX3_INTEROP_CALLBACK_COMMAND *)command->payload;
	payload->callback = (uintptr_t)callback;
	payload->user_data = (uintptr_t)user_data;
	return context_submit(context, command, TRUE, NULL);
}

/* ------------------------------------------------------------------------- */
/* Surface lifecycle and transfers                                           */
/* ------------------------------------------------------------------------- */

int fb_gfx3_surface_create(FB_GFX3_CONTEXT *context,
	FB_GFX3_SURFACE *surface, uint32_t width, uint32_t height,
	uint32_t depth, uint32_t usage, uint32_t clear_color)
{
	const uint32_t supported_usage =
		FB_GFX3_SURFACE_RENDER_TARGET |
		FB_GFX3_SURFACE_SAMPLED |
		FB_GFX3_SURFACE_TRANSFER_SOURCE |
		FB_GFX3_SURFACE_TRANSFER_DESTINATION |
		FB_GFX3_SURFACE_CPU_VISIBLE;
	FB_GFX3_SURFACE_CREATE_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	uint64_t handle = 0;
	int result;

	if ((context == NULL) || !context->renderer_initialized ||
	    (surface == NULL) || (width == 0) || (height == 0) ||
	    (usage == 0) || ((usage & ~supported_usage) != 0u) ||
	    (surface_bytes_per_pixel(depth) == 0))
		return FB_GFX3_INVALID;
	memset(surface, 0, sizeof(*surface));
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_SURFACE_CREATE,
		sizeof(*payload));
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	payload = (FB_GFX3_SURFACE_CREATE_COMMAND *)command->payload;
	payload->width = width;
	payload->height = height;
	payload->depth = depth;
	payload->usage = usage;
	payload->clear_color = clear_color;
	result = context_submit(context, command, TRUE, &handle);
	if (result != FB_GFX3_OK)
		return result;

	surface->context = context;
	surface->handle = handle;
	surface->width = width;
	surface->height = height;
	surface->depth = depth;
	surface->usage = usage;
	return FB_GFX3_OK;
}

int fb_gfx3_surface_destroy(FB_GFX3_SURFACE *surface)
{
	FB_GFX3_COMMAND *command;
	int result;

	if (!surface_is_valid(surface))
		return FB_GFX3_INVALID;
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_SURFACE_DESTROY, 0);
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = surface->handle;
	result = context_submit(surface->context, command, TRUE, NULL);
	if (result == FB_GFX3_OK)
		memset(surface, 0, sizeof(*surface));
	return result;
}

int fb_gfx3_surface_set_visible(FB_GFX3_SURFACE *surface)
{
	FB_GFX3_CONTEXT *context;
	FB_GFX3_PAGE_SET_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	int result;

	result = surface_require_usage(surface, FB_GFX3_SURFACE_SAMPLED);
	if (result != FB_GFX3_OK)
		return result;
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_PAGE_SET,
		sizeof(*payload));
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = surface->handle;
	payload = (FB_GFX3_PAGE_SET_COMMAND *)command->payload;
	payload->width = surface->width;
	payload->height = surface->height;
	payload->depth = surface->depth;
	context = surface->context;
	fb_MutexLock(context->submission_mutex);
	result = context_submit_async_locked(context, command);
	if (result == FB_GFX3_OK) {
		atomic_store(&context->visible_surface_handle, surface->handle);
		atomic_fetch_add(&context->visible_content_revision, 1u);
	} else {
		fb_gfx3_command_destroy(command);
	}
	fb_MutexUnlock(context->submission_mutex);
	return result;
}

int fb_gfx3_surface_present(FB_GFX3_SURFACE *surface, int wait)
{
	FB_GFX3_CONTEXT *context;
	FB_GFX3_COMMAND *command;
	uint64_t content_revision;
	int result;

	result = surface_require_usage(surface, FB_GFX3_SURFACE_SAMPLED);
	if (result != FB_GFX3_OK)
		return result;
	context = surface->context;
	if (wait) {
		command = fb_gfx3_command_create(FB_GFX3_COMMAND_PRESENT, 0);
		if (command == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		command->target = surface->handle;
		return context_submit(context, command, TRUE, NULL);
	}

	/*
		The view-update hook may request an asynchronous presentation on every
		SLEEP 0. The fully clean path needs only atomic reads, so an idle loop does
		not contend with the render thread or enter the allocator. If producer
		work remains, recheck under submission_mutex before flushing it or
		queueing a real PRESENT.
	*/
	content_revision = atomic_load(&context->visible_content_revision);
	if ((atomic_load(&context->visible_surface_handle) == surface->handle) &&
	    (atomic_load(&context->queued_present_revision) ==
	     content_revision) &&
	    !atomic_load(&context->pending_submission_work))
		return FB_GFX3_OK;

	fb_MutexLock(context->submission_mutex);
	content_revision = atomic_load(&context->visible_content_revision);
	if ((atomic_load(&context->visible_surface_handle) == surface->handle) &&
	    (atomic_load(&context->queued_present_revision) ==
	     content_revision)) {
		result = context_submit_pending_locked(context);
		fb_MutexUnlock(context->submission_mutex);
		return result;
	}
	fb_MutexUnlock(context->submission_mutex);

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_PRESENT, 0);
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = surface->handle;
	fb_MutexLock(context->submission_mutex);
	content_revision = atomic_load(&context->visible_content_revision);
	if ((atomic_load(&context->visible_surface_handle) == surface->handle) &&
	    (atomic_load(&context->queued_present_revision) ==
	     content_revision)) {
		fb_gfx3_command_destroy(command);
		result = context_submit_pending_locked(context);
		fb_MutexUnlock(context->submission_mutex);
		return result;
	}
	result = context_submit_async_locked(context, command);
	if (result == FB_GFX3_OK) {
		command = NULL;
		result = context_submit_pending_locked(context);
	}
	if ((result == FB_GFX3_OK) &&
	    (atomic_load(&context->visible_surface_handle) == surface->handle))
		atomic_store(&context->queued_present_revision, content_revision);
	if (command != NULL)
		fb_gfx3_command_destroy(command);
	fb_MutexUnlock(context->submission_mutex);
	/*
		An asynchronous PRESENT is a frame-production boundary, not merely
		another command to collect for a later batch. SCREENSET and SCREENCOPY
		already flush at this point. Give the explicit GPU-surface API the same
		contract so a producer which only uploads and presents cannot leave its
		first frame parked in the pending command array.
	*/
	return result;
}

int fb_gfx3_surface_upload(FB_GFX3_SURFACE *surface, int x, int y,
	uint32_t width, uint32_t height, uint32_t pitch, const void *pixels)
{
	FB_GFX3_SURFACE_UPLOAD_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	size_t data_size;
	size_t payload_size;
	size_t row_size;
	uint32_t bytes_per_pixel;

	if (!surface_is_valid(surface) || (pixels == NULL) || (x < 0) ||
	    (y < 0) || (width == 0) || (height == 0) ||
	    ((uint64_t)(uint32_t)x + width > surface->width) ||
	    ((uint64_t)(uint32_t)y + height > surface->height))
		return FB_GFX3_INVALID;
	if ((surface->usage & FB_GFX3_SURFACE_TRANSFER_DESTINATION) == 0)
		return FB_GFX3_UNSUPPORTED;
	bytes_per_pixel = surface_bytes_per_pixel(surface->depth);
	if ((fb_gfx3_size_multiply(width, bytes_per_pixel, &row_size) !=
	     FB_GFX3_OK) || (pitch < row_size) ||
	    (fb_gfx3_size_multiply(pitch, height, &data_size) != FB_GFX3_OK) ||
	    (data_size > UINT32_MAX) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_SURFACE_UPLOAD_COMMAND, data),
	     data_size, &payload_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_SURFACE_UPLOAD,
		payload_size);
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = surface->handle;
	payload = (FB_GFX3_SURFACE_UPLOAD_COMMAND *)command->payload;
	payload->destination_x = x;
	payload->destination_y = y;
	payload->width = width;
	payload->height = height;
	payload->source_pitch = pitch;
	payload->data_size = (uint32_t)data_size;
	memcpy(payload->data, pixels, data_size);
	return context_submit(surface->context, command, FALSE, NULL);
}

int fb_gfx3_surface_download(FB_GFX3_SURFACE *surface, int x, int y,
	uint32_t width, uint32_t height, uint32_t pitch, void *pixels)
{
	FB_GFX3_SURFACE_DOWNLOAD_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	size_t destination_size;
	size_t row_size;
	uint32_t bytes_per_pixel;

	if (!surface_is_valid(surface) || (pixels == NULL) || (x < 0) ||
	    (y < 0) || (width == 0) || (height == 0) ||
	    ((uint64_t)(uint32_t)x + width > surface->width) ||
	    ((uint64_t)(uint32_t)y + height > surface->height))
		return FB_GFX3_INVALID;
	if ((surface->usage & FB_GFX3_SURFACE_TRANSFER_SOURCE) == 0)
		return FB_GFX3_UNSUPPORTED;
	bytes_per_pixel = surface_bytes_per_pixel(surface->depth);
	if ((fb_gfx3_size_multiply(width, bytes_per_pixel, &row_size) !=
	     FB_GFX3_OK) || (pitch < row_size) ||
	    (fb_gfx3_size_multiply(pitch, height, &destination_size) !=
	     FB_GFX3_OK) || (destination_size > UINT32_MAX))
		return FB_GFX3_INVALID;

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_SURFACE_DOWNLOAD,
		sizeof(*payload));
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = surface->handle;
	payload = (FB_GFX3_SURFACE_DOWNLOAD_COMMAND *)command->payload;
	payload->source_x = x;
	payload->source_y = y;
	payload->width = width;
	payload->height = height;
	payload->destination_pitch = pitch;
	payload->destination_size = (uint32_t)destination_size;
	payload->destination_address = (uint64_t)(uintptr_t)pixels;
	return context_submit(surface->context, command, TRUE, NULL);
}

/* ------------------------------------------------------------------------- */
/* Queued drawing                                                            */
/* ------------------------------------------------------------------------- */

int fb_gfx3_surface_clear(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, uint32_t color, uint32_t flags)
{
	FB_GFX3_CLEAR_COMMAND *payload;
	FB_GFX3_COMMAND *command;

	if (!surface_is_valid(surface) || (clip == NULL))
		return FB_GFX3_INVALID;
	if ((surface->usage & FB_GFX3_SURFACE_RENDER_TARGET) == 0)
		return FB_GFX3_UNSUPPORTED;
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_CLEAR,
		sizeof(*payload));
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = surface->handle;
	payload = (FB_GFX3_CLEAR_COMMAND *)command->payload;
	payload->clip = *clip;
	payload->color = color;
	payload->flags = flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	return context_submit(surface->context, command, FALSE, NULL);
}

int fb_gfx3_surface_points(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, const FB_GFX3_POINT *points, uint32_t count)
{
	FB_GFX3_POINTS_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	size_t points_size;
	size_t payload_size;

	if (!surface_is_valid(surface) || (clip == NULL) ||
	    ((count != 0) && (points == NULL)))
		return FB_GFX3_INVALID;
	if ((surface->usage & FB_GFX3_SURFACE_RENDER_TARGET) == 0)
		return FB_GFX3_UNSUPPORTED;
	if (count == 0)
		return FB_GFX3_OK;
	if ((fb_gfx3_size_multiply(count, sizeof(*points), &points_size) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point),
	     points_size, &payload_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_POINTS, payload_size);
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = surface->handle;
	payload = (FB_GFX3_POINTS_COMMAND *)command->payload;
	payload->clip = *clip;
	payload->count = count;
	memcpy(payload->point, points, points_size);
	return context_submit(surface->context, command, FALSE, NULL);
}

int fb_gfx3_surface_glyphs(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, const FB_GFX3_GLYPH *glyphs, uint32_t count)
{
	return context_stage_glyphs(surface, clip, glyphs, count);
}

static int context_surface_line(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style, uint32_t flags, int graphics_locked)
{
	FB_GFX3_CONTEXT *context;
	FB_GFX3_LINE_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	int result;

	if (!surface_is_valid(surface) || (clip == NULL))
		return FB_GFX3_INVALID;
	if ((surface->usage & FB_GFX3_SURFACE_RENDER_TARGET) == 0)
		return FB_GFX3_UNSUPPORTED;
	context = surface->context;
	if (((flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) == 0u) &&
	    ((context->renderer.backend.caps.features &
	      FB_GFX3_FEATURE_PACKED_LINES) != 0u)) {
		if (!graphics_locked)
			fb_MutexLock(context->submission_mutex);
		if ((context->pending_glyph_count != 0u) ||
		    (context->pending_rectangle_count != 0u) ||
		    (context->pending_blit_count != 0u)) {
			result = context_flush_staged_locked(context);
			if (result != FB_GFX3_OK)
				goto staged_done;
		}
		if ((context->pending_line_count != 0u) &&
		    ((context->pending_line_target != surface->handle) ||
		     (context->pending_lines[0].style != (style & 0xFFFFu)) ||
		     (memcmp(&context->pending_lines[0].clip, clip,
		      sizeof(*clip)) != 0))) {
			result = context_flush_lines_locked(context);
			if (result != FB_GFX3_OK)
				goto staged_done;
		}
		if (context->pending_line_count ==
		    FB_GFX3_CONTEXT_PENDING_LINE_LIMIT) {
			result = context_flush_lines_locked(context);
			if (result != FB_GFX3_OK)
				goto staged_done;
			/*
				Let the renderer consume a full shader-sized packet while the
				producer fills the next one.
			*/
			result = context_submit_commands_locked(context);
			if (result != FB_GFX3_OK)
				goto staged_done;
		}
		payload = &context->pending_lines[context->pending_line_count++];
		payload->clip = *clip;
		payload->x1 = x1;
		payload->y1 = y1;
		payload->x2 = x2;
		payload->y2 = y2;
		payload->color = color;
		payload->style = style & 0xFFFFu;
		payload->flags = 0u;
		payload->reserved = 0u;
		context->pending_line_target = surface->handle;
		context_mark_visible_modified(context, surface->handle);
		atomic_store(&context->pending_submission_work, TRUE);
		result = FB_GFX3_OK;

staged_done:
		if (!graphics_locked)
			fb_MutexUnlock(context->submission_mutex);
		return result;
	}
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_LINE,
		sizeof(*payload));
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = surface->handle;
	payload = (FB_GFX3_LINE_COMMAND *)command->payload;
	payload->clip = *clip;
	payload->x1 = x1;
	payload->y1 = y1;
	payload->x2 = x2;
	payload->y2 = y2;
	payload->color = color;
	payload->style = style & 0xFFFFu;
	payload->flags = flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	if (graphics_locked) {
		result = context_submit_async_locked(surface->context, command);
		if (result != FB_GFX3_OK)
			fb_gfx3_command_destroy(command);
		return result;
	}
	return context_submit(surface->context, command, FALSE, NULL);
}

int fb_gfx3_surface_line(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style, uint32_t flags)
{
	return context_surface_line(surface, clip, x1, y1, x2, y2, color,
		style, flags, FALSE);
}

int fb_gfx3_surface_line_graphics_locked(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style, uint32_t flags)
{
	return context_surface_line(surface, clip, x1, y1, x2, y2, color,
		style, flags, TRUE);
}

static int context_surface_rectangle(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style, int filled, uint32_t flags,
	int graphics_locked)
{
	FB_GFX3_RECTANGLE_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	uint32_t features;

	if (!surface_is_valid(surface) || (clip == NULL) ||
	    (x1 > x2) || (y1 > y2))
		return FB_GFX3_INVALID;
	if ((surface->usage & FB_GFX3_SURFACE_RENDER_TARGET) == 0)
		return FB_GFX3_UNSUPPORTED;
	/*
		Desktop GPU backends rasterize an opaque rectangle packet in submission
		order. PACKED_RECTANGLES extends that path to outline boxes. GLES retains
		its existing per-command route, whose instanced path is faster for the
		small sprites used on the physical API-24 device.
	*/
	features = surface->context->renderer.backend.caps.features;
	if (((flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) == 0u) &&
	    ((((features & FB_GFX3_FEATURE_PACKED_RECTANGLES) != 0u) &&
	      ((filled != 0) || ((style & 0xFFFFu) == 0xFFFFu) ||
	       ((features & FB_GFX3_FEATURE_PACKED_STYLED_RECTANGLES) != 0u))) ||
	     ((filled != 0) &&
	      ((features & FB_GFX3_FEATURE_COMPUTE) != 0u)))) {
		if (graphics_locked)
			return context_stage_rectangle_locked(surface, clip, x1, y1, x2,
				y2, color, style, filled, flags);
		return context_stage_rectangle(surface, clip, x1, y1, x2, y2,
			color, style, filled, flags);
	}
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_RECTANGLE,
		sizeof(*payload));
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = surface->handle;
	payload = (FB_GFX3_RECTANGLE_COMMAND *)command->payload;
	payload->clip = *clip;
	payload->x1 = x1;
	payload->y1 = y1;
	payload->x2 = x2;
	payload->y2 = y2;
	payload->color = color;
	payload->style = style & 0xFFFFu;
	payload->filled = (filled != 0);
	payload->flags = flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	if (graphics_locked) {
		int result = context_submit_async_locked(surface->context, command);

		if (result != FB_GFX3_OK)
			fb_gfx3_command_destroy(command);
		return result;
	}
	return context_submit(surface->context, command, FALSE, NULL);
}

int fb_gfx3_surface_rectangle(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style, int filled, uint32_t flags)
{
	return context_surface_rectangle(surface, clip, x1, y1, x2, y2, color,
		style, filled, flags, FALSE);
}

int fb_gfx3_surface_rectangle_graphics_locked(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style, int filled, uint32_t flags)
{
	return context_surface_rectangle(surface, clip, x1, y1, x2, y2, color,
		style, filled, flags, TRUE);
}

int fb_gfx3_surface_rectangles(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECTANGLE_COMMAND *rectangles, uint32_t count)
{
	return context_stage_rectangles(surface, rectangles, count);
}

static int context_surface_ellipse(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int center_x, int center_y,
	float radius_x, float radius_y, uint32_t color, int filled,
	uint32_t flags, int graphics_locked)
{
	FB_GFX3_ELLIPSE_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	int result;

	/*
		The first compute implementation uses exact integer-valued arithmetic
		inside a double-precision shader. This bound keeps every midpoint
		intermediate exactly representable and rejects pathological commands
		before either backend begins a very long loop.
	*/
	if (!surface_is_valid(surface) || (clip == NULL) ||
	    !isfinite(radius_x) || !isfinite(radius_y) ||
	    (radius_x < 0.0f) || (radius_y < 0.0f) ||
	    (radius_x > 32767.0f) || (radius_y > 32767.0f))
		return FB_GFX3_INVALID;
	if ((surface->usage & FB_GFX3_SURFACE_RENDER_TARGET) == 0)
		return FB_GFX3_UNSUPPORTED;
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_ELLIPSE,
		sizeof(*payload));
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = surface->handle;
	payload = (FB_GFX3_ELLIPSE_COMMAND *)command->payload;
	payload->clip = *clip;
	payload->center_x = center_x;
	payload->center_y = center_y;
	payload->radius_x = radius_x;
	payload->radius_y = radius_y;
	payload->color = color;
	payload->filled = (filled != 0);
	payload->flags = flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	if (graphics_locked) {
		result = context_submit_async_locked(surface->context, command);
		if (result != FB_GFX3_OK)
			fb_gfx3_command_destroy(command);
		return result;
	}
	return context_submit(surface->context, command, FALSE, NULL);
}

int fb_gfx3_surface_ellipse(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int center_x, int center_y,
	float radius_x, float radius_y, uint32_t color, int filled,
	uint32_t flags)
{
	return context_surface_ellipse(surface, clip, center_x, center_y,
		radius_x, radius_y, color, filled, flags, FALSE);
}

int fb_gfx3_surface_ellipse_graphics_locked(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int center_x, int center_y,
	float radius_x, float radius_y, uint32_t color, int filled,
	uint32_t flags)
{
	return context_surface_ellipse(surface, clip, center_x, center_y,
		radius_x, radius_y, color, filled, flags, TRUE);
}

int fb_gfx3_surface_paint(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x, int y, uint32_t color,
	uint32_t border_color, uint32_t flags, uint32_t paint_mode,
	const unsigned char *pattern, size_t pattern_size,
	uint32_t pattern_origin_x, uint32_t pattern_origin_y)
{
	FB_GFX3_PAINT_COMMAND *payload;
	FB_GFX3_COMMAND *command;

	if (!surface_is_valid(surface) || (clip == NULL))
		return FB_GFX3_INVALID;
	if ((pattern_size > sizeof(payload->pattern_word)) ||
	    ((pattern_size != 0) && (pattern == NULL)))
		return FB_GFX3_INVALID;
	if ((surface->usage & FB_GFX3_SURFACE_RENDER_TARGET) == 0)
		return FB_GFX3_UNSUPPORTED;
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_PAINT,
		sizeof(*payload));
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = surface->handle;
	payload = (FB_GFX3_PAINT_COMMAND *)command->payload;
	payload->clip = *clip;
	payload->x = x;
	payload->y = y;
	payload->color = color;
	payload->border_color = border_color;
	payload->flags = flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	payload->paint_mode = paint_mode;
	payload->pattern_size = (uint32_t)pattern_size;
	payload->pattern_origin_x = pattern_origin_x;
	payload->pattern_origin_y = pattern_origin_y;
	if (pattern_size != 0) {
		size_t index;

		/* The shader protocol is explicitly little-endian, not host-packed. */
		for (index = 0; index < pattern_size; index++)
			payload->pattern_word[index / 4u] |=
				(uint32_t)pattern[index] << ((index & 3u) * 8u);
	}
	return context_submit(surface->context, command, FALSE, NULL);
}

static int context_surface_blit(FB_GFX3_SURFACE *destination,
	const FB_GFX3_RECT *clip, const FB_GFX3_SURFACE *source,
	const FB_GFX3_RECT *source_rect, int destination_x, int destination_y,
	uint32_t mode, uint32_t alpha, int graphics_locked)
{
	FB_GFX3_BLIT_COMMAND *payload;
	FB_GFX3_COMMAND *command;

	if (!surface_is_valid(destination) || !surface_is_valid(source) ||
	    (clip == NULL) || (source_rect == NULL) ||
	    (destination->context != source->context) ||
	    (destination->depth != source->depth) ||
	    (mode > FB_GFX3_BLIT_BLEND) || (mode == FB_GFX3_BLIT_CUSTOM))
		return FB_GFX3_INVALID;
	if ((destination->usage & FB_GFX3_SURFACE_RENDER_TARGET) == 0 ||
	    (source->usage & FB_GFX3_SURFACE_SAMPLED) == 0)
		return FB_GFX3_UNSUPPORTED;
	/*
		Every production backend consumes the bounded producer packet. Desktop
		compute backends replay it through their ordered blit shaders. GLES
		decodes the same packet into consecutive instanced raster runs, retaining
		the exact clip and FIFO boundaries carried by the public calls.
	*/
	if ((destination != source) &&
	    context_blit_packet_supported(destination->context, mode)) {
		if (graphics_locked)
			return context_stage_blit_locked(destination, clip, source,
				source_rect, destination_x, destination_y, mode, alpha);
		return context_stage_blit(destination, clip, source, source_rect,
			destination_x, destination_y, mode, alpha);
	}
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_BLIT,
		sizeof(*payload));
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = destination->handle;
	payload = (FB_GFX3_BLIT_COMMAND *)command->payload;
	payload->source = source->handle;
	payload->clip = *clip;
	payload->source_rect = *source_rect;
	payload->destination_x = destination_x;
	payload->destination_y = destination_y;
	payload->mode = mode;
	payload->alpha = alpha;
	return context_submit(destination->context, command, FALSE, NULL);
}

int fb_gfx3_surface_blit(FB_GFX3_SURFACE *destination,
	const FB_GFX3_RECT *clip, const FB_GFX3_SURFACE *source,
	const FB_GFX3_RECT *source_rect, int destination_x, int destination_y,
	uint32_t mode, uint32_t alpha)
{
	return context_surface_blit(destination, clip, source, source_rect,
		destination_x, destination_y, mode, alpha, FALSE);
}

int fb_gfx3_surface_blit_graphics_locked(FB_GFX3_SURFACE *destination,
	const FB_GFX3_RECT *clip, const FB_GFX3_SURFACE *source,
	const FB_GFX3_RECT *source_rect, int destination_x, int destination_y,
	uint32_t mode, uint32_t alpha)
{
	return context_surface_blit(destination, clip, source, source_rect,
		destination_x, destination_y, mode, alpha, TRUE);
}

int fb_gfx3_surface_transform_blit(FB_GFX3_SURFACE *destination,
	const FB_GFX3_RECT *clip, const FB_GFX3_SURFACE *source,
	const FB_GFX3_RECT *source_rect, const FB_GFX3_RECT *destination_bounds,
	const float inverse[9], uint32_t mode, uint32_t alpha,
	uint32_t filter, uint32_t wrap)
{
	FB_GFX3_TRANSFORM_BLIT_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	size_t index;

	if (!surface_is_valid(destination) || !surface_is_valid(source) ||
	    (clip == NULL) || (source_rect == NULL) ||
	    (destination_bounds == NULL) || (inverse == NULL) ||
	    (destination->context != source->context) ||
	    (destination->depth != source->depth) ||
	    (mode > FB_GFX3_BLIT_BLEND) || (mode == FB_GFX3_BLIT_CUSTOM) ||
	    (filter > FB_GFX3_TRANSFORM_FILTER_LINEAR) ||
	    (wrap > FB_GFX3_TRANSFORM_WRAP_REPEAT))
		return FB_GFX3_INVALID;
	if ((destination->usage & FB_GFX3_SURFACE_RENDER_TARGET) == 0 ||
	    (source->usage & FB_GFX3_SURFACE_SAMPLED) == 0)
		return FB_GFX3_UNSUPPORTED;
	for (index = 0; index < 9u; ++index) {
		if (!isfinite(inverse[index]))
			return FB_GFX3_INVALID;
	}
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_TRANSFORM_BLIT,
		sizeof(*payload));
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = destination->handle;
	payload = (FB_GFX3_TRANSFORM_BLIT_COMMAND *)command->payload;
	payload->source = source->handle;
	payload->clip = *clip;
	payload->source_rect = *source_rect;
	payload->destination_bounds = *destination_bounds;
	memcpy(payload->inverse, inverse, sizeof(payload->inverse));
	payload->mode = mode;
	payload->alpha = alpha;
	payload->filter = filter;
	payload->wrap = wrap;
	return context_submit(destination->context, command, FALSE, NULL);
}

int fb_gfx3_surface_read_pixel(FB_GFX3_SURFACE *surface, int x, int y,
	uint32_t *color)
{
	FB_GFX3_READ_PIXEL_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	uint64_t value;
	int result;

	if (!surface_is_valid(surface) || (color == NULL))
		return FB_GFX3_INVALID;
	if ((surface->usage & FB_GFX3_SURFACE_TRANSFER_SOURCE) == 0)
		return FB_GFX3_UNSUPPORTED;
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_READ_PIXEL,
		sizeof(*payload));
	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	command->target = surface->handle;
	payload = (FB_GFX3_READ_PIXEL_COMMAND *)command->payload;
	payload->x = x;
	payload->y = y;
	result = context_submit(surface->context, command, TRUE, &value);
	if (result == FB_GFX3_OK)
		*color = (uint32_t)value;
	return result;
}

/* end of gfx3_context.c */
