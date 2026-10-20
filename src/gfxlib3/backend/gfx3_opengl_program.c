/*
 * FreeBASIC gfxlib3: gfx3_opengl_program.c
 * Purpose: Load OpenGL functions and construct shader programs.
 * Responsibilities: render-thread operations within this module.
 * This file intentionally does NOT contain BASIC compatibility or native input dispatch.
 */

#include "gfx3_opengl_private.h"

#if (defined(HOST_WIN32) || defined(HOST_LINUX)) && !defined(DISABLE_OPENGL)

/* ------------------------------------------------------------------------- */
/* OpenGL function loading                                                   */
/* ------------------------------------------------------------------------- */

#define LOAD_OPENGL_FUNCTION(state, field, type, name) \
	do { \
		if ((state)->platform_vtable->load_opengl_function( \
		    (state)->platform, (name), (void *)&(state)->gl.field, \
		    sizeof((state)->gl.field)) != FB_GFX3_OK) \
			return FB_GFX3_UNSUPPORTED; \
	} while (0)

int fb_gfx3_gl_load_functions(FB_GFX3_OPENGL_STATE *state)
{
	LOAD_OPENGL_FUNCTION(state, get_integer, PFNGLGETINTEGERVPROC,
		"glGetIntegerv");
	LOAD_OPENGL_FUNCTION(state, get_integer_indexed, PFNGLGETINTEGERI_VPROC,
		"glGetIntegeri_v");
	LOAD_OPENGL_FUNCTION(state, get_integer64, PFNGLGETINTEGER64VPROC,
		"glGetInteger64v");
	LOAD_OPENGL_FUNCTION(state, get_string, PFNGLGETSTRINGPROC,
		"glGetString");
	LOAD_OPENGL_FUNCTION(state, get_string_indexed, PFNGLGETSTRINGIPROC,
		"glGetStringi");
	LOAD_OPENGL_FUNCTION(state, get_error, PFNGLGETERRORPROC, "glGetError");
	LOAD_OPENGL_FUNCTION(state, generate_textures, PFNGLGENTEXTURESPROC,
		"glGenTextures");
	LOAD_OPENGL_FUNCTION(state, bind_texture, PFNGLBINDTEXTUREPROC,
		"glBindTexture");
	LOAD_OPENGL_FUNCTION(state, texture_parameter_i, PFNGLTEXPARAMETERIPROC,
		"glTexParameteri");
	LOAD_OPENGL_FUNCTION(state, texture_sub_image_2d,
		PFNGLTEXSUBIMAGE2DPROC, "glTexSubImage2D");
	LOAD_OPENGL_FUNCTION(state, texture_storage_2d, PFNGLTEXSTORAGE2DPROC,
		"glTexStorage2D");
	LOAD_OPENGL_FUNCTION(state, copy_image_sub_data,
		PFNGLCOPYIMAGESUBDATAPROC, "glCopyImageSubData");
	LOAD_OPENGL_FUNCTION(state, delete_textures, PFNGLDELETETEXTURESPROC,
		"glDeleteTextures");
	LOAD_OPENGL_FUNCTION(state, pixel_store_i, PFNGLPIXELSTOREIPROC,
		"glPixelStorei");
	LOAD_OPENGL_FUNCTION(state, generate_framebuffers,
		PFNGLGENFRAMEBUFFERSPROC, "glGenFramebuffers");
	LOAD_OPENGL_FUNCTION(state, bind_framebuffer, PFNGLBINDFRAMEBUFFERPROC,
		"glBindFramebuffer");
	LOAD_OPENGL_FUNCTION(state, framebuffer_texture_2d,
		PFNGLFRAMEBUFFERTEXTURE2DPROC, "glFramebufferTexture2D");
	LOAD_OPENGL_FUNCTION(state, check_framebuffer_status,
		PFNGLCHECKFRAMEBUFFERSTATUSPROC, "glCheckFramebufferStatus");
	LOAD_OPENGL_FUNCTION(state, read_buffer, PFNGLREADBUFFERPROC,
		"glReadBuffer");
	LOAD_OPENGL_FUNCTION(state, read_pixels, PFNGLREADPIXELSPROC,
		"glReadPixels");
	LOAD_OPENGL_FUNCTION(state, delete_framebuffers,
		PFNGLDELETEFRAMEBUFFERSPROC, "glDeleteFramebuffers");
	LOAD_OPENGL_FUNCTION(state, create_shader, PFNGLCREATESHADERPROC,
		"glCreateShader");
	LOAD_OPENGL_FUNCTION(state, shader_source, PFNGLSHADERSOURCEPROC,
		"glShaderSource");
	LOAD_OPENGL_FUNCTION(state, compile_shader, PFNGLCOMPILESHADERPROC,
		"glCompileShader");
	LOAD_OPENGL_FUNCTION(state, get_shader_i, PFNGLGETSHADERIVPROC,
		"glGetShaderiv");
	LOAD_OPENGL_FUNCTION(state, get_shader_log, PFNGLGETSHADERINFOLOGPROC,
		"glGetShaderInfoLog");
	LOAD_OPENGL_FUNCTION(state, delete_shader, PFNGLDELETESHADERPROC,
		"glDeleteShader");
	LOAD_OPENGL_FUNCTION(state, create_program, PFNGLCREATEPROGRAMPROC,
		"glCreateProgram");
	LOAD_OPENGL_FUNCTION(state, attach_shader, PFNGLATTACHSHADERPROC,
		"glAttachShader");
	LOAD_OPENGL_FUNCTION(state, link_program, PFNGLLINKPROGRAMPROC,
		"glLinkProgram");
	LOAD_OPENGL_FUNCTION(state, get_program_i, PFNGLGETPROGRAMIVPROC,
		"glGetProgramiv");
	LOAD_OPENGL_FUNCTION(state, get_program_log, PFNGLGETPROGRAMINFOLOGPROC,
		"glGetProgramInfoLog");
	LOAD_OPENGL_FUNCTION(state, delete_program, PFNGLDELETEPROGRAMPROC,
		"glDeleteProgram");
	LOAD_OPENGL_FUNCTION(state, use_program, PFNGLUSEPROGRAMPROC,
		"glUseProgram");
	LOAD_OPENGL_FUNCTION(state, get_uniform_location,
		PFNGLGETUNIFORMLOCATIONPROC, "glGetUniformLocation");
	LOAD_OPENGL_FUNCTION(state, uniform_1ui, PFNGLUNIFORM1UIPROC,
		"glUniform1ui");
	LOAD_OPENGL_FUNCTION(state, uniform_1uiv, PFNGLUNIFORM1UIVPROC,
		"glUniform1uiv");
	LOAD_OPENGL_FUNCTION(state, uniform_1i, PFNGLUNIFORM1IPROC,
		"glUniform1i");
	LOAD_OPENGL_FUNCTION(state, uniform_2f, PFNGLUNIFORM2FPROC,
		"glUniform2f");
	LOAD_OPENGL_FUNCTION(state, uniform_4i, PFNGLUNIFORM4IPROC,
		"glUniform4i");
	LOAD_OPENGL_FUNCTION(state, uniform_matrix_3fv,
		PFNGLUNIFORMMATRIX3FVPROC, "glUniformMatrix3fv");
	LOAD_OPENGL_FUNCTION(state, bind_image_texture,
		PFNGLBINDIMAGETEXTUREPROC, "glBindImageTexture");
	LOAD_OPENGL_FUNCTION(state, dispatch_compute, PFNGLDISPATCHCOMPUTEPROC,
		"glDispatchCompute");
	LOAD_OPENGL_FUNCTION(state, memory_barrier, PFNGLMEMORYBARRIERPROC,
		"glMemoryBarrier");
	LOAD_OPENGL_FUNCTION(state, generate_buffers, PFNGLGENBUFFERSPROC,
		"glGenBuffers");
	LOAD_OPENGL_FUNCTION(state, bind_buffer, PFNGLBINDBUFFERPROC,
		"glBindBuffer");
	LOAD_OPENGL_FUNCTION(state, buffer_data, PFNGLBUFFERDATAPROC,
		"glBufferData");
	LOAD_OPENGL_FUNCTION(state, bind_buffer_base, PFNGLBINDBUFFERBASEPROC,
		"glBindBufferBase");
	LOAD_OPENGL_FUNCTION(state, delete_buffers, PFNGLDELETEBUFFERSPROC,
		"glDeleteBuffers");
	LOAD_OPENGL_FUNCTION(state, generate_vertex_arrays,
		PFNGLGENVERTEXARRAYSPROC, "glGenVertexArrays");
	LOAD_OPENGL_FUNCTION(state, bind_vertex_array,
		PFNGLBINDVERTEXARRAYPROC, "glBindVertexArray");
	LOAD_OPENGL_FUNCTION(state, delete_vertex_arrays,
		PFNGLDELETEVERTEXARRAYSPROC, "glDeleteVertexArrays");
	LOAD_OPENGL_FUNCTION(state, enable_vertex_attrib_array,
		PFNGLENABLEVERTEXATTRIBARRAYPROC, "glEnableVertexAttribArray");
	LOAD_OPENGL_FUNCTION(state, disable_vertex_attrib_array,
		PFNGLDISABLEVERTEXATTRIBARRAYPROC, "glDisableVertexAttribArray");
	LOAD_OPENGL_FUNCTION(state, vertex_attrib_i_pointer,
		PFNGLVERTEXATTRIBIPOINTERPROC, "glVertexAttribIPointer");
	LOAD_OPENGL_FUNCTION(state, vertex_attrib_divisor,
		PFNGLVERTEXATTRIBDIVISORPROC, "glVertexAttribDivisor");
	LOAD_OPENGL_FUNCTION(state, draw_arrays_instanced,
		PFNGLDRAWARRAYSINSTANCEDPROC, "glDrawArraysInstanced");
	LOAD_OPENGL_FUNCTION(state, active_texture, PFNGLACTIVETEXTUREPROC,
		"glActiveTexture");
	LOAD_OPENGL_FUNCTION(state, enable, FB_GFX3_GL_ENABLE, "glEnable");
	LOAD_OPENGL_FUNCTION(state, disable, FB_GFX3_GL_ENABLE, "glDisable");
	LOAD_OPENGL_FUNCTION(state, logic_op, FB_GFX3_GL_LOGIC_OP, "glLogicOp");
	LOAD_OPENGL_FUNCTION(state, viewport, PFNGLVIEWPORTPROC, "glViewport");
	LOAD_OPENGL_FUNCTION(state, clear_color, FB_GFX3_GL_CLEAR_COLOR,
		"glClearColor");
	LOAD_OPENGL_FUNCTION(state, clear, FB_GFX3_GL_CLEAR, "glClear");
	LOAD_OPENGL_FUNCTION(state, draw_arrays, PFNGLDRAWARRAYSPROC,
		"glDrawArrays");
	LOAD_OPENGL_FUNCTION(state, fence_sync, PFNGLFENCESYNCPROC,
		"glFenceSync");
	LOAD_OPENGL_FUNCTION(state, client_wait_sync, PFNGLCLIENTWAITSYNCPROC,
		"glClientWaitSync");
	LOAD_OPENGL_FUNCTION(state, delete_sync, PFNGLDELETESYNCPROC,
		"glDeleteSync");
	LOAD_OPENGL_FUNCTION(state, finish, PFNGLFINISHPROC, "glFinish");
	return FB_GFX3_OK;
}

#undef LOAD_OPENGL_FUNCTION

/* ------------------------------------------------------------------------- */
/* Compute program setup                                                     */
/* ------------------------------------------------------------------------- */

#include "gfx3_opengl_shaders.h"

static void opengl_report_shader_log(FB_GFX3_OPENGL_STATE *state,
	GLuint object, int program, const char *description)
{
	char log[FB_GFX3_LOG_MESSAGE_SIZE];
	GLsizei length = 0;

	memset(log, 0, sizeof(log));
	if (program) {
		state->gl.get_program_log(object, (GLsizei)sizeof(log) - 1,
			&length, log);
	} else {
		state->gl.get_shader_log(object, (GLsizei)sizeof(log) - 1,
			&length, log);
	}
	if (length > 0)
		fb_gfx3_log_write(state->logger, FB_GFX3_LOG_ERROR,
			"OpenGL %s: %s", description, log);
}

static int opengl_create_compute_program(FB_GFX3_OPENGL_STATE *state,
	const char *source, const char *description, GLuint *program_result)
{
	GLuint shader;
	GLuint program;
	GLint status;

	shader = state->gl.create_shader(GL_COMPUTE_SHADER);
	if (shader == 0)
		return FB_GFX3_FAILED;
	state->gl.shader_source(shader, 1, &source, NULL);
	state->gl.compile_shader(shader);
	state->gl.get_shader_i(shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		opengl_report_shader_log(state, shader, FALSE, description);
		state->gl.delete_shader(shader);
		return FB_GFX3_FAILED;
	}

	program = state->gl.create_program();
	if (program == 0) {
		state->gl.delete_shader(shader);
		return FB_GFX3_FAILED;
	}
	state->gl.attach_shader(program, shader);
	state->gl.link_program(program);
	state->gl.delete_shader(shader);
	state->gl.get_program_i(program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		opengl_report_shader_log(state, program, TRUE, description);
		state->gl.delete_program(program);
		return FB_GFX3_FAILED;
	}

	*program_result = program;
	return FB_GFX3_OK;
}

static int opengl_create_graphics_program(FB_GFX3_OPENGL_STATE *state,
	const char *vertex_source, const char *fragment_source,
	const char *description, GLuint *program_result)
{
	GLuint vertex_shader = 0;
	GLuint fragment_shader = 0;
	GLuint program = 0;
	GLint status;
	int result = FB_GFX3_FAILED;

	vertex_shader = state->gl.create_shader(GL_VERTEX_SHADER);
	fragment_shader = state->gl.create_shader(GL_FRAGMENT_SHADER);
	if ((vertex_shader == 0) || (fragment_shader == 0))
		goto done;
	state->gl.shader_source(vertex_shader, 1, &vertex_source, NULL);
	state->gl.compile_shader(vertex_shader);
	state->gl.get_shader_i(vertex_shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		opengl_report_shader_log(state, vertex_shader, FALSE, description);
		goto done;
	}
	state->gl.shader_source(fragment_shader, 1, &fragment_source, NULL);
	state->gl.compile_shader(fragment_shader);
	state->gl.get_shader_i(fragment_shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		opengl_report_shader_log(state, fragment_shader, FALSE, description);
		goto done;
	}
	program = state->gl.create_program();
	if (program == 0)
		goto done;
	state->gl.attach_shader(program, vertex_shader);
	state->gl.attach_shader(program, fragment_shader);
	state->gl.link_program(program);
	state->gl.get_program_i(program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		opengl_report_shader_log(state, program, TRUE, description);
		goto done;
	}
	*program_result = program;
	program = 0;
	result = FB_GFX3_OK;

done:
	if (program != 0)
		state->gl.delete_program(program);
	if (fragment_shader != 0)
		state->gl.delete_shader(fragment_shader);
	if (vertex_shader != 0)
		state->gl.delete_shader(vertex_shader);
	return result;
}

int fb_gfx3_gl_create_programs(FB_GFX3_OPENGL_STATE *state)
{
	int result;

	result = opengl_create_compute_program(state, opengl_clear_shader,
		"clear compute shader", &state->clear_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state, opengl_points_shader,
		"point compute shader", &state->points_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state, opengl_points_batch_shader,
		"point batch selection compute shader", &state->points_batch_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state,
		opengl_points_batch_resolve_shader,
		"point batch resolve compute shader", &state->points_batch_resolve_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state, opengl_glyph_batch_shader,
		"ordered glyph tile compute shader", &state->glyph_batch_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state, opengl_line_shader,
		"line compute shader", &state->line_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state, opengl_line_batch_shader,
		"line batch selection compute shader", &state->line_batch_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state,
		opengl_line_batch_resolve_shader, "line batch resolve compute shader",
		&state->line_batch_resolve_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state, opengl_rectangle_shader,
		"rectangle compute shader", &state->rectangle_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state, opengl_ellipse_shader,
		"ellipse compute shader", &state->ellipse_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state, opengl_ellipse_batch_shader,
		"ellipse batch selection compute shader", &state->ellipse_batch_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state,
		opengl_ellipse_batch_resolve_shader,
		"ellipse batch resolve compute shader",
		&state->ellipse_batch_resolve_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state,
		opengl_primitive_batch_shader,
		"mixed primitive selection compute shader",
		&state->primitive_batch_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state,
		opengl_primitive_batch_resolve_shader,
		"mixed primitive resolve compute shader",
		&state->primitive_batch_resolve_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state, opengl_paint_shader,
		"scanline paint compute shader", &state->paint_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state, opengl_blit_shader,
		"blit compute shader", &state->blit_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state, opengl_transform_blit_shader,
		"transform blit compute shader", &state->transform_blit_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state, opengl_blit_alpha_tile_shader,
		"alpha tile blit compute shader", &state->blit_alpha_tile_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state, opengl_blit_batch_shader,
		"blit batch compute shader", &state->blit_batch_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state,
		opengl_blit_batch_resolve_shader, "blit batch resolve compute shader",
		&state->blit_batch_resolve_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_graphics_program(state,
		opengl_blit_raster_batch_vertex_shader,
		opengl_blit_raster_batch_fragment_shader,
		"ordered blit raster batch shaders", &state->blit_raster_batch_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_graphics_program(state,
		opengl_rectangle_raster_batch_vertex_shader,
		opengl_rectangle_raster_batch_fragment_shader,
		"ordered rectangle raster batch shaders",
		&state->rectangle_raster_batch_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state, opengl_rectangle_batch_shader,
		"rectangle batch compute shader", &state->rectangle_batch_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_compute_program(state,
		opengl_rectangle_batch_resolve_shader,
		"rectangle batch resolve compute shader",
		&state->rectangle_batch_resolve_program);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_create_graphics_program(state,
		opengl_present_vertex_shader, opengl_present_fragment_shader,
		"presentation shaders", &state->present_program);
	if (result != FB_GFX3_OK)
		return result;

	state->clear_rect_location = state->gl.get_uniform_location(
		state->clear_program, "operation_rect");
	state->clear_color_location = state->gl.get_uniform_location(
		state->clear_program, "operation_color");
	state->clear_mask_location = state->gl.get_uniform_location(
		state->clear_program, "operation_mask");
	state->clear_flags_location = state->gl.get_uniform_location(
		state->clear_program, "operation_flags");
	state->points_clip_location = state->gl.get_uniform_location(
		state->points_program, "operation_clip");
	state->points_count_location = state->gl.get_uniform_location(
		state->points_program, "operation_count");
	state->points_mask_location = state->gl.get_uniform_location(
		state->points_program, "operation_mask");
	state->points_batch_clip_location = state->gl.get_uniform_location(
		state->points_batch_program, "operation_clip");
	state->points_batch_count_location = state->gl.get_uniform_location(
		state->points_batch_program, "operation_count");
	state->points_batch_key_location = state->gl.get_uniform_location(
		state->points_batch_program, "operation_batch_key");
	state->points_batch_resolve_rect_location = state->gl.get_uniform_location(
		state->points_batch_resolve_program, "operation_resolve_rect");
	state->points_batch_resolve_count_location = state->gl.get_uniform_location(
		state->points_batch_resolve_program, "operation_count");
	state->points_batch_resolve_generation_location =
		state->gl.get_uniform_location(state->points_batch_resolve_program,
			"operation_batch_generation");
	state->points_batch_resolve_mask_location = state->gl.get_uniform_location(
		state->points_batch_resolve_program, "operation_mask");
	state->glyph_batch_clip_location = state->gl.get_uniform_location(
		state->glyph_batch_program, "operation_clip");
	state->glyph_batch_count_location = state->gl.get_uniform_location(
		state->glyph_batch_program, "operation_count");
	state->glyph_batch_tile_origin_location = state->gl.get_uniform_location(
		state->glyph_batch_program, "operation_tile_origin");
	state->glyph_batch_tiles_x_location = state->gl.get_uniform_location(
		state->glyph_batch_program, "operation_tiles_x");
	state->glyph_batch_mask_location = state->gl.get_uniform_location(
		state->glyph_batch_program, "operation_mask");
	state->line_endpoints_location = state->gl.get_uniform_location(
		state->line_program, "operation_endpoints");
	state->line_clip_location = state->gl.get_uniform_location(
		state->line_program, "operation_clip");
	state->line_count_location = state->gl.get_uniform_location(
		state->line_program, "operation_count");
	state->line_color_location = state->gl.get_uniform_location(
		state->line_program, "operation_color");
	state->line_style_location = state->gl.get_uniform_location(
		state->line_program, "operation_style");
	state->line_mask_location = state->gl.get_uniform_location(
		state->line_program, "operation_mask");
	state->line_flags_location = state->gl.get_uniform_location(
		state->line_program, "operation_flags");
	state->line_batch_clip_location = state->gl.get_uniform_location(
		state->line_batch_program, "operation_clip");
	state->line_batch_style_location = state->gl.get_uniform_location(
		state->line_batch_program, "operation_style");
	state->line_batch_key_location = state->gl.get_uniform_location(
		state->line_batch_program, "operation_batch_key");
	state->line_batch_resolve_rect_location = state->gl.get_uniform_location(
		state->line_batch_resolve_program, "operation_resolve_rect");
	state->line_batch_resolve_generation_location =
		state->gl.get_uniform_location(state->line_batch_resolve_program,
			"operation_batch_generation");
	state->line_batch_resolve_mask_location = state->gl.get_uniform_location(
		state->line_batch_resolve_program, "operation_mask");
	state->rectangle_box_location = state->gl.get_uniform_location(
		state->rectangle_program, "operation_box");
	state->rectangle_clip_location = state->gl.get_uniform_location(
		state->rectangle_program, "operation_clip");
	state->rectangle_width_location = state->gl.get_uniform_location(
		state->rectangle_program, "operation_width");
	state->rectangle_height_location = state->gl.get_uniform_location(
		state->rectangle_program, "operation_height");
	state->rectangle_count_location = state->gl.get_uniform_location(
		state->rectangle_program, "operation_count");
	state->rectangle_color_location = state->gl.get_uniform_location(
		state->rectangle_program, "operation_color");
	state->rectangle_style_location = state->gl.get_uniform_location(
		state->rectangle_program, "operation_style");
	state->rectangle_mask_location = state->gl.get_uniform_location(
		state->rectangle_program, "operation_mask");
	state->rectangle_filled_location = state->gl.get_uniform_location(
		state->rectangle_program, "operation_filled");
	state->rectangle_flags_location = state->gl.get_uniform_location(
		state->rectangle_program, "operation_flags");
	state->ellipse_center_location = state->gl.get_uniform_location(
		state->ellipse_program, "operation_center");
	state->ellipse_clip_location = state->gl.get_uniform_location(
		state->ellipse_program, "operation_clip");
	state->ellipse_radii_location = state->gl.get_uniform_location(
		state->ellipse_program, "operation_radii");
	state->ellipse_color_location = state->gl.get_uniform_location(
		state->ellipse_program, "operation_color");
	state->ellipse_filled_location = state->gl.get_uniform_location(
		state->ellipse_program, "operation_filled");
	state->ellipse_mask_location = state->gl.get_uniform_location(
		state->ellipse_program, "operation_mask");
	state->ellipse_flags_location = state->gl.get_uniform_location(
		state->ellipse_program, "operation_flags");
	state->ellipse_batch_key_location = state->gl.get_uniform_location(
		state->ellipse_batch_program, "operation_batch_key");
	state->ellipse_batch_resolve_rect_location = state->gl.get_uniform_location(
		state->ellipse_batch_resolve_program, "operation_resolve_rect");
	state->ellipse_batch_resolve_generation_location =
		state->gl.get_uniform_location(state->ellipse_batch_resolve_program,
			"operation_batch_generation");
	state->ellipse_batch_resolve_mask_location = state->gl.get_uniform_location(
		state->ellipse_batch_resolve_program, "operation_mask");
	state->primitive_batch_key_location = state->gl.get_uniform_location(
		state->primitive_batch_program, "operation_batch_key");
	state->primitive_batch_resolve_rect_location =
		state->gl.get_uniform_location(state->primitive_batch_resolve_program,
			"operation_resolve_rect");
	state->primitive_batch_resolve_generation_location =
		state->gl.get_uniform_location(state->primitive_batch_resolve_program,
			"operation_batch_generation");
	state->primitive_batch_resolve_mask_location =
		state->gl.get_uniform_location(state->primitive_batch_resolve_program,
			"operation_mask");
	state->paint_seed_location = state->gl.get_uniform_location(
		state->paint_program, "operation_seed");
	state->paint_clip_location = state->gl.get_uniform_location(
		state->paint_program, "operation_clip");
	state->paint_color_location = state->gl.get_uniform_location(
		state->paint_program, "operation_color");
	state->paint_border_location = state->gl.get_uniform_location(
		state->paint_program, "operation_border");
	state->paint_mask_location = state->gl.get_uniform_location(
		state->paint_program, "operation_mask");
	state->paint_flags_location = state->gl.get_uniform_location(
		state->paint_program, "operation_flags");
	state->paint_mode_location = state->gl.get_uniform_location(
		state->paint_program, "operation_mode");
	state->paint_pattern_size_location = state->gl.get_uniform_location(
		state->paint_program, "operation_pattern_size");
	state->paint_pattern_origin_location = state->gl.get_uniform_location(
		state->paint_program, "operation_pattern_origin");
	state->paint_bytes_per_pixel_location = state->gl.get_uniform_location(
		state->paint_program, "operation_bytes_per_pixel");
	state->paint_pattern_location = state->gl.get_uniform_location(
		state->paint_program, "operation_pattern[0]");
	state->paint_phase_location = state->gl.get_uniform_location(
		state->paint_program, "operation_phase");
	state->blit_source_rect_location = state->gl.get_uniform_location(
		state->blit_program, "operation_source_rect");
	state->blit_clip_location = state->gl.get_uniform_location(
		state->blit_program, "operation_clip");
	state->blit_destination_x_location = state->gl.get_uniform_location(
		state->blit_program, "operation_destination_x");
	state->blit_destination_y_location = state->gl.get_uniform_location(
		state->blit_program, "operation_destination_y");
	state->blit_mode_location = state->gl.get_uniform_location(
		state->blit_program, "operation_mode");
	state->blit_alpha_location = state->gl.get_uniform_location(
		state->blit_program, "operation_alpha");
	state->blit_depth_location = state->gl.get_uniform_location(
		state->blit_program, "operation_depth");
	state->blit_mask_location = state->gl.get_uniform_location(
		state->blit_program, "operation_mask");
	state->transform_blit_source_rect_location =
		state->gl.get_uniform_location(state->transform_blit_program,
			"operation_source_rect");
	state->transform_blit_clip_location = state->gl.get_uniform_location(
		state->transform_blit_program, "operation_clip");
	state->transform_blit_bounds_location = state->gl.get_uniform_location(
		state->transform_blit_program, "operation_bounds");
	state->transform_blit_inverse_location = state->gl.get_uniform_location(
		state->transform_blit_program, "operation_inverse");
	state->transform_blit_mode_location = state->gl.get_uniform_location(
		state->transform_blit_program, "operation_mode");
	state->transform_blit_alpha_location = state->gl.get_uniform_location(
		state->transform_blit_program, "operation_alpha");
	state->transform_blit_depth_location = state->gl.get_uniform_location(
		state->transform_blit_program, "operation_depth");
	state->transform_blit_mask_location = state->gl.get_uniform_location(
		state->transform_blit_program, "operation_mask");
	state->transform_blit_filter_location = state->gl.get_uniform_location(
		state->transform_blit_program, "operation_filter");
	state->transform_blit_wrap_location = state->gl.get_uniform_location(
		state->transform_blit_program, "operation_wrap");
	state->blit_alpha_tile_count_location = state->gl.get_uniform_location(
		state->blit_alpha_tile_program, "operation_tiles_x");
	state->blit_alpha_tile_mode_location = state->gl.get_uniform_location(
		state->blit_alpha_tile_program, "operation_mode");
	state->blit_alpha_tile_alpha_location = state->gl.get_uniform_location(
		state->blit_alpha_tile_program, "operation_alpha");
	state->blit_alpha_tile_depth_location = state->gl.get_uniform_location(
		state->blit_alpha_tile_program, "operation_depth");
	state->blit_alpha_tile_mask_location = state->gl.get_uniform_location(
		state->blit_alpha_tile_program, "operation_mask");
	state->blit_batch_mode_location = state->gl.get_uniform_location(
		state->blit_batch_program, "operation_mode");
	state->blit_batch_depth_location = state->gl.get_uniform_location(
		state->blit_batch_program, "operation_depth");
	state->blit_batch_mask_location = state->gl.get_uniform_location(
		state->blit_batch_program, "operation_mask");
	state->blit_batch_key_location = state->gl.get_uniform_location(
		state->blit_batch_program, "operation_batch_key");
	state->blit_batch_resolve_rect_location = state->gl.get_uniform_location(
		state->blit_batch_resolve_program, "operation_resolve_rect");
	state->blit_batch_resolve_generation_location =
		state->gl.get_uniform_location(state->blit_batch_resolve_program,
			"operation_batch_generation");
	state->blit_batch_resolve_mask_location = state->gl.get_uniform_location(
		state->blit_batch_resolve_program, "operation_mask");
	state->blit_raster_batch_source_location = state->gl.get_uniform_location(
		state->blit_raster_batch_program, "source_image");
	state->blit_raster_batch_size_location = state->gl.get_uniform_location(
		state->blit_raster_batch_program, "operation_surface_size");
	state->blit_raster_batch_mode_location = state->gl.get_uniform_location(
		state->blit_raster_batch_program, "operation_mode");
	state->blit_raster_batch_depth_location = state->gl.get_uniform_location(
		state->blit_raster_batch_program, "operation_depth");
	state->blit_raster_batch_mask_location = state->gl.get_uniform_location(
		state->blit_raster_batch_program, "operation_mask");
	state->rectangle_raster_batch_size_location = state->gl.get_uniform_location(
		state->rectangle_raster_batch_program, "operation_surface_size");
	state->rectangle_raster_batch_mask_location = state->gl.get_uniform_location(
		state->rectangle_raster_batch_program, "operation_mask");
	state->rectangle_batch_key_location = state->gl.get_uniform_location(
		state->rectangle_batch_program, "operation_batch_key");
	state->rectangle_batch_resolve_rect_location =
		state->gl.get_uniform_location(state->rectangle_batch_resolve_program,
			"operation_resolve_rect");
	state->rectangle_batch_resolve_generation_location =
		state->gl.get_uniform_location(state->rectangle_batch_resolve_program,
			"operation_batch_generation");
	state->rectangle_batch_resolve_mask_location =
		state->gl.get_uniform_location(state->rectangle_batch_resolve_program,
			"operation_mask");
	state->present_source_location = state->gl.get_uniform_location(
		state->present_program, "source_image");
	state->present_depth_location = state->gl.get_uniform_location(
		state->present_program, "operation_depth");
	state->present_palette_location = state->gl.get_uniform_location(
		state->present_program, "operation_palette[0]");
	if ((state->clear_rect_location < 0) ||
	    (state->clear_color_location < 0) ||
	    (state->clear_mask_location < 0) ||
	    (state->clear_flags_location < 0) ||
	    (state->points_clip_location < 0) ||
	    (state->points_count_location < 0) ||
	    (state->points_mask_location < 0) ||
	    (state->points_batch_clip_location < 0) ||
	    (state->points_batch_count_location < 0) ||
	    (state->points_batch_key_location < 0) ||
	    (state->points_batch_resolve_rect_location < 0) ||
	    (state->points_batch_resolve_count_location < 0) ||
	    (state->points_batch_resolve_generation_location < 0) ||
	    (state->points_batch_resolve_mask_location < 0) ||
	    (state->glyph_batch_clip_location < 0) ||
	    (state->glyph_batch_count_location < 0) ||
	    (state->glyph_batch_tile_origin_location < 0) ||
	    (state->glyph_batch_tiles_x_location < 0) ||
	    (state->glyph_batch_mask_location < 0) ||
	    (state->line_endpoints_location < 0) ||
	    (state->line_clip_location < 0) ||
	    (state->line_count_location < 0) ||
	    (state->line_color_location < 0) ||
	    (state->line_style_location < 0) ||
	    (state->line_mask_location < 0) ||
	    (state->line_flags_location < 0) ||
	    (state->line_batch_clip_location < 0) ||
	    (state->line_batch_style_location < 0) ||
	    (state->line_batch_key_location < 0) ||
	    (state->line_batch_resolve_rect_location < 0) ||
	    (state->line_batch_resolve_generation_location < 0) ||
	    (state->line_batch_resolve_mask_location < 0) ||
	    (state->rectangle_box_location < 0) ||
	    (state->rectangle_clip_location < 0) ||
	    (state->rectangle_width_location < 0) ||
	    (state->rectangle_height_location < 0) ||
	    (state->rectangle_count_location < 0) ||
	    (state->rectangle_color_location < 0) ||
	    (state->rectangle_style_location < 0) ||
	    (state->rectangle_mask_location < 0) ||
	    (state->rectangle_filled_location < 0) ||
	    (state->rectangle_flags_location < 0) ||
	    (state->ellipse_center_location < 0) ||
	    (state->ellipse_clip_location < 0) ||
	    (state->ellipse_radii_location < 0) ||
	    (state->ellipse_color_location < 0) ||
	    (state->ellipse_filled_location < 0) ||
	    (state->ellipse_mask_location < 0) ||
	    (state->ellipse_flags_location < 0) ||
	    (state->ellipse_batch_key_location < 0) ||
	    (state->ellipse_batch_resolve_rect_location < 0) ||
	    (state->ellipse_batch_resolve_generation_location < 0) ||
	    (state->ellipse_batch_resolve_mask_location < 0) ||
	    (state->paint_seed_location < 0) ||
	    (state->paint_clip_location < 0) ||
	    (state->paint_color_location < 0) ||
	    (state->paint_border_location < 0) ||
	    (state->paint_mask_location < 0) ||
	    (state->paint_flags_location < 0) ||
	    (state->paint_mode_location < 0) ||
	    (state->paint_pattern_size_location < 0) ||
	    (state->paint_bytes_per_pixel_location < 0) ||
	    (state->paint_pattern_location < 0) ||
	    (state->paint_phase_location < 0) ||
	    (state->blit_source_rect_location < 0) ||
	    (state->blit_clip_location < 0) ||
	    (state->blit_destination_x_location < 0) ||
	    (state->blit_destination_y_location < 0) ||
	    (state->blit_mode_location < 0) ||
	    (state->blit_alpha_location < 0) ||
	    (state->blit_depth_location < 0) ||
	    (state->blit_mask_location < 0) ||
	    (state->transform_blit_source_rect_location < 0) ||
	    (state->transform_blit_clip_location < 0) ||
	    (state->transform_blit_bounds_location < 0) ||
	    (state->transform_blit_inverse_location < 0) ||
	    (state->transform_blit_mode_location < 0) ||
	    (state->transform_blit_alpha_location < 0) ||
	    (state->transform_blit_depth_location < 0) ||
	    (state->transform_blit_mask_location < 0) ||
	    (state->transform_blit_filter_location < 0) ||
	    (state->transform_blit_wrap_location < 0) ||
	    (state->blit_alpha_tile_count_location < 0) ||
	    (state->blit_alpha_tile_mode_location < 0) ||
	    (state->blit_alpha_tile_alpha_location < 0) ||
	    (state->blit_alpha_tile_depth_location < 0) ||
	    (state->blit_alpha_tile_mask_location < 0) ||
	    (state->blit_batch_mode_location < 0) ||
	    (state->blit_batch_depth_location < 0) ||
	    (state->blit_batch_mask_location < 0) ||
	    (state->blit_batch_key_location < 0) ||
	    (state->blit_batch_resolve_rect_location < 0) ||
	    (state->blit_batch_resolve_generation_location < 0) ||
	    (state->blit_batch_resolve_mask_location < 0) ||
	    (state->blit_raster_batch_source_location < 0) ||
	    (state->blit_raster_batch_size_location < 0) ||
	    (state->blit_raster_batch_mode_location < 0) ||
	    (state->blit_raster_batch_depth_location < 0) ||
	    (state->blit_raster_batch_mask_location < 0) ||
	    (state->rectangle_raster_batch_size_location < 0) ||
	    (state->rectangle_raster_batch_mask_location < 0) ||
	    (state->rectangle_batch_key_location < 0) ||
	    (state->rectangle_batch_resolve_rect_location < 0) ||
	    (state->rectangle_batch_resolve_generation_location < 0) ||
	    (state->rectangle_batch_resolve_mask_location < 0) ||
	    (state->present_source_location < 0) ||
	    (state->present_depth_location < 0) ||
	    (state->present_palette_location < 0))
		return FB_GFX3_FAILED;
	return FB_GFX3_OK;
}

#endif

/* end of gfx3_opengl_program.c */
