/*
 * FreeBASIC gfxlib3: gfx3_opengl_surface.c
 * Purpose: Own OpenGL surface storage, transfers and presentation.
 * Responsibilities: render-thread operations within this module.
 * This file intentionally does NOT contain BASIC compatibility or native input dispatch.
 */

#include "gfx3_opengl_private.h"

#if (defined(HOST_WIN32) || defined(HOST_LINUX)) && !defined(DISABLE_OPENGL)

/* ------------------------------------------------------------------------- */
/* GPU surface storage and commands                                          */
/* ------------------------------------------------------------------------- */

uint32_t fb_gfx3_gl_color_mask(uint32_t depth)
{
	if (depth >= 32)
		return UINT32_MAX;
	return (1u << depth) - 1u;
}

uint32_t fb_gfx3_gl_bytes_per_pixel(uint32_t depth, GLenum *type)
{
	switch (depth) {
	case 1:
	case 2:
	case 4:
	case 8:
		if (type != NULL)
			*type = GL_UNSIGNED_BYTE;
		return 1;
	case 16:
		if (type != NULL)
			*type = GL_UNSIGNED_SHORT;
		return 2;
	case 32:
		if (type != NULL)
			*type = GL_UNSIGNED_INT;
		return 4;
	default:
		return 0;
	}
}

int fb_gfx3_gl_check_error(FB_GFX3_OPENGL_STATE *state,
	const char *operation)
{
	GLenum error;

	if ((state == NULL) || (operation == NULL) ||
	    (state->gl.get_error == NULL))
		return FB_GFX3_INVALID;
	if (state->runtime_ready && !state->validate_runtime_errors)
		return FB_GFX3_OK;
	error = state->gl.get_error();
	if (error == GL_NO_ERROR)
		return FB_GFX3_OK;
	fb_gfx3_log_write(state->logger, FB_GFX3_LOG_ERROR,
		"OpenGL error 0x%04X during %s", (unsigned int)error, operation);
	while (state->gl.get_error() != GL_NO_ERROR) {
		/* Drain the context error queue so a later check has clear meaning. */
	}
	return FB_GFX3_FAILED;
}

int fb_gfx3_gl_bind_color_framebuffer(FB_GFX3_OPENGL_STATE *state,
	GLuint texture)
{
	if ((state == NULL) || (texture == 0u) ||
	    (state->read_framebuffer == 0u))
		return FB_GFX3_INVALID;
	state->gl.bind_framebuffer(GL_FRAMEBUFFER, state->read_framebuffer);
	if (state->framebuffer_color_texture == texture)
		return FB_GFX3_OK;
	state->gl.framebuffer_texture_2d(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, texture, 0);
	if (state->gl.check_framebuffer_status(GL_FRAMEBUFFER) !=
	    GL_FRAMEBUFFER_COMPLETE) {
		state->framebuffer_color_texture = 0u;
		return FB_GFX3_FAILED;
	}
	state->framebuffer_color_texture = texture;
	return FB_GFX3_OK;
}

int fb_gfx3_gl_clip_rect(const FB_GFX3_OPENGL_SURFACE *surface,
	const FB_GFX3_RECT *requested, FB_GFX3_RECT *clipped)
{
	if ((surface == NULL) || (requested == NULL) || (clipped == NULL))
		return FALSE;
	*clipped = *requested;
	if (clipped->x1 < 0)
		clipped->x1 = 0;
	if (clipped->y1 < 0)
		clipped->y1 = 0;
	if (clipped->x2 >= (int32_t)surface->width)
		clipped->x2 = (int32_t)surface->width - 1;
	if (clipped->y2 >= (int32_t)surface->height)
		clipped->y2 = (int32_t)surface->height - 1;
	return (clipped->x1 <= clipped->x2) &&
		(clipped->y1 <= clipped->y2);
}

static void opengl_surface_destroy(void *resource)
{
	FB_GFX3_OPENGL_SURFACE *surface =
		(FB_GFX3_OPENGL_SURFACE *)resource;

	if (surface == NULL)
		return;
	if ((surface->texture != 0) && (surface->state != NULL)) {
		if (surface->state->framebuffer_color_texture == surface->texture)
			surface->state->framebuffer_color_texture = 0u;
		surface->state->gl.delete_textures(1, &surface->texture);
	}
	free(surface);
}

int fb_gfx3_gl_surface_retain(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_HANDLE handle, uint64_t sequence,
	FB_GFX3_OPENGL_SURFACE **surface)
{
	int result;

	result = fb_gfx3_resource_retain(state->resources, handle,
		FB_GFX3_RESOURCE_SURFACE, (void **)surface);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_resource_mark_used(state->resources, handle, sequence);
	if (result != FB_GFX3_OK) {
		fb_gfx3_resource_release(state->resources, handle);
		*surface = NULL;
	}
	return result;
}

int fb_gfx3_gl_palette(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_PALETTE_COMMAND *payload;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_PALETTE_COMMAND *)command->payload;
	memcpy(state->palette, payload->color, sizeof(state->palette));
	if (state->visible_surface != 0)
		state->presentation_dirty = TRUE;
	return FB_GFX3_OK;
}

int fb_gfx3_gl_page_set(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_PAGE_SET_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *surface;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_PAGE_SET_COMMAND *)command->payload;
	result = fb_gfx3_gl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if ((payload->width != surface->width) ||
	    (payload->height != surface->height) ||
	    (payload->depth != surface->depth)) {
		result = FB_GFX3_INVALID;
	} else {
		state->visible_surface = command->target;
		state->presentation_dirty = TRUE;
	}
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

int fb_gfx3_gl_present_handle(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_HANDLE handle, uint64_t sequence)
{
	FB_GFX3_PRESENTATION_LAYOUT layout;
	FB_GFX3_OPENGL_SURFACE *surface;
	uint32_t width;
	uint32_t height;
	int viewport_y;
	int result;

	result = fb_gfx3_gl_surface_retain(state, handle, sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	result = state->platform_vtable->client_size(state->platform, &width,
		&height);
	if (result != FB_GFX3_OK)
		goto done;
	if ((width == 0) || (height == 0)) {
		result = FB_GFX3_OK;
		goto done;
	}
	result = fb_gfx3_platform_presentation_layout(surface->width,
		surface->height, width, height, &layout);
	if (result != FB_GFX3_OK)
		goto done;
	viewport_y = (int)height - layout.y - (int)layout.height;
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
	state->gl.bind_framebuffer(GL_FRAMEBUFFER, 0);
	if ((layout.x != 0) || (layout.y != 0) ||
	    (layout.width != width) || (layout.height != height)) {
		/*
			The bars belong only to the native back buffer. The logical page
			stays untouched, so POINT and direct-memory access never observe
			presentation padding.
		*/
		state->gl.clear_color(0.0f, 0.0f, 0.0f, 1.0f);
		state->gl.clear(GL_COLOR_BUFFER_BIT);
	}
	state->gl.viewport(layout.x, viewport_y, (GLsizei)layout.width,
		(GLsizei)layout.height);
	state->gl.use_program(state->present_program);
	state->gl.bind_texture(GL_TEXTURE_2D, surface->texture);
	state->gl.uniform_1i(state->present_source_location, 0);
	state->gl.uniform_1ui(state->present_depth_location, surface->depth);
	state->gl.uniform_1uiv(state->present_palette_location, 256,
		state->palette);
	state->gl.bind_vertex_array(state->present_vertex_array);
	state->gl.draw_arrays(GL_TRIANGLES, 0, 3);
	result = fb_gfx3_gl_check_error(state, "presentation draw");
	if (result == FB_GFX3_OK)
		result = state->platform_vtable->swap_buffers(state->platform);
	if (result == FB_GFX3_OK)
		result = state->platform_vtable->show_window(state->platform);
	if (result == FB_GFX3_OK)
		state->presentation_dirty = FALSE;
	state->client_width = width;
	state->client_height = height;

done:
	fb_gfx3_resource_release(state->resources, handle);
	state->platform_vtable->pump_events(state->platform);
	return result;
}

int fb_gfx3_gl_present(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	if (fb_gfx3_command_payload_size(command) != 0)
		return FB_GFX3_INVALID;
	return fb_gfx3_gl_present_handle(state, command->target,
		command->sequence);
}

int fb_gfx3_gl_window_title(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_WINDOW_TITLE_COMMAND *payload;
	size_t payload_size = fb_gfx3_command_payload_size(command);
	size_t expected_size;

	if (payload_size < sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_WINDOW_TITLE_COMMAND *)command->payload;
	if ((fb_gfx3_size_add(sizeof(*payload), payload->length,
	     &expected_size) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(expected_size, 1u, &expected_size) !=
	     FB_GFX3_OK) || (payload_size != expected_size) ||
	    (payload->title[payload->length] != '\0'))
		return FB_GFX3_INVALID;
	return state->platform_vtable->set_window_title(state->platform,
		payload->title);
}

int fb_gfx3_gl_dispatch_clear(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_OPENGL_SURFACE *surface, const FB_GFX3_RECT *rect,
	uint32_t color, uint32_t flags)
{
	uint32_t width = (uint32_t)(rect->x2 - rect->x1 + 1);
	uint32_t height = (uint32_t)(rect->y2 - rect->y1 + 1);
	uint32_t groups_x = (width + FB_GFX3_OPENGL_LOCAL_SIZE_X - 1) /
		FB_GFX3_OPENGL_LOCAL_SIZE_X;
	uint32_t groups_y = (height + FB_GFX3_OPENGL_LOCAL_SIZE_Y - 1) /
		FB_GFX3_OPENGL_LOCAL_SIZE_Y;

	if ((groups_x > state->maximum_compute_groups_x) ||
	    (groups_y > state->maximum_compute_groups_y))
		return FB_GFX3_UNSUPPORTED;
	/*
		POINT readback leaves the reusable read framebuffer attached to its
		source texture. Detach it before binding any texture as a writable image;
		some drivers otherwise retain a readback hazard across the next dispatch.
	*/
	state->gl.bind_framebuffer(GL_READ_FRAMEBUFFER, 0);
	state->gl.use_program(state->clear_program);
	state->gl.bind_image_texture(0, surface->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_4i(state->clear_rect_location, rect->x1, rect->y1,
		(int)width, (int)height);
	state->gl.uniform_1ui(state->clear_color_location, color);
	state->gl.uniform_1ui(state->clear_mask_location,
		fb_gfx3_gl_color_mask(surface->depth));
	state->gl.uniform_1ui(state->clear_flags_location,
		flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND);
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	return fb_gfx3_gl_check_error(state, "clear compute dispatch");
}

int fb_gfx3_gl_surface_create(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_SURFACE_CREATE_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *surface;
	FB_GFX3_RECT full_rect;
	FB_GFX3_HANDLE handle;
	int result;

	if ((command->completion == NULL) ||
	    (fb_gfx3_command_payload_size(command) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_SURFACE_CREATE_COMMAND *)command->payload;
	if ((payload->width == 0) || (payload->height == 0) ||
	    (payload->width > state->maximum_compute_groups_x *
		FB_GFX3_OPENGL_LOCAL_SIZE_X) ||
	    (payload->height > state->maximum_compute_groups_y *
		FB_GFX3_OPENGL_LOCAL_SIZE_Y))
		return FB_GFX3_INVALID;
	switch (payload->depth) {
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
	case 32:
		break;
	default:
		return FB_GFX3_INVALID;
	}

	surface = (FB_GFX3_OPENGL_SURFACE *)calloc(1, sizeof(*surface));
	if (surface == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	surface->state = state;
	surface->width = payload->width;
	surface->height = payload->height;
	surface->depth = payload->depth;
	state->gl.generate_textures(1, &surface->texture);
	state->gl.bind_texture(GL_TEXTURE_2D, surface->texture);
	state->gl.texture_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_NEAREST);
	state->gl.texture_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		GL_NEAREST);
	state->gl.texture_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
		GL_CLAMP_TO_EDGE);
	state->gl.texture_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
		GL_CLAMP_TO_EDGE);
	state->gl.texture_storage_2d(GL_TEXTURE_2D, 1, GL_R32UI,
		(GLsizei)surface->width, (GLsizei)surface->height);
	result = fb_gfx3_gl_check_error(state, "surface texture allocation");
	if (result != FB_GFX3_OK) {
		opengl_surface_destroy(surface);
		return result;
	}

	full_rect.x1 = 0;
	full_rect.y1 = 0;
	full_rect.x2 = (int32_t)surface->width - 1;
	full_rect.y2 = (int32_t)surface->height - 1;
	result = fb_gfx3_gl_dispatch_clear(state, surface, &full_rect,
		payload->clear_color, 0);
	if (result != FB_GFX3_OK) {
		opengl_surface_destroy(surface);
		return result;
	}

	handle = fb_gfx3_resource_register(state->resources,
		FB_GFX3_RESOURCE_SURFACE, surface, opengl_surface_destroy);
	if (handle == 0) {
		opengl_surface_destroy(surface);
		return FB_GFX3_OUT_OF_MEMORY;
	}
	result = fb_gfx3_completion_set_value(command->completion, 0, handle);
	if (result != FB_GFX3_OK) {
		fb_gfx3_resource_release(state->resources, handle);
		fb_gfx3_resources_collect(state->resources, UINT64_MAX);
	}
	return result;
}

int fb_gfx3_gl_surface_release(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	int result;

	if (fb_gfx3_command_payload_size(command) != 0)
		return FB_GFX3_INVALID;
	result = fb_gfx3_resource_mark_used(state->resources, command->target,
		command->sequence);
	if (result != FB_GFX3_OK)
		return result;
	return fb_gfx3_resource_release(state->resources, command->target);
}

int fb_gfx3_gl_surface_upload(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_SURFACE_UPLOAD_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *surface;
	size_t header_size = offsetof(FB_GFX3_SURFACE_UPLOAD_COMMAND, data);
	size_t expected_data_size;
	size_t row_size;
	uint32_t bytes_per_pixel;
	uint32_t y;
	GLenum type = GL_UNSIGNED_INT;
	int result;

	if (fb_gfx3_command_payload_size(command) < header_size)
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_SURFACE_UPLOAD_COMMAND *)command->payload;
	if ((payload->width == 0) || (payload->height == 0) ||
	    (payload->destination_x < 0) || (payload->destination_y < 0) ||
	    (payload->data_size !=
		fb_gfx3_command_payload_size(command) - header_size))
		return FB_GFX3_INVALID;
	result = fb_gfx3_gl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	bytes_per_pixel = fb_gfx3_gl_bytes_per_pixel(surface->depth, &type);
	if (((uint64_t)(uint32_t)payload->destination_x + payload->width >
	     surface->width) ||
	    ((uint64_t)(uint32_t)payload->destination_y + payload->height >
	     surface->height) ||
	    (bytes_per_pixel == 0) ||
	    (fb_gfx3_size_multiply(payload->width, bytes_per_pixel,
	     &row_size) != FB_GFX3_OK) ||
	    (payload->source_pitch < row_size) ||
	    (fb_gfx3_size_multiply(payload->source_pitch, payload->height,
	     &expected_data_size) != FB_GFX3_OK) ||
	    (expected_data_size != payload->data_size)) {
		result = FB_GFX3_INVALID;
		goto done;
	}

	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_UPDATE_BARRIER_BIT);
	state->gl.bind_texture(GL_TEXTURE_2D, surface->texture);
	state->gl.pixel_store_i(GL_UNPACK_ALIGNMENT, 1);
	for (y = 0; y < payload->height; y++) {
		state->gl.texture_sub_image_2d(GL_TEXTURE_2D, 0,
			payload->destination_x, payload->destination_y + (int32_t)y,
			(GLsizei)payload->width, 1, GL_RED_INTEGER, type,
			payload->data + ((size_t)y * payload->source_pitch));
	}
	state->gl.pixel_store_i(GL_UNPACK_ALIGNMENT, 4);
	state->gl.memory_barrier(GL_TEXTURE_UPDATE_BARRIER_BIT |
		GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "surface upload");

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

int fb_gfx3_gl_surface_download(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_SURFACE_DOWNLOAD_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *surface;
	unsigned char *destination;
	size_t expected_size;
	size_t row_size;
	uint32_t bytes_per_pixel;
	uint32_t y;
	GLenum type = GL_UNSIGNED_INT;
	int result;

	if ((command->completion == NULL) ||
	    (fb_gfx3_command_payload_size(command) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_SURFACE_DOWNLOAD_COMMAND *)command->payload;
	if ((payload->width == 0) || (payload->height == 0) ||
	    (payload->source_x < 0) || (payload->source_y < 0) ||
	    (payload->destination_address == 0) ||
	    (payload->destination_address > UINTPTR_MAX))
		return FB_GFX3_INVALID;
	result = fb_gfx3_gl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	bytes_per_pixel = fb_gfx3_gl_bytes_per_pixel(surface->depth, &type);
	if (((uint64_t)(uint32_t)payload->source_x + payload->width >
	     surface->width) ||
	    ((uint64_t)(uint32_t)payload->source_y + payload->height >
	     surface->height) ||
	    (bytes_per_pixel == 0) ||
	    (fb_gfx3_size_multiply(payload->width, bytes_per_pixel,
	     &row_size) != FB_GFX3_OK) ||
	    (payload->destination_pitch < row_size) ||
	    (fb_gfx3_size_multiply(payload->destination_pitch, payload->height,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != payload->destination_size)) {
		result = FB_GFX3_INVALID;
		goto done;
	}

	destination = (unsigned char *)(uintptr_t)payload->destination_address;
	state->gl.memory_barrier(GL_FRAMEBUFFER_BARRIER_BIT |
		GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
	result = fb_gfx3_gl_bind_color_framebuffer(state, surface->texture);
	if (result != FB_GFX3_OK)
		goto done;
	state->gl.read_buffer(GL_COLOR_ATTACHMENT0);
	state->gl.pixel_store_i(GL_PACK_ALIGNMENT, 1);
	/*
		Most FB.IMAGE targets are tightly packed.  One readback is important
		here because GET is synchronous: issuing one GL call per scanline adds
		driver crossings but cannot expose any additional Basic-visible state.
		Preserve the row loop for callers that deliberately provide padding.
	*/
	if (payload->destination_pitch == row_size) {
		state->gl.read_pixels(payload->source_x, payload->source_y,
			(GLsizei)payload->width, (GLsizei)payload->height,
			GL_RED_INTEGER, type, destination);
	} else {
		for (y = 0; y < payload->height; y++) {
			state->gl.read_pixels(payload->source_x,
				payload->source_y + (int32_t)y, (GLsizei)payload->width, 1,
				GL_RED_INTEGER, type,
				destination + ((size_t)y * payload->destination_pitch));
		}
	}
	state->gl.pixel_store_i(GL_PACK_ALIGNMENT, 4);
	result = fb_gfx3_gl_check_error(state, "surface download");

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

int fb_gfx3_gl_clear(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_CLEAR_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *surface;
	FB_GFX3_RECT clip;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_CLEAR_COMMAND *)command->payload;
	result = fb_gfx3_gl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (fb_gfx3_gl_clip_rect(surface, &payload->clip, &clip))
		result = fb_gfx3_gl_dispatch_clear(state, surface, &clip, payload->color,
			payload->flags);
	else
		result = FB_GFX3_OK;
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

#endif

/* end of gfx3_opengl_surface.c */
