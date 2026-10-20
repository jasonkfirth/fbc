/*
 * FreeBASIC gfxlib3: gfx3_opengl_draw.c
 * Purpose: Dispatch individual OpenGL geometric and paint commands.
 * Responsibilities: render-thread operations within this module.
 * This file intentionally does NOT contain BASIC compatibility or native input dispatch.
 */

#include "gfx3_opengl_private.h"

#if (defined(HOST_WIN32) || defined(HOST_LINUX)) && !defined(DISABLE_OPENGL)

int fb_gfx3_gl_points(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_POINTS_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *surface;
	FB_GFX3_RECT clip;
	size_t points_size;
	size_t expected_size;
	uint32_t groups;
	int result;

	if (fb_gfx3_command_payload_size(command) <
	    offsetof(FB_GFX3_POINTS_COMMAND, point))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_POINTS_COMMAND *)command->payload;
	if ((fb_gfx3_size_multiply(payload->count, sizeof(payload->point[0]),
	     &points_size) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point), points_size,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)) ||
	    (points_size > state->maximum_storage_buffer_size))
		return FB_GFX3_INVALID;
	if (payload->count == 0)
		return FB_GFX3_OK;
	groups = (payload->count + FB_GFX3_OPENGL_POINT_GROUP_SIZE - 1) /
		FB_GFX3_OPENGL_POINT_GROUP_SIZE;
	if (groups > state->maximum_compute_groups_x)
		return FB_GFX3_UNSUPPORTED;

	result = fb_gfx3_gl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!fb_gfx3_gl_clip_rect(surface, &payload->clip, &clip)) {
		fb_gfx3_resource_release(state->resources, command->target);
		return FB_GFX3_OK;
	}

	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->point_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)points_size,
		payload->point, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 1,
		state->point_buffer);
	state->gl.use_program(state->points_program);
	state->gl.bind_image_texture(0, surface->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_4i(state->points_clip_location, clip.x1, clip.y1,
		clip.x2, clip.y2);
	state->gl.uniform_1ui(state->points_count_location, payload->count);
	state->gl.uniform_1ui(state->points_mask_location,
		fb_gfx3_gl_color_mask(surface->depth));
	state->gl.dispatch_compute(groups, 1, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_BUFFER_UPDATE_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "point compute dispatch");
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

int fb_gfx3_gl_line(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_LINE_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *surface;
	FB_GFX3_RECT clip;
	int64_t difference_x;
	int64_t difference_y;
	uint32_t count;
	uint32_t groups;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_LINE_COMMAND *)command->payload;
	difference_x = (int64_t)payload->x2 - payload->x1;
	difference_y = (int64_t)payload->y2 - payload->y1;
	if (difference_x < 0)
		difference_x = -difference_x;
	if (difference_y < 0)
		difference_y = -difference_y;
	/* The shader's closed-form integer products are safe within this bound. */
	if ((difference_x > 32767) || (difference_y > 32767))
		return FB_GFX3_UNSUPPORTED;
	count = (uint32_t)((difference_x > difference_y) ?
		difference_x : difference_y) + 1;
	groups = (count + FB_GFX3_OPENGL_POINT_GROUP_SIZE - 1) /
		FB_GFX3_OPENGL_POINT_GROUP_SIZE;
	if (groups > state->maximum_compute_groups_x)
		return FB_GFX3_UNSUPPORTED;

	result = fb_gfx3_gl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!fb_gfx3_gl_clip_rect(surface, &payload->clip, &clip)) {
		fb_gfx3_resource_release(state->resources, command->target);
		return FB_GFX3_OK;
	}
	if (((payload->x1 < clip.x1) && (payload->x2 < clip.x1)) ||
	    ((payload->x1 > clip.x2) && (payload->x2 > clip.x2)) ||
	    ((payload->y1 < clip.y1) && (payload->y2 < clip.y1)) ||
	    ((payload->y1 > clip.y2) && (payload->y2 > clip.y2))) {
		fb_gfx3_resource_release(state->resources, command->target);
		return FB_GFX3_OK;
	}

	state->gl.use_program(state->line_program);
	state->gl.bind_image_texture(0, surface->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_4i(state->line_endpoints_location, payload->x1,
		payload->y1, payload->x2, payload->y2);
	state->gl.uniform_4i(state->line_clip_location, clip.x1, clip.y1,
		clip.x2, clip.y2);
	state->gl.uniform_1ui(state->line_count_location, count);
	state->gl.uniform_1ui(state->line_color_location, payload->color);
	state->gl.uniform_1ui(state->line_style_location,
		payload->style & 0xFFFFu);
	state->gl.uniform_1ui(state->line_mask_location,
		fb_gfx3_gl_color_mask(surface->depth));
	state->gl.uniform_1ui(state->line_flags_location, payload->flags);
	state->gl.dispatch_compute(groups, 1, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "line compute dispatch");
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

int fb_gfx3_gl_rectangle(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_RECTANGLE_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *surface;
	FB_GFX3_RECT clip;
	FB_GFX3_RECT filled_rect;
	FB_GFX3_RECT draw_rect;
	int64_t width64;
	int64_t height64;
	uint32_t width;
	uint32_t height;
	uint32_t count;
	uint32_t groups;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_RECTANGLE_COMMAND *)command->payload;
	if ((payload->x1 > payload->x2) || (payload->y1 > payload->y2))
		return FB_GFX3_INVALID;
	result = fb_gfx3_gl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!fb_gfx3_gl_clip_rect(surface, &payload->clip, &clip) ||
	    (payload->x2 < clip.x1) || (payload->y2 < clip.y1) ||
	    (payload->x1 > clip.x2) || (payload->y1 > clip.y2)) {
		fb_gfx3_resource_release(state->resources, command->target);
		return FB_GFX3_OK;
	}

	if (payload->filled != 0) {
		filled_rect.x1 = (payload->x1 < clip.x1) ? clip.x1 : payload->x1;
		filled_rect.y1 = (payload->y1 < clip.y1) ? clip.y1 : payload->y1;
		filled_rect.x2 = (payload->x2 > clip.x2) ? clip.x2 : payload->x2;
		filled_rect.y2 = (payload->y2 > clip.y2) ? clip.y2 : payload->y2;
		/*
			A filled box has one write per pixel. The two-dimensional clear
			compute program therefore preserves primitive alpha semantics while
			avoiding the outline program's corner-write serialization path.
		*/
		result = fb_gfx3_gl_dispatch_clear(state, surface, &filled_rect,
			payload->color, payload->flags);
		fb_gfx3_resource_release(state->resources, command->target);
		return result;
	} else {
		draw_rect.x1 = payload->x1;
		draw_rect.y1 = payload->y1;
		draw_rect.x2 = payload->x2;
		draw_rect.y2 = payload->y2;
	}

	width64 = (int64_t)draw_rect.x2 - draw_rect.x1 + 1;
	height64 = (int64_t)draw_rect.y2 - draw_rect.y1 + 1;
	if ((width64 > 32767) || (height64 > 32767)) {
		fb_gfx3_resource_release(state->resources, command->target);
		return FB_GFX3_UNSUPPORTED;
	}
	width = (uint32_t)width64;
	height = (uint32_t)height64;
	if (payload->filled != 0) {
		if ((height != 0) && (width > UINT32_MAX / height)) {
			fb_gfx3_resource_release(state->resources, command->target);
			return FB_GFX3_UNSUPPORTED;
		}
		count = width * height;
	} else {
		count = (width + height) * 2;
	}
	groups = (count + FB_GFX3_OPENGL_POINT_GROUP_SIZE - 1) /
		FB_GFX3_OPENGL_POINT_GROUP_SIZE;
	if (groups > state->maximum_compute_groups_x) {
		fb_gfx3_resource_release(state->resources, command->target);
		return FB_GFX3_UNSUPPORTED;
	}

	state->gl.use_program(state->rectangle_program);
	state->gl.bind_image_texture(0, surface->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_4i(state->rectangle_box_location, draw_rect.x1,
		draw_rect.y1, draw_rect.x2, draw_rect.y2);
	state->gl.uniform_4i(state->rectangle_clip_location, clip.x1, clip.y1,
		clip.x2, clip.y2);
	state->gl.uniform_1ui(state->rectangle_width_location, width);
	state->gl.uniform_1ui(state->rectangle_height_location, height);
	state->gl.uniform_1ui(state->rectangle_count_location, count);
	state->gl.uniform_1ui(state->rectangle_color_location, payload->color);
	state->gl.uniform_1ui(state->rectangle_style_location,
		payload->style & 0xFFFFu);
	state->gl.uniform_1ui(state->rectangle_mask_location,
		fb_gfx3_gl_color_mask(surface->depth));
	state->gl.uniform_1ui(state->rectangle_filled_location,
		payload->filled != 0);
	state->gl.uniform_1ui(state->rectangle_flags_location, payload->flags);
	state->gl.dispatch_compute(groups, 1, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "rectangle compute dispatch");
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

int fb_gfx3_gl_ellipse(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_ELLIPSE_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *surface;
	FB_GFX3_RECT clip;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_ELLIPSE_COMMAND *)command->payload;
	if (!(payload->radius_x >= 0.0f) ||
	    !(payload->radius_x <= 32767.0f) ||
	    !(payload->radius_y >= 0.0f) ||
	    !(payload->radius_y <= 32767.0f))
		return FB_GFX3_INVALID;
	result = fb_gfx3_gl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!fb_gfx3_gl_clip_rect(surface, &payload->clip, &clip)) {
		fb_gfx3_resource_release(state->resources, command->target);
		return FB_GFX3_OK;
	}

	state->gl.use_program(state->ellipse_program);
	state->gl.bind_image_texture(0, surface->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_4i(state->ellipse_center_location, payload->center_x,
		payload->center_y, 0, 0);
	state->gl.uniform_4i(state->ellipse_clip_location, clip.x1, clip.y1,
		clip.x2, clip.y2);
	state->gl.uniform_2f(state->ellipse_radii_location, payload->radius_x,
		payload->radius_y);
	state->gl.uniform_1ui(state->ellipse_color_location, payload->color);
	state->gl.uniform_1ui(state->ellipse_filled_location,
		payload->filled != 0);
	state->gl.uniform_1ui(state->ellipse_mask_location,
		fb_gfx3_gl_color_mask(surface->depth));
	state->gl.uniform_1ui(state->ellipse_flags_location, payload->flags);
	state->gl.dispatch_compute(1, 1, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "ellipse compute dispatch");
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

/*
	The scanline queue is cleared by the paint shader itself.  Retain its GPU
	allocation across fills because deleting a buffer while the preceding PAINT
	is still in flight can make otherwise asynchronous command streams wait for
	the driver to retire it.
*/
static int opengl_paint_scratch_ensure(FB_GFX3_OPENGL_STATE *state,
	size_t required_bytes)
{
	if ((state == NULL) || (required_bytes == 0u) ||
	    (required_bytes > state->maximum_storage_buffer_size) ||
	    (required_bytes > (size_t)PTRDIFF_MAX))
		return FB_GFX3_INVALID;
	if ((state->paint_scratch_buffer != 0u) &&
	    (state->paint_scratch_bytes >= required_bytes))
		return FB_GFX3_OK;
	if (state->paint_scratch_buffer == 0u) {
		state->gl.generate_buffers(1, &state->paint_scratch_buffer);
		if (state->paint_scratch_buffer == 0u)
			return FB_GFX3_OUT_OF_MEMORY;
	}
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER,
		state->paint_scratch_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)required_bytes, NULL, GL_DYNAMIC_DRAW);
	if (fb_gfx3_gl_check_error(state, "paint scratch allocation") != FB_GFX3_OK)
		return FB_GFX3_OUT_OF_MEMORY;
	state->paint_scratch_bytes = required_bytes;
	return FB_GFX3_OK;
}

int fb_gfx3_gl_paint(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_PAINT_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *surface;
	FB_GFX3_RECT clip;
	size_t pixel_count;
	size_t scratch_words;
	size_t scratch_bytes;
	size_t clip_pixel_count;
	uint32_t clip_height;
	uint32_t clip_width;
	uint32_t rectangle_groups_x;
	uint32_t rectangle_groups_y;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_PAINT_COMMAND *)command->payload;
	result = fb_gfx3_gl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!fb_gfx3_gl_clip_rect(surface, &payload->clip, &clip) ||
	    (payload->x < clip.x1) || (payload->y < clip.y1) ||
	    (payload->x > clip.x2) || (payload->y > clip.y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	clip_width = (uint32_t)((int64_t)clip.x2 - clip.x1 + 1);
	clip_height = (uint32_t)((int64_t)clip.y2 - clip.y1 + 1);
	if ((fb_gfx3_size_multiply(surface->width, surface->height,
	     &pixel_count) != FB_GFX3_OK) ||
	    (pixel_count > FB_GFX3_OPENGL_PAINT_MAX_PIXELS) ||
	    (fb_gfx3_size_multiply(pixel_count, 2u, &scratch_words) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_size_add(scratch_words, 5u, &scratch_words) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(scratch_words, sizeof(uint32_t),
	     &scratch_bytes) != FB_GFX3_OK) ||
	    (scratch_bytes > state->maximum_storage_buffer_size) ||
	    (fb_gfx3_size_multiply(clip_width, clip_height,
	     &clip_pixel_count) != FB_GFX3_OK) ||
	    (clip_pixel_count == 0u)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	rectangle_groups_x = (clip_width +
		FB_GFX3_OPENGL_LOCAL_SIZE_X - 1u) /
		FB_GFX3_OPENGL_LOCAL_SIZE_X;
	rectangle_groups_y = (clip_height +
		FB_GFX3_OPENGL_LOCAL_SIZE_Y - 1u) /
		FB_GFX3_OPENGL_LOCAL_SIZE_Y;
	result = opengl_paint_scratch_ensure(state, scratch_bytes);
	if (result != FB_GFX3_OK)
		goto done;
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER,
		state->paint_scratch_buffer);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 1,
		state->paint_scratch_buffer);
	state->gl.use_program(state->paint_program);
	state->gl.bind_image_texture(0, surface->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_4i(state->paint_seed_location, payload->x, payload->y,
		0, 0);
	state->gl.uniform_4i(state->paint_clip_location, clip.x1, clip.y1,
		clip.x2, clip.y2);
	state->gl.uniform_1ui(state->paint_color_location, payload->color);
	state->gl.uniform_1ui(state->paint_border_location, payload->border_color);
	state->gl.uniform_1ui(state->paint_mask_location,
		fb_gfx3_gl_color_mask(surface->depth));
	state->gl.uniform_1ui(state->paint_flags_location, payload->flags);
	state->gl.uniform_1ui(state->paint_mode_location, payload->paint_mode);
	state->gl.uniform_1ui(state->paint_pattern_size_location,
		payload->pattern_size);
	state->gl.uniform_4i(state->paint_pattern_origin_location,
		(int)payload->pattern_origin_x, (int)payload->pattern_origin_y, 0, 0);
	state->gl.uniform_1ui(state->paint_bytes_per_pixel_location,
		fb_gfx3_gl_bytes_per_pixel(surface->depth, NULL));
	state->gl.uniform_1uiv(state->paint_pattern_location, 64,
		payload->pattern_word);
	/*
		The first dispatch discovers candidate bounds. The second verifies every
		interior and adjacent-perimeter pixel across the GPU. The third writes a
		verified rectangle, and the fourth runs the exact scanline fallback only
		when verification rejected it. Storage barriers are global dispatch
		boundaries; a workgroup barrier cannot synchronize the whole device.
	*/
	state->gl.uniform_1ui(state->paint_phase_location, 1u);
	state->gl.dispatch_compute(1, 1, 1);
	state->gl.memory_barrier(GL_SHADER_STORAGE_BARRIER_BIT);
	state->gl.uniform_1ui(state->paint_phase_location, 2u);
	state->gl.dispatch_compute(rectangle_groups_x, rectangle_groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_STORAGE_BARRIER_BIT);
	state->gl.uniform_1ui(state->paint_phase_location, 3u);
	state->gl.dispatch_compute(rectangle_groups_x, rectangle_groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT);
	state->gl.uniform_1ui(state->paint_phase_location, 4u);
	state->gl.dispatch_compute(1, 1, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "multi-dispatch paint compute");

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

#endif

/* end of gfx3_opengl_draw.c */
