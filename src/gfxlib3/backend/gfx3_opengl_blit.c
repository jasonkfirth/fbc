/*
 * FreeBASIC gfxlib3: gfx3_opengl_blit.c
 * Purpose: Dispatch OpenGL image copies and transformed blits.
 * Responsibilities: render-thread operations within this module.
 * This file intentionally does NOT contain BASIC compatibility or native input dispatch.
 */

#include "gfx3_opengl_private.h"

#if (defined(HOST_WIN32) || defined(HOST_LINUX)) && !defined(DISABLE_OPENGL)

int fb_gfx3_gl_blit(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_BLIT_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *destination;
	FB_GFX3_OPENGL_SURFACE *source;
	FB_GFX3_RECT clip;
	int64_t destination_x2;
	int64_t destination_y2;
	uint32_t width;
	uint32_t height;
	uint32_t groups_x;
	uint32_t groups_y;
	GLuint source_texture;
	GLuint temporary_texture = 0;
	int source_x;
	int source_y;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_BLIT_COMMAND *)command->payload;
	switch (payload->mode) {
	case FB_GFX3_BLIT_TRANS:
	case FB_GFX3_BLIT_PSET:
	case FB_GFX3_BLIT_PRESET:
	case FB_GFX3_BLIT_AND:
	case FB_GFX3_BLIT_OR:
	case FB_GFX3_BLIT_XOR:
	case FB_GFX3_BLIT_ALPHA:
	case FB_GFX3_BLIT_ADD:
	case FB_GFX3_BLIT_BLEND:
		break;
	default:
		return FB_GFX3_UNSUPPORTED;
	}

	result = fb_gfx3_gl_surface_retain(state, command->target,
		command->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_gl_surface_retain(state, payload->source,
		command->sequence, &source);
	if (result != FB_GFX3_OK) {
		fb_gfx3_resource_release(state->resources, command->target);
		return result;
	}
	if ((source->depth != destination->depth) ||
	    (payload->source_rect.x1 < 0) || (payload->source_rect.y1 < 0) ||
	    (payload->source_rect.x1 > payload->source_rect.x2) ||
	    (payload->source_rect.y1 > payload->source_rect.y2) ||
	    (payload->source_rect.x2 >= (int32_t)source->width) ||
	    (payload->source_rect.y2 >= (int32_t)source->height)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	if (!fb_gfx3_gl_clip_rect(destination, &payload->clip, &clip)) {
		result = FB_GFX3_OK;
		goto done;
	}

	width = (uint32_t)(payload->source_rect.x2 -
		payload->source_rect.x1 + 1);
	height = (uint32_t)(payload->source_rect.y2 -
		payload->source_rect.y1 + 1);
	destination_x2 = (int64_t)payload->destination_x + width - 1;
	destination_y2 = (int64_t)payload->destination_y + height - 1;
	if ((destination_x2 < clip.x1) || (destination_y2 < clip.y1) ||
	    (payload->destination_x > clip.x2) ||
	    (payload->destination_y > clip.y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	/*
		A complete same-depth PSET copy between different surfaces has no
		per-pixel decision to make. glCopyImageSubData keeps this common
		SCREENCOPY operation entirely in GPU memory and avoids launching the
		general compute shader once for every page flip. Restrict the shortcut
		to a fully visible rectangle and matching logical formats: clipping,
		self-copy overlap, depth conversion, and every non-PSET transfer mode retain the exact
		compute implementation below.
	*/
	if ((payload->mode == FB_GFX3_BLIT_PSET) &&
	    (source != destination) && (source->depth == destination->depth) &&
	    (payload->destination_x >= clip.x1) &&
	    (payload->destination_y >= clip.y1) &&
	    (destination_x2 <= clip.x2) && (destination_y2 <= clip.y2)) {
		state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
			GL_TEXTURE_UPDATE_BARRIER_BIT);
		state->gl.copy_image_sub_data(source->texture, GL_TEXTURE_2D, 0,
			payload->source_rect.x1, payload->source_rect.y1, 0,
			destination->texture, GL_TEXTURE_2D, 0,
			payload->destination_x, payload->destination_y, 0,
			(GLsizei)width, (GLsizei)height, 1);
		state->gl.memory_barrier(GL_TEXTURE_UPDATE_BARRIER_BIT |
			GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
			GL_TEXTURE_FETCH_BARRIER_BIT);
		result = fb_gfx3_gl_check_error(state, "surface PSET image copy");
		goto done;
	}
	groups_x = (width + FB_GFX3_OPENGL_LOCAL_SIZE_X - 1) /
		FB_GFX3_OPENGL_LOCAL_SIZE_X;
	groups_y = (height + FB_GFX3_OPENGL_LOCAL_SIZE_Y - 1) /
		FB_GFX3_OPENGL_LOCAL_SIZE_Y;
	if ((groups_x > state->maximum_compute_groups_x) ||
	    (groups_y > state->maximum_compute_groups_y)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	source_texture = source->texture;
	source_x = payload->source_rect.x1;
	source_y = payload->source_rect.y1;
	if (payload->source == command->target) {
		/*
			Compute invocations may execute in any order. Snapshot the source
			region on the GPU before an overlapping write can change it.
		*/
		state->gl.generate_textures(1, &temporary_texture);
		state->gl.bind_texture(GL_TEXTURE_2D, temporary_texture);
		state->gl.texture_storage_2d(GL_TEXTURE_2D, 1, GL_R32UI,
			(GLsizei)width, (GLsizei)height);
		result = fb_gfx3_gl_check_error(state,
			"overlapping blit temporary allocation");
		if (result != FB_GFX3_OK)
			goto done;
		state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
			GL_TEXTURE_UPDATE_BARRIER_BIT);
		state->gl.copy_image_sub_data(source->texture, GL_TEXTURE_2D, 0,
			payload->source_rect.x1, payload->source_rect.y1, 0,
			temporary_texture, GL_TEXTURE_2D, 0, 0, 0, 0,
			(GLsizei)width, (GLsizei)height, 1);
		state->gl.memory_barrier(GL_TEXTURE_UPDATE_BARRIER_BIT |
			GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		result = fb_gfx3_gl_check_error(state, "overlapping surface blit copy");
		if (result != FB_GFX3_OK)
			goto done;
		source_texture = temporary_texture;
		source_x = 0;
		source_y = 0;
	}

	state->gl.use_program(state->blit_program);
	state->gl.bind_image_texture(0, destination->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.bind_image_texture(1, source_texture, 0, GL_FALSE, 0,
		GL_READ_ONLY, GL_R32UI);
	state->gl.uniform_4i(state->blit_source_rect_location,
		source_x, source_y,
		(int)width, (int)height);
	state->gl.uniform_4i(state->blit_clip_location, clip.x1, clip.y1,
		clip.x2, clip.y2);
	state->gl.uniform_1i(state->blit_destination_x_location,
		payload->destination_x);
	state->gl.uniform_1i(state->blit_destination_y_location,
		payload->destination_y);
	state->gl.uniform_1ui(state->blit_mode_location, payload->mode);
	state->gl.uniform_1ui(state->blit_alpha_location, payload->alpha);
	state->gl.uniform_1ui(state->blit_depth_location, destination->depth);
	state->gl.uniform_1ui(state->blit_mask_location,
		fb_gfx3_gl_color_mask(destination->depth));
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "surface blit compute dispatch");

done:
	if (temporary_texture != 0)
		state->gl.delete_textures(1, &temporary_texture);
	fb_gfx3_resource_release(state->resources, payload->source);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

/*
	Transform blit

	The BASIC thread supplies only an inverse mapping matrix and a conservative
	destination box. The compute shader maps and blends every destination pixel,
	so scaling, rotation, and projective Mode 7 do not generate CPU-side spans or
	transformed image copies. A self-transform takes one GPU snapshot because
	compute workgroups have no defined execution order relative to each other.
*/
int fb_gfx3_gl_transform_blit(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_TRANSFORM_BLIT_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	FB_GFX3_OPENGL_SURFACE *source = NULL;
	FB_GFX3_RECT clip;
	FB_GFX3_RECT bounds;
	GLuint source_texture;
	GLuint temporary_texture = 0;
	uint32_t width;
	uint32_t height;
	uint32_t groups_x;
	uint32_t groups_y;
	int result;

	if ((state == NULL) || (command == NULL) ||
	    (fb_gfx3_command_payload_size(command) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_TRANSFORM_BLIT_COMMAND *)command->payload;
	switch (payload->mode) {
	case FB_GFX3_BLIT_TRANS:
	case FB_GFX3_BLIT_PSET:
	case FB_GFX3_BLIT_PRESET:
	case FB_GFX3_BLIT_AND:
	case FB_GFX3_BLIT_OR:
	case FB_GFX3_BLIT_XOR:
	case FB_GFX3_BLIT_ALPHA:
	case FB_GFX3_BLIT_ADD:
	case FB_GFX3_BLIT_BLEND:
		break;
	default:
		return FB_GFX3_UNSUPPORTED;
	}
	if ((payload->filter > FB_GFX3_TRANSFORM_FILTER_LINEAR) ||
	    (payload->wrap > FB_GFX3_TRANSFORM_WRAP_REPEAT))
		return FB_GFX3_INVALID;
	result = fb_gfx3_gl_surface_retain(state, command->target, command->sequence,
		&destination);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_gl_surface_retain(state, payload->source, command->sequence,
		&source);
	if (result != FB_GFX3_OK)
		goto done;
	if ((source->depth != destination->depth) ||
	    (payload->source_rect.x1 < 0) || (payload->source_rect.y1 < 0) ||
	    (payload->source_rect.x1 > payload->source_rect.x2) ||
	    (payload->source_rect.y1 > payload->source_rect.y2) ||
	    (payload->source_rect.x2 >= (int32_t)source->width) ||
	    (payload->source_rect.y2 >= (int32_t)source->height) ||
	    (payload->destination_bounds.x1 > payload->destination_bounds.x2) ||
	    (payload->destination_bounds.y1 > payload->destination_bounds.y2)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	if (!fb_gfx3_gl_clip_rect(destination, &payload->clip, &clip)) {
		result = FB_GFX3_OK;
		goto done;
	}
	bounds = payload->destination_bounds;
	if (bounds.x1 < clip.x1)
		bounds.x1 = clip.x1;
	if (bounds.y1 < clip.y1)
		bounds.y1 = clip.y1;
	if (bounds.x2 > clip.x2)
		bounds.x2 = clip.x2;
	if (bounds.y2 > clip.y2)
		bounds.y2 = clip.y2;
	if ((bounds.x1 > bounds.x2) || (bounds.y1 > bounds.y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	width = (uint32_t)(bounds.x2 - bounds.x1 + 1);
	height = (uint32_t)(bounds.y2 - bounds.y1 + 1);
	groups_x = (width + FB_GFX3_OPENGL_LOCAL_SIZE_X - 1u) /
		FB_GFX3_OPENGL_LOCAL_SIZE_X;
	groups_y = (height + FB_GFX3_OPENGL_LOCAL_SIZE_Y - 1u) /
		FB_GFX3_OPENGL_LOCAL_SIZE_Y;
	if ((groups_x > state->maximum_compute_groups_x) ||
	    (groups_y > state->maximum_compute_groups_y)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}

	source_texture = source->texture;
	if (payload->source == command->target) {
		state->gl.generate_textures(1, &temporary_texture);
		state->gl.bind_texture(GL_TEXTURE_2D, temporary_texture);
		state->gl.texture_storage_2d(GL_TEXTURE_2D, 1, GL_R32UI,
			(GLsizei)source->width, (GLsizei)source->height);
		result = fb_gfx3_gl_check_error(state,
			"self-transform temporary allocation");
		if (result != FB_GFX3_OK)
			goto done;
		state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
			GL_TEXTURE_UPDATE_BARRIER_BIT);
		state->gl.copy_image_sub_data(source->texture, GL_TEXTURE_2D, 0,
			0, 0, 0, temporary_texture, GL_TEXTURE_2D, 0, 0, 0, 0,
			(GLsizei)source->width, (GLsizei)source->height, 1);
		state->gl.memory_barrier(GL_TEXTURE_UPDATE_BARRIER_BIT |
			GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		result = fb_gfx3_gl_check_error(state, "self-transform surface copy");
		if (result != FB_GFX3_OK)
			goto done;
		source_texture = temporary_texture;
	}

	state->gl.use_program(state->transform_blit_program);
	state->gl.bind_image_texture(0, destination->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.bind_image_texture(1, source_texture, 0, GL_FALSE, 0,
		GL_READ_ONLY, GL_R32UI);
	state->gl.uniform_4i(state->transform_blit_source_rect_location,
		payload->source_rect.x1, payload->source_rect.y1,
		payload->source_rect.x2, payload->source_rect.y2);
	state->gl.uniform_4i(state->transform_blit_clip_location,
		clip.x1, clip.y1, clip.x2, clip.y2);
	state->gl.uniform_4i(state->transform_blit_bounds_location,
		bounds.x1, bounds.y1, bounds.x2, bounds.y2);
	/* The public packet stores rows for C callers; OpenGL stores matrix columns. */
	state->gl.uniform_matrix_3fv(state->transform_blit_inverse_location, 1,
		GL_TRUE, payload->inverse);
	state->gl.uniform_1ui(state->transform_blit_mode_location, payload->mode);
	state->gl.uniform_1ui(state->transform_blit_alpha_location, payload->alpha);
	state->gl.uniform_1ui(state->transform_blit_depth_location,
		destination->depth);
	state->gl.uniform_1ui(state->transform_blit_mask_location,
		fb_gfx3_gl_color_mask(destination->depth));
	state->gl.uniform_1ui(state->transform_blit_filter_location,
		payload->filter);
	state->gl.uniform_1ui(state->transform_blit_wrap_location, payload->wrap);
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "transform blit compute dispatch");

done:
	if (temporary_texture != 0)
		state->gl.delete_textures(1, &temporary_texture);
	if (source != NULL)
		fb_gfx3_resource_release(state->resources, payload->source);
	if (destination != NULL)
		fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

int fb_gfx3_gl_read_pixel(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_READ_PIXEL_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *surface;
	uint32_t color = UINT32_MAX;
	int result;

	if ((command->completion == NULL) ||
	    (fb_gfx3_command_payload_size(command) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_READ_PIXEL_COMMAND *)command->payload;
	result = fb_gfx3_gl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if ((payload->x >= 0) && (payload->y >= 0) &&
	    (payload->x < (int32_t)surface->width) &&
	    (payload->y < (int32_t)surface->height)) {
		state->gl.memory_barrier(GL_FRAMEBUFFER_BARRIER_BIT |
			GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		result = fb_gfx3_gl_bind_color_framebuffer(state, surface->texture);
		if (result != FB_GFX3_OK)
			goto done;
		state->gl.read_buffer(GL_COLOR_ATTACHMENT0);
		state->gl.read_pixels(payload->x, payload->y, 1, 1,
			GL_RED_INTEGER, GL_UNSIGNED_INT, &color);
		result = fb_gfx3_gl_check_error(state, "single-pixel readback");
		if (result != FB_GFX3_OK)
			goto done;
	}
	result = fb_gfx3_completion_set_value(command->completion, 0, color);

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

#endif

/* end of gfx3_opengl_blit.c */
