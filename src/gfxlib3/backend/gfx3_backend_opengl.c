/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_backend_opengl.c

    Purpose:

        Execute gfxlib3 surface commands with OpenGL integer textures and
        compute shaders owned by the render thread.

	Responsibilities:

		- use a platform adapter's render-thread-owned OpenGL context
        - initialize and release the backend's shared state
        - dispatch ordered batches to the surface and drawing modules
        - track asynchronous completion with GPU fence objects

    This file intentionally does NOT contain:

        - FreeBASIC coordinate or QB language handling
		- native window creation, WGL policy, or input events
        - CPU reference raster algorithms
        - shader sources, pixel transfers, or primitive implementations
*/

#include "../gfx3_backend_opengl.h"
#include "../gfx3_debug.h"
#include "../gfx3_platform.h"
#include "../gfx3_protocol.h"
#include "../gfx3_resource.h"

#if (defined(HOST_WIN32) || defined(HOST_LINUX)) && \
	!defined(DISABLE_OPENGL)

#if defined(HOST_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#ifndef APIENTRY
#define APIENTRY
#endif

#include "gfx3_opengl_private.h"

/* ------------------------------------------------------------------------- */
/* GPU fence tracking                                                        */
/* ------------------------------------------------------------------------- */

static void opengl_remove_first_fence(FB_GFX3_OPENGL_STATE *state)
{
	FB_GFX3_OPENGL_FENCE *fence = state->first_fence;

	if (fence == NULL)
		return;
	state->first_fence = fence->next;
	if (state->first_fence == NULL)
		state->last_fence = NULL;
	state->completed_sequence = fence->sequence;
	state->gl.delete_sync(fence->sync);
	free(fence);
}

static int opengl_record_fence(FB_GFX3_OPENGL_STATE *state,
	uint64_t sequence)
{
	FB_GFX3_OPENGL_FENCE *fence;

	fence = (FB_GFX3_OPENGL_FENCE *)calloc(1, sizeof(*fence));
	if (fence == NULL) {
		/* A blocking fallback preserves lifetime safety during memory pressure. */
		state->gl.finish();
		while (state->first_fence != NULL)
			opengl_remove_first_fence(state);
		state->completed_sequence = sequence;
		return FB_GFX3_OK;
	}
	fence->sync = state->gl.fence_sync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	if (fence->sync == NULL) {
		free(fence);
		return FB_GFX3_FAILED;
	}
	fence->sequence = sequence;
	if (state->last_fence != NULL)
		state->last_fence->next = fence;
	else
		state->first_fence = fence;
	state->last_fence = fence;
	return FB_GFX3_OK;
}

static int opengl_wait_for_sequence(FB_GFX3_OPENGL_STATE *state,
	uint64_t sequence)
{
	GLenum wait_result;

	if (sequence > state->submitted_sequence)
		return FB_GFX3_INVALID;
	while (state->completed_sequence < sequence) {
		if (state->first_fence == NULL) {
			if (state->control_sequence < sequence)
				return FB_GFX3_FAILED;
			state->completed_sequence = state->control_sequence;
			break;
		}
		wait_result = state->gl.client_wait_sync(state->first_fence->sync,
			GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
		if ((wait_result != GL_ALREADY_SIGNALED) &&
		    (wait_result != GL_CONDITION_SATISFIED))
			return FB_GFX3_FAILED;
		opengl_remove_first_fence(state);
	}
	return FB_GFX3_OK;
}

static uint64_t opengl_poll_completed(FB_GFX3_OPENGL_STATE *state)
{
	GLenum wait_result;

	while (state->first_fence != NULL) {
		wait_result = state->gl.client_wait_sync(state->first_fence->sync,
			0, 0);
		if ((wait_result != GL_ALREADY_SIGNALED) &&
		    (wait_result != GL_CONDITION_SATISFIED))
			break;
		opengl_remove_first_fence(state);
	}
	if ((state->first_fence == NULL) &&
	    (state->control_sequence > state->completed_sequence))
		state->completed_sequence = state->control_sequence;
	return state->completed_sequence;
}

/* ------------------------------------------------------------------------- */
/* Backend interface                                                         */
/* ------------------------------------------------------------------------- */

static int opengl_probe(FB_GFX3_BACKEND_CAPS *caps)
{
	const FB_GFX3_PLATFORM_VTABLE *platform_vtable;

	if (caps == NULL)
		return FB_GFX3_INVALID;
	platform_vtable = fb_gfx3_platform_default();
	if ((platform_vtable == NULL) ||
	    (platform_vtable->probe_opengl == NULL) ||
	    (platform_vtable->probe_opengl() != FB_GFX3_OK))
		return FB_GFX3_UNSUPPORTED;

	memset(caps, 0, sizeof(*caps));
	caps->abi_version = FB_GFX3_BACKEND_ABI_VERSION;
	caps->features = FB_GFX3_FEATURE_INDEXED_SURFACES |
		FB_GFX3_FEATURE_COMPUTE | FB_GFX3_FEATURE_STORAGE_IMAGES |
		FB_GFX3_FEATURE_TIMELINE_FENCES |
		FB_GFX3_FEATURE_PRESENT_IMMEDIATE | FB_GFX3_FEATURE_PACKED_BLITS |
		FB_GFX3_FEATURE_HETEROGENEOUS_BLITS |
		FB_GFX3_FEATURE_PACKED_RECTANGLES |
		FB_GFX3_FEATURE_PACKED_LINES;
	caps->max_surface_width = 16384;
	caps->max_surface_height = 16384;
	caps->max_batch_commands = 4096;
	caps->max_packed_blits = FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT;
	return FB_GFX3_OK;
}

static uint32_t opengl_get_nonnegative_integer(
	const FB_GFX3_OPENGL_STATE *state, GLenum name)
{
	GLint value = 0;

	state->gl.get_integer(name, &value);
	/*
		A core context may reject a legacy default-framebuffer attribute.
		SCREENCONTROL must report that capability as unavailable without
		leaving the error queued for later renderer initialization work.
	*/
	if (state->gl.get_error() != GL_NO_ERROR)
		return 0u;
	return (value > 0) ? (uint32_t)value : 0u;
}

static void opengl_capture_gl_info(FB_GFX3_BACKEND *backend,
	const FB_GFX3_OPENGL_STATE *state)
{
	FB_GFX3_BACKEND_GL_INFO *info = &backend->gl_info;
	GLint extension_count = 0;
	GLint index;
	size_t used = 0;

	memset(info, 0, sizeof(*info));
	/* Context creation and driver setup may leave a diagnostic error behind. */
	while (state->gl.get_error() != GL_NO_ERROR)
		;
	info->available = TRUE;
	info->color_red_bits = opengl_get_nonnegative_integer(state, GL_RED_BITS);
	info->color_green_bits = opengl_get_nonnegative_integer(state,
		GL_GREEN_BITS);
	info->color_blue_bits = opengl_get_nonnegative_integer(state, GL_BLUE_BITS);
	info->color_alpha_bits = opengl_get_nonnegative_integer(state,
		GL_ALPHA_BITS);
	info->color_bits = info->color_red_bits + info->color_green_bits +
		info->color_blue_bits + info->color_alpha_bits;
	/*
		Some core drivers report zero default-framebuffer component widths even
		though the platform adapter deliberately selected an RGBA8 window
		format.  gfxlib3's presentation contract is RGBA8 in that case, so
		publish that known format instead of exposing a driver quirk as a
		zero-colour context.
	*/
	if (info->color_bits == 0u) {
		info->color_red_bits = 8u;
		info->color_green_bits = 8u;
		info->color_blue_bits = 8u;
		info->color_alpha_bits = 8u;
		info->color_bits = 32u;
	}
	info->depth_bits = opengl_get_nonnegative_integer(state, GL_DEPTH_BITS);
	info->stencil_bits = opengl_get_nonnegative_integer(state, GL_STENCIL_BITS);
	/* Core-profile contexts intentionally have no legacy accumulation buffer. */
	info->samples = opengl_get_nonnegative_integer(state, GL_SAMPLES);
	state->gl.get_integer(GL_NUM_EXTENSIONS, &extension_count);
	if ((state->gl.get_error() != GL_NO_ERROR) || (extension_count < 0))
		return;
	for (index = 0; index < extension_count; ++index) {
		const GLubyte *extension = state->gl.get_string_indexed(
			GL_EXTENSIONS, (GLuint)index);
		size_t length;

		if (state->gl.get_error() != GL_NO_ERROR)
			break;
		if (extension == NULL)
			continue;
		length = strlen((const char *)extension);
		if ((used != 0) && (used < sizeof(info->extensions) - 1u))
			info->extensions[used++] = ' ';
		if (length > sizeof(info->extensions) - 1u - used)
			length = sizeof(info->extensions) - 1u - used;
		memcpy(info->extensions + used, extension, length);
		used += length;
		if (used == sizeof(info->extensions) - 1u)
			break;
	}
	info->extensions[used] = '\0';
}

static int opengl_init(FB_GFX3_BACKEND *backend,
	const FB_GFX3_BACKEND_CONFIG *config)
{
	FB_GFX3_OPENGL_STATE *state;
	FB_GFX3_PLATFORM_OPENGL_CONFIG platform_config;
	const GLubyte *version;
	const char *validation;
	GLint major = 0;
	GLint minor = 0;
	GLint maximum_texture_size = 0;
	GLint maximum_groups_x = 0;
	GLint maximum_groups_y = 0;
	GLint maximum_groups_z = 0;
	GLint64 maximum_storage_size = 0;
	int result;

	if ((backend == NULL) || (config == NULL) ||
	    (config->resources == NULL))
		return FB_GFX3_INVALID;
	state = (FB_GFX3_OPENGL_STATE *)calloc(1, sizeof(*state));
	if (state == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	backend->state = state;
	state->resources = config->resources;
	state->logger = config->logger;
	state->platform_vtable = fb_gfx3_platform_default();
	if (state->platform_vtable == NULL)
		return FB_GFX3_UNSUPPORTED;

	memset(&platform_config, 0, sizeof(platform_config));
	platform_config.input = config->platform;
	platform_config.width = config->width;
	platform_config.height = config->height;
	platform_config.major_version = 4;
	platform_config.minor_version = 3;
	platform_config.flags = config->flags;
	platform_config.title = (config->title != NULL) ? config->title :
		"FreeBASIC gfxlib3 OpenGL";
	result = state->platform_vtable->create_opengl(&state->platform,
		&platform_config);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_gl_load_functions(state);
	if (result != FB_GFX3_OK)
		return result;
	state->gl.get_integer(GL_MAJOR_VERSION, &major);
	state->gl.get_integer(GL_MINOR_VERSION, &minor);
	if ((major < 4) || ((major == 4) && (minor < 3)))
		return FB_GFX3_UNSUPPORTED;
	state->gl.get_integer(GL_MAX_TEXTURE_SIZE, &maximum_texture_size);
	state->gl.get_integer_indexed(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0,
		&maximum_groups_x);
	state->gl.get_integer_indexed(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1,
		&maximum_groups_y);
	state->gl.get_integer_indexed(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2,
		&maximum_groups_z);
	state->gl.get_integer64(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,
		&maximum_storage_size);
	if ((maximum_texture_size <= 0) || (maximum_groups_x <= 0) ||
	    (maximum_groups_y <= 0) || (maximum_groups_z <= 0) ||
	    (maximum_storage_size <= 0))
		return FB_GFX3_UNSUPPORTED;
	state->maximum_compute_groups_x = (uint32_t)maximum_groups_x;
	state->maximum_compute_groups_y = (uint32_t)maximum_groups_y;
	state->maximum_compute_groups_z = (uint32_t)maximum_groups_z;
	state->maximum_storage_buffer_size = (uint64_t)maximum_storage_size;
	state->client_width = config->width;
	state->client_height = config->height;
	backend->caps.max_surface_width = (uint32_t)maximum_texture_size;
	backend->caps.max_surface_height = (uint32_t)maximum_texture_size;
	opengl_capture_gl_info(backend, state);

	result = fb_gfx3_gl_create_programs(state);
	if (result != FB_GFX3_OK)
		return result;
	state->gl.generate_buffers(1, &state->point_buffer);
	state->gl.generate_buffers(1, &state->blit_batch_buffer);
	state->gl.generate_buffers(1, &state->blit_tile_range_buffer);
	state->gl.generate_buffers(1, &state->blit_tile_index_buffer);
	state->gl.generate_framebuffers(1, &state->read_framebuffer);
	state->gl.generate_vertex_arrays(1, &state->present_vertex_array);
	state->gl.generate_vertex_arrays(1, &state->blit_batch_vertex_array);
	state->gl.generate_vertex_arrays(1,
		&state->rectangle_batch_vertex_array);
	/*
		Vertex-array objects retain their attribute format and source buffer.
		Only the buffer contents change for later packets, so configure the two
		instanced layouts once instead of toggling six attributes per frame.
	*/
	state->gl.bind_buffer(GL_ARRAY_BUFFER, state->blit_batch_buffer);
	state->gl.bind_vertex_array(state->blit_batch_vertex_array);
	state->gl.enable_vertex_attrib_array(1);
	state->gl.vertex_attrib_i_pointer(1, 4, GL_INT,
		sizeof(FB_GFX3_OPENGL_BLIT_BATCH_ITEM),
		(const void *)offsetof(FB_GFX3_OPENGL_BLIT_BATCH_ITEM, source_x));
	state->gl.vertex_attrib_divisor(1, 1);
	state->gl.enable_vertex_attrib_array(2);
	state->gl.vertex_attrib_i_pointer(2, 4, GL_INT,
		sizeof(FB_GFX3_OPENGL_BLIT_BATCH_ITEM),
		(const void *)offsetof(FB_GFX3_OPENGL_BLIT_BATCH_ITEM, clip_x1));
	state->gl.vertex_attrib_divisor(2, 1);
	state->gl.enable_vertex_attrib_array(3);
	state->gl.vertex_attrib_i_pointer(3, 4, GL_INT,
		sizeof(FB_GFX3_OPENGL_BLIT_BATCH_ITEM),
		(const void *)offsetof(FB_GFX3_OPENGL_BLIT_BATCH_ITEM, destination_x));
	state->gl.vertex_attrib_divisor(3, 1);
	state->gl.bind_vertex_array(state->rectangle_batch_vertex_array);
	state->gl.enable_vertex_attrib_array(1);
	state->gl.vertex_attrib_i_pointer(1, 4, GL_INT,
		sizeof(FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM),
		(const void *)offsetof(FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM, x1));
	state->gl.vertex_attrib_divisor(1, 1);
	state->gl.enable_vertex_attrib_array(2);
	state->gl.vertex_attrib_i_pointer(2, 1, GL_UNSIGNED_INT,
		sizeof(FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM),
		(const void *)offsetof(FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM, color));
	state->gl.vertex_attrib_divisor(2, 1);
	state->gl.bind_vertex_array(state->present_vertex_array);
	result = fb_gfx3_gl_check_error(state, "backend object allocation");
	if (result != FB_GFX3_OK)
		return result;

	validation = getenv("FBGFX3_OPENGL_VALIDATE");
	state->validate_runtime_errors = (validation != NULL) &&
		(validation[0] != '\0') && (strcmp(validation, "0") != 0) &&
		(strcmp(validation, "off") != 0) &&
		(strcmp(validation, "OFF") != 0) &&
		(strcmp(validation, "false") != 0) &&
		(strcmp(validation, "FALSE") != 0);
	state->runtime_ready = TRUE;
	version = state->gl.get_string(GL_VERSION);
	fb_gfx3_log_write(state->logger, FB_GFX3_LOG_INFO,
		"OpenGL gfxlib3 backend initialized: %s",
		(version != NULL) ? (const char *)version : "unknown version");
	return FB_GFX3_OK;
}

static void opengl_shutdown(FB_GFX3_BACKEND *backend)
{
	FB_GFX3_OPENGL_STATE *state;

	if ((backend == NULL) || (backend->state == NULL))
		return;
	state = (FB_GFX3_OPENGL_STATE *)backend->state;
	if (state->gl.finish != NULL)
		state->gl.finish();
	while (state->first_fence != NULL)
		opengl_remove_first_fence(state);
	if ((state->point_buffer != 0) && (state->gl.delete_buffers != NULL))
		state->gl.delete_buffers(1, &state->point_buffer);
	if ((state->blit_batch_buffer != 0) &&
	    (state->gl.delete_buffers != NULL))
		state->gl.delete_buffers(1, &state->blit_batch_buffer);
	if ((state->blit_tile_range_buffer != 0) &&
	    (state->gl.delete_buffers != NULL))
		state->gl.delete_buffers(1, &state->blit_tile_range_buffer);
	if ((state->blit_tile_index_buffer != 0) &&
	    (state->gl.delete_buffers != NULL))
		state->gl.delete_buffers(1, &state->blit_tile_index_buffer);
	if ((state->paint_scratch_buffer != 0) &&
	    (state->gl.delete_buffers != NULL))
		state->gl.delete_buffers(1, &state->paint_scratch_buffer);
	if ((state->blit_batch_winner_texture != 0) &&
	    (state->gl.delete_textures != NULL))
		state->gl.delete_textures(1, &state->blit_batch_winner_texture);
	if ((state->present_vertex_array != 0) &&
	    (state->gl.delete_vertex_arrays != NULL))
		state->gl.delete_vertex_arrays(1, &state->present_vertex_array);
	if ((state->blit_batch_vertex_array != 0) &&
	    (state->gl.delete_vertex_arrays != NULL))
		state->gl.delete_vertex_arrays(1,
			&state->blit_batch_vertex_array);
	if ((state->rectangle_batch_vertex_array != 0) &&
	    (state->gl.delete_vertex_arrays != NULL))
		state->gl.delete_vertex_arrays(1,
			&state->rectangle_batch_vertex_array);
	if ((state->read_framebuffer != 0) &&
	    (state->gl.delete_framebuffers != NULL))
		state->gl.delete_framebuffers(1, &state->read_framebuffer);
	if ((state->points_program != 0) && (state->gl.delete_program != NULL))
		state->gl.delete_program(state->points_program);
	if ((state->points_batch_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->points_batch_program);
	if ((state->points_batch_resolve_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->points_batch_resolve_program);
	if ((state->glyph_batch_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->glyph_batch_program);
	if ((state->line_program != 0) && (state->gl.delete_program != NULL))
		state->gl.delete_program(state->line_program);
	if ((state->line_batch_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->line_batch_program);
	if ((state->line_batch_resolve_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->line_batch_resolve_program);
	if ((state->rectangle_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->rectangle_program);
	if ((state->ellipse_program != 0) && (state->gl.delete_program != NULL))
		state->gl.delete_program(state->ellipse_program);
	if ((state->ellipse_batch_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->ellipse_batch_program);
	if ((state->ellipse_batch_resolve_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->ellipse_batch_resolve_program);
	if ((state->primitive_batch_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->primitive_batch_program);
	if ((state->primitive_batch_resolve_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->primitive_batch_resolve_program);
	if ((state->paint_program != 0) && (state->gl.delete_program != NULL))
		state->gl.delete_program(state->paint_program);
	if ((state->blit_program != 0) && (state->gl.delete_program != NULL))
		state->gl.delete_program(state->blit_program);
	if ((state->transform_blit_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->transform_blit_program);
	if ((state->blit_alpha_tile_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->blit_alpha_tile_program);
	if ((state->blit_batch_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->blit_batch_program);
	if ((state->blit_batch_resolve_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->blit_batch_resolve_program);
	if ((state->blit_raster_batch_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->blit_raster_batch_program);
	if ((state->rectangle_raster_batch_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->rectangle_raster_batch_program);
	if ((state->rectangle_batch_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->rectangle_batch_program);
	if ((state->rectangle_batch_resolve_program != 0) &&
	    (state->gl.delete_program != NULL))
		state->gl.delete_program(state->rectangle_batch_resolve_program);
	if ((state->present_program != 0) && (state->gl.delete_program != NULL))
		state->gl.delete_program(state->present_program);
	if ((state->clear_program != 0) && (state->gl.delete_program != NULL))
		state->gl.delete_program(state->clear_program);

	if ((state->platform_vtable != NULL) &&
	    (state->platform_vtable->destroy != NULL))
		state->platform_vtable->destroy(state->platform);
	free(state);
	backend->state = NULL;
}

static int opengl_command_writes_visible(
	const FB_GFX3_OPENGL_STATE *state, const FB_GFX3_COMMAND *command)
{
	if ((state->visible_surface == 0) ||
	    (command->target != state->visible_surface))
		return FALSE;
	switch (command->type) {
	case FB_GFX3_COMMAND_SURFACE_UPLOAD:
	case FB_GFX3_COMMAND_CLEAR:
	case FB_GFX3_COMMAND_POINTS:
	case FB_GFX3_COMMAND_GLYPHS:
	case FB_GFX3_COMMAND_LINE:
	case FB_GFX3_COMMAND_LINES:
	case FB_GFX3_COMMAND_RECTANGLE:
	case FB_GFX3_COMMAND_RECTANGLES:
	case FB_GFX3_COMMAND_ELLIPSE:
	case FB_GFX3_COMMAND_PAINT:
	case FB_GFX3_COMMAND_BLIT:
	case FB_GFX3_COMMAND_BLITS:
	case FB_GFX3_COMMAND_TRANSFORM_BLIT:
		return TRUE;
	default:
		return FALSE;
	}
}

/*
	An opaque clear replaces every pixel in the same logical region. Adjacent
	clears of an identical region therefore have no observable intermediate
	state, because no command between them can read that surface. Coalescing
	them removes redundant compute dispatches while retaining the final colour.
*/
static uint32_t opengl_clear_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available)
{
	const FB_GFX3_CLEAR_COMMAND *first;
	uint32_t count = 1;

	if ((commands == NULL) || (available == 0) || (commands[0] == NULL) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1;
	first = (const FB_GFX3_CLEAR_COMMAND *)commands[0]->payload;
	if ((first->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0)
		return 1;
	while (count < available) {
		const FB_GFX3_CLEAR_COMMAND *candidate;

		if ((commands[count] == NULL) ||
		    (commands[count]->type != FB_GFX3_COMMAND_CLEAR) ||
		    (commands[count]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[count]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_CLEAR_COMMAND *)commands[count]->payload;
		if (((candidate->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0) ||
		    (memcmp(&candidate->clip, &first->clip,
		     sizeof(candidate->clip)) != 0))
			break;
		count++;
	}
	return count;
}

/*
	An opaque solid PAINT changes every reachable non-border pixel to another
	non-border colour. With the same target, clip, seed, and border, that leaves
	the flood topology unchanged for the next PAINT. No command can observe the
	intermediate colours in an adjacent renderer run, so only the final fill is
	needed. A border-coloured, patterned, or alpha fill is deliberately excluded
	because it can change reachability or depend on the previous pixel value.
*/
static uint32_t opengl_paint_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available)
{
	const FB_GFX3_PAINT_COMMAND *first;
	uint32_t count = 1u;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_PAINT) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1u;
	first = (const FB_GFX3_PAINT_COMMAND *)commands[0]->payload;
	if ((first->paint_mode != 0u) || (first->pattern_size != 0u) ||
	    ((first->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
	    (first->color == first->border_color))
		return 1u;
	while (count < available) {
		const FB_GFX3_PAINT_COMMAND *candidate;

		if ((commands[count] == NULL) ||
		    (commands[count]->type != FB_GFX3_COMMAND_PAINT) ||
		    (commands[count]->target != commands[0]->target) ||
		    (commands[count]->sequence <= commands[count - 1u]->sequence) ||
		    (fb_gfx3_command_payload_size(commands[count]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_PAINT_COMMAND *)commands[count]->payload;
		if ((candidate->paint_mode != 0u) ||
		    (candidate->pattern_size != 0u) ||
		    ((candidate->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
		    (candidate->color == candidate->border_color) ||
		    (candidate->x != first->x) || (candidate->y != first->y) ||
		    (candidate->border_color != first->border_color) ||
		    (memcmp(&candidate->clip, &first->clip,
		     sizeof(candidate->clip)) != 0))
			break;
		count++;
	}
	return count;
}

/* Palette reads are served from the public CPU state. Until a PRESENT command
   intervenes, only the final queued GPU palette can be observed. */
static uint32_t opengl_palette_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available)
{
	uint32_t count = 1;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_PALETTE) ||
	    (fb_gfx3_command_payload_size(commands[0]) !=
	     sizeof(FB_GFX3_PALETTE_COMMAND)))
		return 1;
	while ((count < available) && (commands[count] != NULL) &&
	       (commands[count]->type == FB_GFX3_COMMAND_PALETTE) &&
	       (fb_gfx3_command_payload_size(commands[count]) ==
	        sizeof(FB_GFX3_PALETTE_COMMAND)))
		count++;
	return count;
}

static int opengl_full_page_copy_description(
	const FB_GFX3_COMMAND *command,
	FB_GFX3_OPENGL_PAGE_COPY_DESCRIPTION *description)
{
	const FB_GFX3_BLIT_COMMAND *blit;

	if ((command == NULL) || (description == NULL))
		return FALSE;
	if (command->type == FB_GFX3_COMMAND_BLIT) {
		if (fb_gfx3_command_payload_size(command) != sizeof(*blit))
			return FALSE;
		blit = (const FB_GFX3_BLIT_COMMAND *)command->payload;
	} else if (command->type == FB_GFX3_COMMAND_BLITS) {
		const FB_GFX3_BLITS_COMMAND *blits;
		size_t expected_size;

		if (fb_gfx3_command_payload_size(command) <
		    offsetof(FB_GFX3_BLITS_COMMAND, blit))
			return FALSE;
		blits = (const FB_GFX3_BLITS_COMMAND *)command->payload;
		if ((blits->count != 1u) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_BLITS_COMMAND, blit),
		     sizeof(blits->blit[0]), &expected_size) != FB_GFX3_OK) ||
		    (expected_size != fb_gfx3_command_payload_size(command)))
			return FALSE;
		blit = &blits->blit[0];
		if ((blit->source != blits->source) ||
		    (blit->mode != blits->mode) ||
		    (blit->alpha != blits->alpha))
			return FALSE;
	} else {
		return FALSE;
	}
	if ((blit->source == command->target) ||
	    (blit->mode != FB_GFX3_BLIT_PSET) ||
	    (blit->source_rect.x1 != 0) || (blit->source_rect.y1 != 0) ||
	    (blit->destination_x != 0) || (blit->destination_y != 0) ||
	    (blit->clip.x1 > 0) || (blit->clip.y1 > 0))
		return FALSE;
	description->source = blit->source;
	description->clip = blit->clip;
	description->source_rect = blit->source_rect;
	description->destination_x = blit->destination_x;
	description->destination_y = blit->destination_y;
	return TRUE;
}

static int opengl_command_is_deferred_full_page(
	const FB_GFX3_COMMAND *command)
{
	FB_GFX3_OPENGL_PAGE_COPY_DESCRIPTION description;

	if (command == NULL)
		return FALSE;
	if (command->type == FB_GFX3_COMMAND_PAGE_SET)
		return fb_gfx3_command_payload_size(command) ==
			sizeof(FB_GFX3_PAGE_SET_COMMAND);
	return opengl_full_page_copy_description(command, &description);
}

static void opengl_page_content_invalidate(FB_GFX3_OPENGL_STATE *state)
{
	if (state != NULL)
		state->page_content_count = 0u;
}

static uint32_t opengl_page_content_find(const FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_HANDLE handle)
{
	uint32_t index;

	if ((state == NULL) || (handle == 0))
		return UINT32_MAX;
	for (index = 0u; index < state->page_content_count; index++) {
		if (state->page_content_handle[index] == handle)
			return index;
	}
	return UINT32_MAX;
}

static void opengl_page_content_pair(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_HANDLE destination, FB_GFX3_HANDLE source,
	uint32_t *destination_index, uint32_t *source_index)
{
	uint32_t missing;

	*destination_index = opengl_page_content_find(state, destination);
	*source_index = opengl_page_content_find(state, source);
	missing = (*destination_index == UINT32_MAX) ? 1u : 0u;
	if (*source_index == UINT32_MAX)
		missing++;
	if ((missing > FB_GFX3_OPENGL_PAGE_CONTENT_LIMIT -
	     state->page_content_count) ||
	    (state->next_page_content_token > UINT64_MAX - 2u)) {
		opengl_page_content_invalidate(state);
		state->next_page_content_token = 0u;
		*destination_index = UINT32_MAX;
		*source_index = UINT32_MAX;
	}
	if (*destination_index == UINT32_MAX) {
		*destination_index = state->page_content_count++;
		state->page_content_handle[*destination_index] = destination;
		state->page_content_token[*destination_index] =
			++state->next_page_content_token;
	}
	if (*source_index == UINT32_MAX) {
		*source_index = state->page_content_count++;
		state->page_content_handle[*source_index] = source;
		state->page_content_token[*source_index] =
			++state->next_page_content_token;
	}
}

static int opengl_full_page_copy(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	FB_GFX3_OPENGL_PAGE_COPY_DESCRIPTION description;
	FB_GFX3_OPENGL_SURFACE *destination;
	FB_GFX3_OPENGL_SURFACE *source;
	uint32_t destination_index;
	uint32_t source_index;
	int result;

	if (!opengl_full_page_copy_description(command, &description))
		return FB_GFX3_UNSUPPORTED;
	result = fb_gfx3_gl_surface_retain(state, command->target,
		command->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_gl_surface_retain(state, description.source,
		command->sequence, &source);
	if (result != FB_GFX3_OK) {
		fb_gfx3_resource_release(state->resources, command->target);
		return result;
	}
	if ((destination->width != source->width) ||
	    (destination->height != source->height) ||
	    (destination->depth != source->depth) ||
	    (description.source_rect.x2 != (int32_t)source->width - 1) ||
	    (description.source_rect.y2 != (int32_t)source->height - 1) ||
	    (description.clip.x2 < (int32_t)destination->width - 1) ||
	    (description.clip.y2 < (int32_t)destination->height - 1)) {
		result = FB_GFX3_UNSUPPORTED;
		goto cleanup;
	}
	opengl_page_content_pair(state, command->target, description.source,
		&destination_index, &source_index);
	if (state->page_content_token[destination_index] ==
	    state->page_content_token[source_index]) {
		result = FB_GFX3_OK;
		goto cleanup;
	}
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_UPDATE_BARRIER_BIT);
	state->gl.copy_image_sub_data(source->texture, GL_TEXTURE_2D, 0,
		0, 0, 0, destination->texture, GL_TEXTURE_2D, 0, 0, 0, 0,
		(GLsizei)source->width, (GLsizei)source->height, 1);
	state->gl.memory_barrier(GL_TEXTURE_UPDATE_BARRIER_BIT |
		GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	result = fb_gfx3_gl_check_error(state, "full page image copy");
	if (result == FB_GFX3_OK)
		state->page_content_token[destination_index] =
			state->page_content_token[source_index];

cleanup:
	fb_gfx3_resource_release(state->resources, description.source);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int opengl_full_page_copy_validate(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	FB_GFX3_OPENGL_PAGE_COPY_DESCRIPTION description;
	FB_GFX3_OPENGL_SURFACE *destination;
	FB_GFX3_OPENGL_SURFACE *source;
	int result;

	if (!opengl_full_page_copy_description(command, &description))
		return FB_GFX3_UNSUPPORTED;
	result = fb_gfx3_gl_surface_retain(state, command->target,
		command->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_gl_surface_retain(state, description.source,
		command->sequence, &source);
	if (result != FB_GFX3_OK) {
		fb_gfx3_resource_release(state->resources, command->target);
		return result;
	}
	if ((destination->width != source->width) ||
	    (destination->height != source->height) ||
	    (destination->depth != source->depth) ||
	    (description.source_rect.x2 != (int32_t)source->width - 1) ||
	    (description.source_rect.y2 != (int32_t)source->height - 1) ||
	    (description.clip.x2 < (int32_t)destination->width - 1) ||
	    (description.clip.y2 < (int32_t)destination->height - 1))
		result = FB_GFX3_UNSUPPORTED;
	fb_gfx3_resource_release(state->resources, description.source);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static uint32_t opengl_full_page_copy_batch_count(
	FB_GFX3_COMMAND *const *commands, size_t available)
{
	FB_GFX3_OPENGL_PAGE_COPY_DESCRIPTION description;
	uint32_t count = 0u;

	if (commands == NULL)
		return 0u;
	while ((count < available) &&
	       (count < FB_GFX3_OPENGL_PAGE_COPY_BATCH_LIMIT) &&
	       opengl_full_page_copy_description(commands[count], &description))
		count++;
	return count;
}

/*
	A renderer drain is the first point where the complete ordered page run is
	known. Backward liveness retains a copy when its destination survives the
	drain or feeds a later retained source. Copies overwritten before either can
	be observed are removed without changing any page's final contents.
*/
static int opengl_full_page_copy_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t command_count)
{
	FB_GFX3_HANDLE handles[FB_GFX3_OPENGL_PAGE_COPY_BATCH_LIMIT * 2u];
	unsigned char surface_needed[
		FB_GFX3_OPENGL_PAGE_COPY_BATCH_LIMIT * 2u];
	unsigned char copy_needed[FB_GFX3_OPENGL_PAGE_COPY_BATCH_LIMIT];
	uint32_t destination_index[FB_GFX3_OPENGL_PAGE_COPY_BATCH_LIMIT];
	uint32_t source_index[FB_GFX3_OPENGL_PAGE_COPY_BATCH_LIMIT];
	uint32_t handle_count = 0u;
	uint32_t command_index;
	int result;

	if ((state == NULL) || (commands == NULL) || (command_count < 2u) ||
	    (command_count > FB_GFX3_OPENGL_PAGE_COPY_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	for (command_index = 0u; command_index < command_count; command_index++) {
		FB_GFX3_OPENGL_PAGE_COPY_DESCRIPTION description;
		uint32_t index;

		result = opengl_full_page_copy_validate(state,
			commands[command_index]);
		if (result != FB_GFX3_OK)
			return result;
		opengl_full_page_copy_description(commands[command_index],
			&description);
		destination_index[command_index] = UINT32_MAX;
		source_index[command_index] = UINT32_MAX;
		for (index = 0u; index < handle_count; index++) {
			if (handles[index] == commands[command_index]->target)
				destination_index[command_index] = index;
			if (handles[index] == description.source)
				source_index[command_index] = index;
		}
		if (destination_index[command_index] == UINT32_MAX) {
			destination_index[command_index] = handle_count;
			handles[handle_count++] = commands[command_index]->target;
		}
		if (source_index[command_index] == UINT32_MAX) {
			source_index[command_index] = handle_count;
			handles[handle_count++] = description.source;
		}
	}
	memset(surface_needed, 1,
		handle_count * sizeof(surface_needed[0]));
	memset(copy_needed, 0, sizeof(copy_needed));
	for (command_index = command_count; command_index != 0u;
	     command_index--) {
		uint32_t copy_index = command_index - 1u;
		uint32_t destination = destination_index[copy_index];
		uint32_t source = source_index[copy_index];

		if (!surface_needed[destination])
			continue;
		copy_needed[copy_index] = TRUE;
		surface_needed[destination] = FALSE;
		surface_needed[source] = TRUE;
	}
	for (command_index = 0u; command_index < command_count; command_index++) {
		if (!copy_needed[command_index])
			continue;
		result = opengl_full_page_copy(state, commands[command_index]);
		if (result != FB_GFX3_OK)
			return result;
	}
	return FB_GFX3_OK;
}

static int opengl_command_changes_surface(const FB_GFX3_COMMAND *command)
{
	if (command == NULL)
		return FALSE;
	switch (command->type) {
	case FB_GFX3_COMMAND_SURFACE_CREATE:
	case FB_GFX3_COMMAND_SURFACE_DESTROY:
	case FB_GFX3_COMMAND_SURFACE_UPLOAD:
	case FB_GFX3_COMMAND_CLEAR:
	case FB_GFX3_COMMAND_POINTS:
	case FB_GFX3_COMMAND_GLYPHS:
	case FB_GFX3_COMMAND_LINE:
	case FB_GFX3_COMMAND_LINES:
	case FB_GFX3_COMMAND_RECTANGLE:
	case FB_GFX3_COMMAND_RECTANGLES:
	case FB_GFX3_COMMAND_ELLIPSE:
	case FB_GFX3_COMMAND_PAINT:
	case FB_GFX3_COMMAND_BLIT:
	case FB_GFX3_COMMAND_BLITS:
	case FB_GFX3_COMMAND_TRANSFORM_BLIT:
		return TRUE;
	default:
		return FALSE;
	}
}

static int opengl_command_requires_fence(const FB_GFX3_COMMAND *command)
{
	if (command == NULL)
		return TRUE;
	/*
		These commands change only CPU or window state. A new GLsync for every
		idle PLATFORM_POLL made the desktop driver allocate and query dozens of
		fences per second even when no graphics work existed.

		BARRIER remains ordered without a new fence: control_sequence is
		published only after all older GL fences have completed.
	*/
	switch (command->type) {
	case FB_GFX3_COMMAND_BARRIER:
	case FB_GFX3_COMMAND_PLATFORM_POLL:
	case FB_GFX3_COMMAND_INPUT_POLL:
	case FB_GFX3_COMMAND_WINDOW_TITLE:
		return FALSE;
	default:
		return TRUE;
	}
}

static int opengl_execute(FB_GFX3_BACKEND *backend,
	FB_GFX3_COMMAND *const *commands, size_t count,
	uint64_t *submitted_sequence)
{
	FB_GFX3_OPENGL_STATE *state;
	FB_GFX3_COMMAND *command;
	uint64_t last_sequence;
	size_t i;
	uint32_t batch_count;
	uint32_t batch_index;
	uint32_t primitive_count;
	uint32_t client_width;
	uint32_t client_height;
	int defer_page_presentation = TRUE;
	int requires_fence;
	int result;

	if ((backend == NULL) || (backend->state == NULL) ||
	    (commands == NULL) || (count == 0))
		return FB_GFX3_INVALID;
	state = (FB_GFX3_OPENGL_STATE *)backend->state;
	state->platform_vtable->pump_events(state->platform);
	result = state->platform_vtable->client_size(state->platform,
		&client_width, &client_height);
	if (result != FB_GFX3_OK)
		return result;
	if ((client_width != state->client_width) ||
	    (client_height != state->client_height)) {
		state->client_width = client_width;
		state->client_height = client_height;
		if ((client_width != 0u) && (client_height != 0u) &&
		    (state->visible_surface != 0))
			state->presentation_dirty = TRUE;
	}
	/*
		A dirty page issues a presentation draw even if this batch contains only
		a platform poll, so that case still requires a fence.
	*/
	requires_fence = state->presentation_dirty &&
		(state->visible_surface != 0);
	for (i = 0u; i < count; i++) {
		if (opengl_command_requires_fence(commands[i]))
			requires_fence = TRUE;
		if (!opengl_command_is_deferred_full_page(commands[i])) {
			defer_page_presentation = FALSE;
			break;
		}
	}
	last_sequence = state->submitted_sequence;
	for (i = 0; i < count;) {
		FB_GFX3_OPENGL_PAGE_COPY_DESCRIPTION page_copy_description;

		command = commands[i];
		if ((command == NULL) || (command->sequence <= last_sequence))
			return FB_GFX3_INVALID;
		if (opengl_full_page_copy_description(command,
		    &page_copy_description)) {
			batch_count = opengl_full_page_copy_batch_count(commands + i,
				count - i);
			if (batch_count > 1u) {
				result = opengl_full_page_copy_batch(state, commands + i,
					batch_count);
				if (result == FB_GFX3_OK) {
					for (batch_index = 0u; batch_index < batch_count;
					     batch_index++) {
						command = commands[i + batch_index];
						if (opengl_command_writes_visible(state, command))
							state->presentation_dirty = TRUE;
						last_sequence = command->sequence;
					}
					i += batch_count;
					continue;
				}
				if (result != FB_GFX3_UNSUPPORTED)
					return result;
			}
			result = opengl_full_page_copy(state, command);
			if (result == FB_GFX3_OK) {
				if (opengl_command_writes_visible(state, command))
					state->presentation_dirty = TRUE;
				last_sequence = command->sequence;
				i++;
				continue;
			}
			if (result != FB_GFX3_UNSUPPORTED)
				return result;
		}
		/* Any other surface write ends the conservative page-content epoch. */
		if (opengl_command_changes_surface(command))
			opengl_page_content_invalidate(state);
		if ((command->type == FB_GFX3_COMMAND_POINTS) ||
		    (command->type == FB_GFX3_COMMAND_LINE) ||
		    (command->type == FB_GFX3_COMMAND_LINES) ||
		    (command->type == FB_GFX3_COMMAND_RECTANGLE) ||
		    (command->type == FB_GFX3_COMMAND_RECTANGLES) ||
		    (command->type == FB_GFX3_COMMAND_ELLIPSE)) {
			batch_count = fb_gfx3_gl_primitive_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX :
				count - i), &primitive_count);
			if (batch_count > 1u) {
				result = fb_gfx3_gl_primitive_batch(state, commands + i,
					batch_count, primitive_count);
				if (result == FB_GFX3_UNSUPPORTED) {
					batch_count = 1u;
				} else if (result != FB_GFX3_OK) {
					fb_gfx3_log_write(state->logger, FB_GFX3_LOG_ERROR,
						"OpenGL mixed primitive sequence %llu failed: %d",
						(unsigned long long)command->sequence, result);
					return result;
				}
				if (batch_count > 1u) {
					for (batch_index = 0u; batch_index < batch_count;
					     batch_index++) {
						command = commands[i + batch_index];
						if (opengl_command_writes_visible(state, command))
							state->presentation_dirty = TRUE;
						last_sequence = command->sequence;
					}
					i += batch_count;
					continue;
				}
			}
		}
		if (command->type == FB_GFX3_COMMAND_BLIT) {
			batch_count = fb_gfx3_gl_alpha_blit_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX :
				(count - i)));
			if (batch_count > 1) {
				result = fb_gfx3_gl_alpha_blit_batch(state, commands + i, batch_count);
				if (result == FB_GFX3_UNSUPPORTED)
					batch_count = 1;
				else if (result != FB_GFX3_OK) {
					fb_gfx3_log_write(state->logger, FB_GFX3_LOG_ERROR,
						"OpenGL alpha tile batch sequence %llu failed: %d",
						(unsigned long long)command->sequence, result);
					return result;
				}
				if (batch_count > 1) {
					for (batch_index = 0; batch_index < batch_count;
					     ++batch_index) {
						command = commands[i + batch_index];
						if (opengl_command_writes_visible(state, command))
							state->presentation_dirty = TRUE;
						last_sequence = command->sequence;
					}
					i += batch_count;
					continue;
				}
			}
			batch_count = fb_gfx3_gl_blit_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX :
				(count - i)));
			if (batch_count > 1) {
				for (batch_index = 1; batch_index < batch_count;
				     ++batch_index) {
					if ((commands[i + batch_index] == NULL) ||
					    (commands[i + batch_index]->sequence <=
					     commands[i + batch_index - 1]->sequence))
						return FB_GFX3_INVALID;
				}
				result = fb_gfx3_gl_blit_batch(state, commands + i, batch_count);
				if (result != FB_GFX3_OK) {
					fb_gfx3_log_write(state->logger, FB_GFX3_LOG_ERROR,
						"OpenGL command %u sequence %llu failed: %d",
						command->type,
						(unsigned long long)command->sequence, result);
					return result;
				}
				for (batch_index = 0; batch_index < batch_count;
				     ++batch_index) {
					command = commands[i + batch_index];
					if (opengl_command_writes_visible(state, command))
						state->presentation_dirty = TRUE;
					last_sequence = command->sequence;
				}
				i += batch_count;
				continue;
			}
		}
		if (command->type == FB_GFX3_COMMAND_GLYPHS) {
			batch_count = fb_gfx3_gl_glyph_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX :
				(count - i)));
			for (batch_index = 1u; batch_index < batch_count;
			     ++batch_index) {
				if ((commands[i + batch_index] == NULL) ||
				    (commands[i + batch_index]->sequence <=
				     commands[i + batch_index - 1u]->sequence))
					return FB_GFX3_INVALID;
			}
			result = fb_gfx3_gl_glyph_batch(state, commands + i, batch_count);
			if (result != FB_GFX3_OK) {
				fb_gfx3_log_write(state->logger, FB_GFX3_LOG_ERROR,
					"OpenGL glyph command sequence %llu failed: %d",
					(unsigned long long)command->sequence, result);
				return result;
			}
			for (batch_index = 0u; batch_index < batch_count;
			     ++batch_index) {
				command = commands[i + batch_index];
				if (opengl_command_writes_visible(state, command))
					state->presentation_dirty = TRUE;
				last_sequence = command->sequence;
			}
			i += batch_count;
			continue;
		}
		if (command->type == FB_GFX3_COMMAND_POINTS) {
			batch_count = fb_gfx3_gl_points_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX :
				(count - i)));
			if (batch_count > 1) {
				for (batch_index = 1; batch_index < batch_count;
				     ++batch_index) {
					if ((commands[i + batch_index] == NULL) ||
					    (commands[i + batch_index]->sequence <=
					     commands[i + batch_index - 1]->sequence))
						return FB_GFX3_INVALID;
				}
				result = fb_gfx3_gl_points_batch(state, commands + i, batch_count);
				if (result != FB_GFX3_OK) {
					fb_gfx3_log_write(state->logger, FB_GFX3_LOG_ERROR,
						"OpenGL command %u sequence %llu failed: %d",
						command->type,
						(unsigned long long)command->sequence, result);
					return result;
				}
				for (batch_index = 0; batch_index < batch_count;
				     ++batch_index) {
					command = commands[i + batch_index];
					if (opengl_command_writes_visible(state, command))
						state->presentation_dirty = TRUE;
					last_sequence = command->sequence;
				}
				i += batch_count;
				continue;
			}
		}
		if (command->type == FB_GFX3_COMMAND_LINE) {
			batch_count = fb_gfx3_gl_line_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX :
				(count - i)));
			if (batch_count > 1) {
				for (batch_index = 1; batch_index < batch_count;
				     ++batch_index) {
					if ((commands[i + batch_index] == NULL) ||
					    (commands[i + batch_index]->sequence <=
					     commands[i + batch_index - 1]->sequence))
						return FB_GFX3_INVALID;
				}
				result = fb_gfx3_gl_line_batch(state, commands + i, batch_count);
				if (result != FB_GFX3_OK) {
					fb_gfx3_log_write(state->logger, FB_GFX3_LOG_ERROR,
						"OpenGL command %u sequence %llu failed: %d",
						command->type,
						(unsigned long long)command->sequence, result);
					return result;
				}
				for (batch_index = 0; batch_index < batch_count;
				     ++batch_index) {
					command = commands[i + batch_index];
					if (opengl_command_writes_visible(state, command))
						state->presentation_dirty = TRUE;
					last_sequence = command->sequence;
				}
				i += batch_count;
				continue;
			}
		}
		if (command->type == FB_GFX3_COMMAND_RECTANGLE) {
			batch_count = fb_gfx3_gl_rectangle_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX :
				(count - i)));
			if (batch_count > 1) {
				for (batch_index = 1; batch_index < batch_count;
				     ++batch_index) {
					if ((commands[i + batch_index] == NULL) ||
					    (commands[i + batch_index]->sequence <=
					     commands[i + batch_index - 1]->sequence))
						return FB_GFX3_INVALID;
				}
				result = fb_gfx3_gl_rectangle_batch(state, commands + i,
					batch_count);
				if (result != FB_GFX3_OK) {
					fb_gfx3_log_write(state->logger, FB_GFX3_LOG_ERROR,
						"OpenGL command %u sequence %llu failed: %d",
						command->type,
						(unsigned long long)command->sequence, result);
					return result;
				}
				for (batch_index = 0; batch_index < batch_count;
				     ++batch_index) {
					command = commands[i + batch_index];
					if (opengl_command_writes_visible(state, command))
						state->presentation_dirty = TRUE;
					last_sequence = command->sequence;
				}
				i += batch_count;
				continue;
			}
		}
		if (command->type == FB_GFX3_COMMAND_CLEAR) {
			batch_count = opengl_clear_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX :
				(count - i)));
			if (batch_count > 1) {
				result = fb_gfx3_gl_clear(state, commands[i + batch_count - 1]);
				if (result != FB_GFX3_OK)
					return result;
				for (batch_index = 0; batch_index < batch_count;
				     batch_index++) {
					command = commands[i + batch_index];
					if (opengl_command_writes_visible(state, command))
						state->presentation_dirty = TRUE;
					last_sequence = command->sequence;
				}
				i += batch_count;
				continue;
			}
		}
		if (command->type == FB_GFX3_COMMAND_PAINT) {
			batch_count = opengl_paint_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX :
				(count - i)));
			if (batch_count > 1u) {
				result = fb_gfx3_gl_paint(state,
					commands[i + batch_count - 1u]);
				if (result != FB_GFX3_OK)
					return result;
				for (batch_index = 0u; batch_index < batch_count;
				     batch_index++) {
					command = commands[i + batch_index];
					if (opengl_command_writes_visible(state, command))
						state->presentation_dirty = TRUE;
					last_sequence = command->sequence;
				}
				i += batch_count;
				continue;
			}
		}
		if (command->type == FB_GFX3_COMMAND_PALETTE) {
			batch_count = opengl_palette_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX : count - i));
			if (batch_count > 1) {
				result = fb_gfx3_gl_palette(state, commands[i + batch_count - 1u]);
				if (result != FB_GFX3_OK)
					return result;
				for (batch_index = 0; batch_index < batch_count; batch_index++)
					last_sequence = commands[i + batch_index]->sequence;
				i += batch_count;
				continue;
			}
		}
		switch (command->type) {
		case FB_GFX3_COMMAND_SURFACE_CREATE:
			result = fb_gfx3_gl_surface_create(state, command);
			break;
		case FB_GFX3_COMMAND_SURFACE_DESTROY:
			if (command->target == state->visible_surface) {
				state->visible_surface = 0;
				state->presentation_dirty = FALSE;
			}
			result = fb_gfx3_gl_surface_release(state, command);
			break;
		case FB_GFX3_COMMAND_SURFACE_UPLOAD:
			result = fb_gfx3_gl_surface_upload(state, command);
			break;
		case FB_GFX3_COMMAND_SURFACE_DOWNLOAD:
			result = fb_gfx3_gl_surface_download(state, command);
			break;
		case FB_GFX3_COMMAND_CLEAR:
			result = fb_gfx3_gl_clear(state, command);
			break;
		case FB_GFX3_COMMAND_POINTS:
			result = fb_gfx3_gl_points(state, command);
			break;
		case FB_GFX3_COMMAND_LINE:
			result = fb_gfx3_gl_line(state, command);
			break;
		case FB_GFX3_COMMAND_LINES:
			result = fb_gfx3_gl_lines(state, command);
			break;
		case FB_GFX3_COMMAND_RECTANGLE:
			result = fb_gfx3_gl_rectangle(state, command);
			break;
		case FB_GFX3_COMMAND_RECTANGLES:
			result = fb_gfx3_gl_rectangles(state, command);
			break;
		case FB_GFX3_COMMAND_ELLIPSE:
			batch_count = fb_gfx3_gl_ellipse_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX :
				count - i));
			if (batch_count > 1u) {
				result = fb_gfx3_gl_ellipse_batch(state, commands + i, batch_count);
				if (result == FB_GFX3_UNSUPPORTED)
					batch_count = 1u;
				else if (result != FB_GFX3_OK)
					return result;
				if (batch_count > 1u) {
					for (batch_index = 0u; batch_index < batch_count;
					     ++batch_index) {
						command = commands[i + batch_index];
						if (opengl_command_writes_visible(state, command))
							state->presentation_dirty = TRUE;
						last_sequence = command->sequence;
					}
					i += batch_count;
					continue;
				}
			}
			result = fb_gfx3_gl_ellipse(state, command);
			break;
		case FB_GFX3_COMMAND_PAINT:
			result = fb_gfx3_gl_paint(state, command);
			break;
		case FB_GFX3_COMMAND_BLIT:
			result = fb_gfx3_gl_blit(state, command);
			break;
		case FB_GFX3_COMMAND_BLITS:
			result = fb_gfx3_gl_blits(state, command);
			break;
		case FB_GFX3_COMMAND_TRANSFORM_BLIT:
			result = fb_gfx3_gl_transform_blit(state, command);
			break;
		case FB_GFX3_COMMAND_READ_PIXEL:
			result = fb_gfx3_gl_read_pixel(state, command);
			break;
		case FB_GFX3_COMMAND_PALETTE:
			result = fb_gfx3_gl_palette(state, command);
			break;
		case FB_GFX3_COMMAND_PAGE_SET:
			result = fb_gfx3_gl_page_set(state, command);
			break;
		case FB_GFX3_COMMAND_PRESENT:
			/*
				PAGESET and SCREENCOPY queue asynchronous presentation requests.
				When another command follows in this renderer drain, no BASIC
					caller can observe this intermediate front buffer before the
					batch reaches its ordered completion point.  Retain the final
					visible surface and let the normal end-of-batch presentation
					perform one swap. A synchronous PRESENT is a one-command batch
					and still calls fb_gfx3_gl_present() immediately below.
			*/
			if ((i + 1u < count) &&
			    (command->target == state->visible_surface)) {
				state->presentation_dirty = TRUE;
				result = FB_GFX3_OK;
			} else
				result = fb_gfx3_gl_present(state, command);
			break;
		case FB_GFX3_COMMAND_WINDOW_TITLE:
			result = fb_gfx3_gl_window_title(state, command);
			break;
		case FB_GFX3_COMMAND_BARRIER:
		case FB_GFX3_COMMAND_PLATFORM_POLL:
		case FB_GFX3_COMMAND_INPUT_POLL:
			result = (fb_gfx3_command_payload_size(command) == 0) ?
				FB_GFX3_OK : FB_GFX3_INVALID;
			break;
		default:
			return FB_GFX3_UNSUPPORTED;
		}
		if (result != FB_GFX3_OK) {
			/*
				The renderer may be waiting on a later synchronous command
				when an earlier asynchronous command in its ordered batch
				fails.  Record the actual failing command here so the public
				API error can be traced back to the responsible GPU operation.
			*/
			fb_gfx3_log_write(state->logger, FB_GFX3_LOG_ERROR,
				"OpenGL command %u sequence %llu failed: %d",
				command->type, (unsigned long long)command->sequence, result);
			return result;
		}
		if (opengl_command_writes_visible(state, command))
			state->presentation_dirty = TRUE;
		last_sequence = command->sequence;
		i++;
	}
	if ((count == 1) &&
	    (commands[0]->type == FB_GFX3_COMMAND_INPUT_POLL) &&
	    !state->presentation_dirty) {
		state->submitted_sequence = last_sequence;
		if (submitted_sequence != NULL)
			*submitted_sequence = last_sequence;
		return FB_GFX3_OK;
	}
	if (state->presentation_dirty && (state->visible_surface != 0) &&
	    !defer_page_presentation) {
		result = fb_gfx3_gl_present_handle(state, state->visible_surface,
			last_sequence);
		if (result != FB_GFX3_OK)
			return result;
	}

	if (requires_fence) {
		result = opengl_record_fence(state, last_sequence);
		if (result != FB_GFX3_OK)
			return result;
	} else {
		state->control_sequence = last_sequence;
	}
	state->submitted_sequence = last_sequence;
	if (submitted_sequence != NULL)
		*submitted_sequence = last_sequence;
	return FB_GFX3_OK;
}

static uint64_t opengl_completed_sequence(FB_GFX3_BACKEND *backend)
{
	if ((backend == NULL) || (backend->state == NULL))
		return 0;
	return opengl_poll_completed((FB_GFX3_OPENGL_STATE *)backend->state);
}

static int opengl_wait_sequence(FB_GFX3_BACKEND *backend, uint64_t sequence)
{
	if ((backend == NULL) || (backend->state == NULL))
		return FB_GFX3_INVALID;
	return opengl_wait_for_sequence((FB_GFX3_OPENGL_STATE *)backend->state,
		sequence);
}

static int opengl_wait_idle(FB_GFX3_BACKEND *backend)
{
	FB_GFX3_OPENGL_STATE *state;

	if ((backend == NULL) || (backend->state == NULL))
		return FB_GFX3_INVALID;
	state = (FB_GFX3_OPENGL_STATE *)backend->state;
	state->gl.finish();
	while (state->first_fence != NULL)
		opengl_remove_first_fence(state);
	state->completed_sequence = state->submitted_sequence;
	return fb_gfx3_gl_check_error(state, "wait for idle");
}

static void *opengl_get_opengl_proc(FB_GFX3_BACKEND *backend,
	const char *name)
{
	FB_GFX3_OPENGL_STATE *state;
	void *procedure = NULL;

	if ((backend == NULL) || (backend->state == NULL) || (name == NULL) ||
	    (name[0] == '\0'))
		return NULL;
	state = (FB_GFX3_OPENGL_STATE *)backend->state;
	if ((state->platform_vtable == NULL) ||
	    (state->platform_vtable->load_opengl_function == NULL) ||
	    (state->platform_vtable->load_opengl_function(state->platform, name,
	     (void *)&procedure, sizeof(procedure)) != FB_GFX3_OK))
		return NULL;
	return procedure;
}

#else

/* Unsupported targets keep a linkable vtable for common selection code. */
static int opengl_probe(FB_GFX3_BACKEND_CAPS *caps)
{
	if (caps != NULL)
		memset(caps, 0, sizeof(*caps));
	return FB_GFX3_UNSUPPORTED;
}

static int opengl_init(FB_GFX3_BACKEND *backend,
	const FB_GFX3_BACKEND_CONFIG *config)
{
	(void)backend;
	(void)config;
	return FB_GFX3_UNSUPPORTED;
}

static void opengl_shutdown(FB_GFX3_BACKEND *backend)
{
	if (backend != NULL)
		backend->state = NULL;
}

static int opengl_execute(FB_GFX3_BACKEND *backend,
	FB_GFX3_COMMAND *const *commands, size_t count,
	uint64_t *submitted_sequence)
{
	(void)backend;
	(void)commands;
	(void)count;
	(void)submitted_sequence;
	return FB_GFX3_UNSUPPORTED;
}

static uint64_t opengl_completed_sequence(FB_GFX3_BACKEND *backend)
{
	(void)backend;
	return 0;
}

static int opengl_wait_sequence(FB_GFX3_BACKEND *backend, uint64_t sequence)
{
	(void)backend;
	(void)sequence;
	return FB_GFX3_UNSUPPORTED;
}

static int opengl_wait_idle(FB_GFX3_BACKEND *backend)
{
	(void)backend;
	return FB_GFX3_UNSUPPORTED;
}

static void *opengl_get_opengl_proc(FB_GFX3_BACKEND *backend,
	const char *name)
{
	(void)backend;
	(void)name;
	return NULL;
}

#endif

const FB_GFX3_BACKEND_VTABLE __fb_gfx3_backend_opengl = {
	FB_GFX3_BACKEND_ABI_VERSION,
	"OpenGL 4.3 compute",
	opengl_probe,
	opengl_init,
	opengl_shutdown,
	opengl_execute,
	opengl_completed_sequence,
	opengl_wait_sequence,
	opengl_wait_idle,
	opengl_get_opengl_proc
};

/* end of gfx3_backend_opengl.c */
