/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_backend_opengl.c

    Purpose:

        Execute gfxlib3 surface commands with OpenGL integer textures and
        compute shaders owned by the render thread.

	Responsibilities:

		- use a platform adapter's render-thread-owned OpenGL context
		- load required OpenGL functions without a permanent loader dependency
		- allocate GPU-only integer surfaces
		- execute clear, point, line, box, ellipse, and PUT on the GPU
		- transfer CPU image rows only at explicit compatibility barriers
		- present a selected logical surface through a fullscreen shader
        - track asynchronous completion with GPU fence objects

    This file intentionally does NOT contain:

        - FreeBASIC coordinate or QB language handling
		- native window creation, WGL policy, or input events
        - CPU reference raster algorithms
*/

#include "gfx3_backend_opengl.h"
#include "gfx3_debug.h"
#include "gfx3_platform.h"
#include "gfx3_protocol.h"
#include "gfx3_resource.h"

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

#define FB_GFX3_OPENGL_LOCAL_SIZE_X 16u
#define FB_GFX3_OPENGL_LOCAL_SIZE_Y 16u
#define FB_GFX3_OPENGL_POINT_GROUP_SIZE 64u
/*
	The lower thirteen bits of a winner entry identify one ordered command in
	this batch. This permits one complete 6,000-line primitive fixture to reach
	the ordered winner/resolve path without fragmenting into six full resolves.
	The remaining bits hold a generation tag so an old winner texture value can
	never be mistaken for a current frame.  This matches the renderer's bounded
	8192-command drain and avoids repeated whole-frame resolve passes.
*/
/*
	The raster batch does not encode an item index in a winner texture. Four
	ordinary 1,024-sprite frames therefore share one instance upload and draw
	without weakening the thirteen-bit limits used by the compute winner paths.
*/
#define FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT 4096u
/* Filled boxes use ordinary framebuffer raster order, not winner tags. */
#define FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT 8192u
/*
	Sprite batches use one workgroup per 16 by 16 destination tile.  Every
	invocation replays its tile's commands in BASIC submission order, so this
	remains exact for source-over and Boolean PUT modes as well as opaque PUT.
*/
#define FB_GFX3_OPENGL_ALPHA_TILE_MAX_TILES 65536u
/* Opaque ordered lines use the same thirteen-bit winner-tag representation. */
#define FB_GFX3_OPENGL_LINE_BATCH_LIMIT 8191u
/* Opaque point-command runs use the same thirteen-bit winner-tag representation. */
#define FB_GFX3_OPENGL_POINTS_BATCH_LIMIT 8191u
/* Bound per-pixel tile replay when console output repeatedly overwrites cells. */
#define FB_GFX3_OPENGL_GLYPH_BATCH_LIMIT 8191u
#define FB_GFX3_OPENGL_GLYPH_TILE_SIZE 8u
/* Filled ellipse batches use the same command-index range as other winner paths. */
#define FB_GFX3_OPENGL_ELLIPSE_BATCH_LIMIT 8191u
/* Mixed primitive records use these stable shader-visible type identifiers. */
#define FB_GFX3_OPENGL_PRIMITIVE_LINE 1u
#define FB_GFX3_OPENGL_PRIMITIVE_ELLIPSE 2u
#define FB_GFX3_OPENGL_PRIMITIVE_POINT 3u
/*
	One workgroup owns the exact flood queue, so retain a conservative limit.
	Lane zero discovers spans in gfxlib2 order while the full workgroup writes
	each span. Above this size the public front end uses the bounded CPU
	compatibility path instead of risking an excessive dispatch watchdog interval.
*/
#define FB_GFX3_OPENGL_PAINT_MAX_PIXELS 1048576u
/* FreeBASIC page numbers occupy one byte in SCREENSET's packed return value. */
#define FB_GFX3_OPENGL_PAGE_CONTENT_LIMIT 256u
/* Match the renderer drain when eliminating overwritten full-page copies. */
#define FB_GFX3_OPENGL_PAGE_COPY_BATCH_LIMIT 1024u

typedef struct FB_GFX3_OPENGL_PAGE_COPY_DESCRIPTION {
	FB_GFX3_HANDLE source;
	FB_GFX3_RECT clip;
	FB_GFX3_RECT source_rect;
	int32_t destination_x;
	int32_t destination_y;
} FB_GFX3_OPENGL_PAGE_COPY_DESCRIPTION;

/* OpenGL 1.1 headers expose these as functions but not pointer typedefs. */
typedef void (APIENTRY *FB_GFX3_GL_GET_INTEGER)(GLenum name, GLint *value);
typedef const GLubyte *(APIENTRY *FB_GFX3_GL_GET_STRING)(GLenum name);
typedef GLenum (APIENTRY *FB_GFX3_GL_GET_ERROR)(void);
typedef void (APIENTRY *FB_GFX3_GL_GENERATE_TEXTURES)(GLsizei count,
	GLuint *textures);
typedef void (APIENTRY *FB_GFX3_GL_BIND_TEXTURE)(GLenum target,
	GLuint texture);
typedef void (APIENTRY *FB_GFX3_GL_TEXTURE_PARAMETER_I)(GLenum target,
	GLenum name, GLint value);
typedef void (APIENTRY *FB_GFX3_GL_TEXTURE_SUB_IMAGE_2D)(GLenum target,
	GLint level, GLint x, GLint y, GLsizei width, GLsizei height,
	GLenum format, GLenum type, const void *pixels);
typedef void (APIENTRY *FB_GFX3_GL_DELETE_TEXTURES)(GLsizei count,
	const GLuint *textures);
typedef void (APIENTRY *FB_GFX3_GL_PIXEL_STORE_I)(GLenum name, GLint value);
typedef void (APIENTRY *FB_GFX3_GL_READ_BUFFER)(GLenum buffer);
typedef void (APIENTRY *FB_GFX3_GL_READ_PIXELS)(GLint x, GLint y,
	GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
typedef void (APIENTRY *FB_GFX3_GL_FINISH)(void);
typedef void (APIENTRY *FB_GFX3_GL_VIEWPORT)(GLint x, GLint y,
	GLsizei width, GLsizei height);
typedef void (APIENTRY *FB_GFX3_GL_CLEAR_COLOR)(GLfloat red, GLfloat green,
	GLfloat blue, GLfloat alpha);
typedef void (APIENTRY *FB_GFX3_GL_CLEAR)(GLbitfield mask);
typedef void (APIENTRY *FB_GFX3_GL_DRAW_ARRAYS)(GLenum mode, GLint first,
	GLsizei count);
typedef void (APIENTRY *FB_GFX3_GL_ENABLE)(GLenum capability);
typedef void (APIENTRY *FB_GFX3_GL_LOGIC_OP)(GLenum operation);

typedef struct FB_GFX3_OPENGL_FUNCTIONS {
	FB_GFX3_GL_GET_INTEGER get_integer;
	PFNGLGETINTEGERI_VPROC get_integer_indexed;
	PFNGLGETINTEGER64VPROC get_integer64;
	FB_GFX3_GL_GET_STRING get_string;
	PFNGLGETSTRINGIPROC get_string_indexed;
	FB_GFX3_GL_GET_ERROR get_error;
	FB_GFX3_GL_GENERATE_TEXTURES generate_textures;
	FB_GFX3_GL_BIND_TEXTURE bind_texture;
	FB_GFX3_GL_TEXTURE_PARAMETER_I texture_parameter_i;
	FB_GFX3_GL_TEXTURE_SUB_IMAGE_2D texture_sub_image_2d;
	PFNGLTEXSTORAGE2DPROC texture_storage_2d;
	PFNGLCOPYIMAGESUBDATAPROC copy_image_sub_data;
	FB_GFX3_GL_DELETE_TEXTURES delete_textures;
	FB_GFX3_GL_PIXEL_STORE_I pixel_store_i;
	PFNGLGENFRAMEBUFFERSPROC generate_framebuffers;
	PFNGLBINDFRAMEBUFFERPROC bind_framebuffer;
	PFNGLFRAMEBUFFERTEXTURE2DPROC framebuffer_texture_2d;
	PFNGLCHECKFRAMEBUFFERSTATUSPROC check_framebuffer_status;
	FB_GFX3_GL_READ_BUFFER read_buffer;
	FB_GFX3_GL_READ_PIXELS read_pixels;
	PFNGLDELETEFRAMEBUFFERSPROC delete_framebuffers;
	PFNGLCREATESHADERPROC create_shader;
	PFNGLSHADERSOURCEPROC shader_source;
	PFNGLCOMPILESHADERPROC compile_shader;
	PFNGLGETSHADERIVPROC get_shader_i;
	PFNGLGETSHADERINFOLOGPROC get_shader_log;
	PFNGLDELETESHADERPROC delete_shader;
	PFNGLCREATEPROGRAMPROC create_program;
	PFNGLATTACHSHADERPROC attach_shader;
	PFNGLLINKPROGRAMPROC link_program;
	PFNGLGETPROGRAMIVPROC get_program_i;
	PFNGLGETPROGRAMINFOLOGPROC get_program_log;
	PFNGLDELETEPROGRAMPROC delete_program;
	PFNGLUSEPROGRAMPROC use_program;
	PFNGLGETUNIFORMLOCATIONPROC get_uniform_location;
	PFNGLUNIFORM1UIPROC uniform_1ui;
	PFNGLUNIFORM1UIVPROC uniform_1uiv;
	PFNGLUNIFORM1IPROC uniform_1i;
	PFNGLUNIFORM2FPROC uniform_2f;
	PFNGLUNIFORM4IPROC uniform_4i;
	PFNGLUNIFORMMATRIX3FVPROC uniform_matrix_3fv;
	PFNGLBINDIMAGETEXTUREPROC bind_image_texture;
	PFNGLDISPATCHCOMPUTEPROC dispatch_compute;
	PFNGLMEMORYBARRIERPROC memory_barrier;
	PFNGLGENBUFFERSPROC generate_buffers;
	PFNGLBINDBUFFERPROC bind_buffer;
	PFNGLBUFFERDATAPROC buffer_data;
	PFNGLBINDBUFFERBASEPROC bind_buffer_base;
	PFNGLDELETEBUFFERSPROC delete_buffers;
	PFNGLGENVERTEXARRAYSPROC generate_vertex_arrays;
	PFNGLBINDVERTEXARRAYPROC bind_vertex_array;
	PFNGLDELETEVERTEXARRAYSPROC delete_vertex_arrays;
	PFNGLENABLEVERTEXATTRIBARRAYPROC enable_vertex_attrib_array;
	PFNGLDISABLEVERTEXATTRIBARRAYPROC disable_vertex_attrib_array;
	PFNGLVERTEXATTRIBIPOINTERPROC vertex_attrib_i_pointer;
	PFNGLVERTEXATTRIBDIVISORPROC vertex_attrib_divisor;
	PFNGLDRAWARRAYSINSTANCEDPROC draw_arrays_instanced;
	PFNGLACTIVETEXTUREPROC active_texture;
	FB_GFX3_GL_ENABLE enable;
	FB_GFX3_GL_ENABLE disable;
	FB_GFX3_GL_LOGIC_OP logic_op;
	FB_GFX3_GL_VIEWPORT viewport;
	FB_GFX3_GL_CLEAR_COLOR clear_color;
	FB_GFX3_GL_CLEAR clear;
	FB_GFX3_GL_DRAW_ARRAYS draw_arrays;
	PFNGLFENCESYNCPROC fence_sync;
	PFNGLCLIENTWAITSYNCPROC client_wait_sync;
	PFNGLDELETESYNCPROC delete_sync;
	FB_GFX3_GL_FINISH finish;
} FB_GFX3_OPENGL_FUNCTIONS;

typedef struct FB_GFX3_OPENGL_FENCE {
	GLsync sync;
	uint64_t sequence;
	struct FB_GFX3_OPENGL_FENCE *next;
} FB_GFX3_OPENGL_FENCE;

struct FB_GFX3_OPENGL_STATE;

typedef struct FB_GFX3_OPENGL_SURFACE {
	struct FB_GFX3_OPENGL_STATE *state;
	GLuint texture;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
} FB_GFX3_OPENGL_SURFACE;

/*
	The batch shader receives three std430 ivec4 values for each PUT.  Keeping
	the C layout as complete four-component vectors avoids implementation-
	defined packing rules at the CPU/GPU boundary.
*/
typedef struct FB_GFX3_OPENGL_BLIT_BATCH_ITEM {
	int32_t source_x;
	int32_t source_y;
	int32_t width;
	int32_t height;
	int32_t clip_x1;
	int32_t clip_y1;
	int32_t clip_x2;
	int32_t clip_y2;
	int32_t destination_x;
	int32_t destination_y;
	int32_t reserved1;
	int32_t reserved2;
} FB_GFX3_OPENGL_BLIT_BATCH_ITEM;

typedef struct FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM {
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
	uint32_t color;
	uint32_t reserved1;
	uint32_t reserved2;
	uint32_t reserved3;
} FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM;

typedef struct FB_GFX3_OPENGL_LINE_BATCH_ITEM {
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
	uint32_t color;
	uint32_t reserved1;
	uint32_t reserved2;
	uint32_t reserved3;
} FB_GFX3_OPENGL_LINE_BATCH_ITEM;

typedef struct FB_GFX3_OPENGL_POINTS_BATCH_ITEM {
	int32_t x;
	int32_t y;
	uint32_t command_index;
	uint32_t reserved;
} FB_GFX3_OPENGL_POINTS_BATCH_ITEM;

/*
	Three std430 ivec4 values preserve the exact midpoint inputs for one filled
	ellipse. The selection shader never approximates a circle with a distance
	field, because the public API requires gfxlib2's midpoint pixel coverage.
*/
typedef struct FB_GFX3_OPENGL_ELLIPSE_BATCH_ITEM {
	int32_t center_x;
	int32_t center_y;
	uint32_t radius_x_bits;
	uint32_t radius_y_bits;
	int32_t clip_x1;
	int32_t clip_y1;
	int32_t clip_x2;
	int32_t clip_y2;
	uint32_t color;
	uint32_t reserved1;
	uint32_t reserved2;
	uint32_t reserved3;
} FB_GFX3_OPENGL_ELLIPSE_BATCH_ITEM;

/*
	Opaque points, lines, and ellipses can share one winner image even when
	their public commands alternate. The order field is the low thirteen-bit
	winner key, so the final resolve retains exact BASIC submission order across
	all three shapes.
*/
typedef struct FB_GFX3_OPENGL_PRIMITIVE_BATCH_ITEM {
	int32_t geometry_x1;
	int32_t geometry_y1;
	int32_t geometry_x2_or_radius_x;
	int32_t geometry_y2_or_radius_y;
	int32_t clip_x1;
	int32_t clip_y1;
	int32_t clip_x2;
	int32_t clip_y2;
	uint32_t color;
	uint32_t type;
	uint32_t style_or_filled;
	uint32_t order;
} FB_GFX3_OPENGL_PRIMITIVE_BATCH_ITEM;

typedef struct FB_GFX3_OPENGL_STATE {
	const FB_GFX3_PLATFORM_VTABLE *platform_vtable;
	void *platform;
	FB_GFX3_OPENGL_FUNCTIONS gl;
	FB_GFX3_RESOURCE_REGISTRY *resources;
	FB_GFX3_LOGGER *logger;
	GLuint clear_program;
	GLuint points_program;
	GLuint points_batch_program;
	GLuint points_batch_resolve_program;
	GLuint glyph_batch_program;
	GLuint line_program;
	GLuint line_batch_program;
	GLuint line_batch_resolve_program;
	GLuint rectangle_program;
	GLuint ellipse_program;
	GLuint ellipse_batch_program;
	GLuint ellipse_batch_resolve_program;
	GLuint primitive_batch_program;
	GLuint primitive_batch_resolve_program;
	GLuint paint_program;
	GLuint blit_program;
	GLuint transform_blit_program;
	GLuint blit_alpha_tile_program;
	GLuint blit_batch_program;
	GLuint blit_batch_resolve_program;
	GLuint blit_raster_batch_program;
	GLuint rectangle_raster_batch_program;
	GLuint rectangle_batch_program;
	GLuint rectangle_batch_resolve_program;
	GLuint present_program;
	GLuint point_buffer;
	GLuint blit_batch_buffer;
	GLuint blit_tile_range_buffer;
	GLuint blit_tile_index_buffer;
	GLuint blit_batch_winner_texture;
	uint32_t blit_batch_winner_width;
	uint32_t blit_batch_winner_height;
	uint32_t blit_batch_generation;
	/* Reused by exact GPU PAINT scanline queues. */
	GLuint paint_scratch_buffer;
	size_t paint_scratch_bytes;
	GLuint present_vertex_array;
	GLuint blit_batch_vertex_array;
	GLuint rectangle_batch_vertex_array;
	GLuint read_framebuffer;
	/*
		The framebuffer object retains its colour attachment while it is
		unbound. Remember that attachment so consecutive sprite and rectangle
		packets targeting one page do not ask the driver to rebuild and validate
		the same framebuffer state.
	*/
	GLuint framebuffer_color_texture;
	/*
		Runtime glGetError calls are useful while diagnosing a driver, but they
		add a driver crossing after every primitive packet. Initialization always
		validates its work. The production command path checks only when
		FBGFX3_OPENGL_VALIDATE requests the diagnostic mode.
	*/
	int runtime_ready;
	int validate_runtime_errors;
	GLint clear_rect_location;
	GLint clear_color_location;
	GLint clear_mask_location;
	GLint clear_flags_location;
	GLint points_clip_location;
	GLint points_count_location;
	GLint points_mask_location;
	GLint points_batch_clip_location;
	GLint points_batch_count_location;
	GLint points_batch_key_location;
	GLint points_batch_resolve_rect_location;
	GLint points_batch_resolve_count_location;
	GLint points_batch_resolve_generation_location;
	GLint points_batch_resolve_mask_location;
	GLint glyph_batch_clip_location;
	GLint glyph_batch_count_location;
	GLint glyph_batch_tile_origin_location;
	GLint glyph_batch_tiles_x_location;
	GLint glyph_batch_mask_location;
	GLint line_endpoints_location;
	GLint line_clip_location;
	GLint line_count_location;
	GLint line_color_location;
	GLint line_style_location;
	GLint line_mask_location;
	GLint line_flags_location;
	GLint line_batch_clip_location;
	GLint line_batch_style_location;
	GLint line_batch_key_location;
	GLint line_batch_resolve_rect_location;
	GLint line_batch_resolve_generation_location;
	GLint line_batch_resolve_mask_location;
	GLint rectangle_box_location;
	GLint rectangle_clip_location;
	GLint rectangle_width_location;
	GLint rectangle_height_location;
	GLint rectangle_count_location;
	GLint rectangle_color_location;
	GLint rectangle_style_location;
	GLint rectangle_mask_location;
	GLint rectangle_filled_location;
	GLint rectangle_flags_location;
	GLint ellipse_center_location;
	GLint ellipse_clip_location;
	GLint ellipse_radii_location;
	GLint ellipse_color_location;
	GLint ellipse_filled_location;
	GLint ellipse_mask_location;
	GLint ellipse_flags_location;
	GLint ellipse_batch_key_location;
	GLint ellipse_batch_resolve_rect_location;
	GLint ellipse_batch_resolve_generation_location;
	GLint ellipse_batch_resolve_mask_location;
	GLint primitive_batch_key_location;
	GLint primitive_batch_resolve_rect_location;
	GLint primitive_batch_resolve_generation_location;
	GLint primitive_batch_resolve_mask_location;
	GLint paint_seed_location;
	GLint paint_clip_location;
	GLint paint_color_location;
	GLint paint_border_location;
	GLint paint_mask_location;
	GLint paint_flags_location;
	GLint paint_mode_location;
	GLint paint_pattern_size_location;
	GLint paint_pattern_origin_location;
	GLint paint_bytes_per_pixel_location;
	GLint paint_pattern_location;
	GLint paint_phase_location;
	GLint blit_source_rect_location;
	GLint blit_clip_location;
	GLint blit_destination_x_location;
	GLint blit_destination_y_location;
	GLint blit_mode_location;
	GLint blit_alpha_location;
	GLint blit_depth_location;
	GLint blit_mask_location;
	GLint transform_blit_source_rect_location;
	GLint transform_blit_clip_location;
	GLint transform_blit_bounds_location;
	GLint transform_blit_inverse_location;
	GLint transform_blit_mode_location;
	GLint transform_blit_alpha_location;
	GLint transform_blit_depth_location;
	GLint transform_blit_mask_location;
	GLint transform_blit_filter_location;
	GLint transform_blit_wrap_location;
	GLint blit_alpha_tile_count_location;
	GLint blit_alpha_tile_mode_location;
	GLint blit_alpha_tile_alpha_location;
	GLint blit_alpha_tile_depth_location;
	GLint blit_alpha_tile_mask_location;
	GLint blit_batch_mode_location;
	GLint blit_batch_depth_location;
	GLint blit_batch_mask_location;
	GLint blit_batch_key_location;
	GLint blit_batch_resolve_rect_location;
	GLint blit_batch_resolve_generation_location;
	GLint blit_batch_resolve_mask_location;
	/* Uniform locations for the direct raster sprite batch are immutable after link. */
	GLint blit_raster_batch_source_location;
	GLint blit_raster_batch_size_location;
	GLint blit_raster_batch_mode_location;
	GLint blit_raster_batch_depth_location;
	GLint blit_raster_batch_mask_location;
	GLint rectangle_raster_batch_size_location;
	GLint rectangle_raster_batch_mask_location;
	GLint rectangle_batch_key_location;
	GLint rectangle_batch_resolve_rect_location;
	GLint rectangle_batch_resolve_generation_location;
	GLint rectangle_batch_resolve_mask_location;
	GLint present_source_location;
	GLint present_depth_location;
	GLint present_palette_location;
	uint64_t maximum_storage_buffer_size;
	uint32_t maximum_compute_groups_x;
	uint32_t maximum_compute_groups_y;
	uint32_t maximum_compute_groups_z;
	uint32_t palette[256];
	FB_GFX3_HANDLE visible_surface;
	FB_GFX3_HANDLE page_content_handle[FB_GFX3_OPENGL_PAGE_CONTENT_LIMIT];
	uint64_t page_content_token[FB_GFX3_OPENGL_PAGE_CONTENT_LIMIT];
	uint64_t next_page_content_token;
	uint32_t page_content_count;
	int presentation_dirty;
	uint32_t client_width;
	uint32_t client_height;
	uint64_t submitted_sequence;
	uint64_t completed_sequence;
	/*
		A CPU-only control batch has no GL operation to fence. Its sequence
		becomes complete after every older GL fence has retired.
	*/
	uint64_t control_sequence;
	FB_GFX3_OPENGL_FENCE *first_fence;
	FB_GFX3_OPENGL_FENCE *last_fence;
} FB_GFX3_OPENGL_STATE;

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

static int opengl_load_functions(FB_GFX3_OPENGL_STATE *state)
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

static const char opengl_clear_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 16, local_size_y = 16) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D target_image;\n"
	"uniform ivec4 operation_rect;\n"
	"uniform uint operation_color;\n"
	"uniform uint operation_mask;\n"
	"uniform uint operation_flags;\n"
	"uniform uint operation_mode;\n"
	"uniform uint operation_pattern_size;\n"
	"uniform ivec4 operation_pattern_origin;\n"
	"uniform uint operation_bytes_per_pixel;\n"
	"uniform uint operation_pattern[64];\n"
	"uint alpha_primitive(uint source, uint destination)\n"
	"{\n"
	"    uint a = source >> 24; uint srb = source & 0x00FF00FFu;\n"
	"    uint sg = source & 0x0000FF00u;\n"
	"    uint drb = destination & 0x00FF00FFu;\n"
	"    uint dg = destination & 0x0000FF00u;\n"
	"    srb = ((srb - drb) * a) >> 8; sg = ((sg - dg) * a) >> 8;\n"
	"    return ((drb + srb) & 0x00FF00FFu) |\n"
	"        ((dg + sg) & 0x0000FF00u) | (source & 0xFF000000u);\n"
	"}\n"
	"void write_pixel(ivec2 position, uint source)\n"
	"{\n"
	"    source &= operation_mask;\n"
	"    if ((operation_flags & 1u) == 0u) {\n"
	"        imageStore(target_image, position, uvec4(source, 0u, 0u, 0u));\n"
	"        return;\n"
	"    }\n"
	"    uint expected = imageLoad(target_image, position).r; uint prior;\n"
	"    do { prior = expected; expected = imageAtomicCompSwap(target_image,\n"
	"        position, prior, alpha_primitive(source, prior)); }\n"
	"    while (expected != prior);\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 offset = ivec2(gl_GlobalInvocationID.xy);\n"
	"    if ((offset.x >= operation_rect.z) ||\n"
	"        (offset.y >= operation_rect.w))\n"
	"        return;\n"
	"    write_pixel(operation_rect.xy + offset, operation_color);\n"
	"}\n";

static const char opengl_points_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 64) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D target_image;\n"
	"struct Point { ivec2 position; uint color; uint flags; };\n"
	"layout(std430, binding = 1) readonly buffer PointData {\n"
	"    Point points[];\n"
	"};\n"
	"uniform ivec4 operation_clip;\n"
	"uniform uint operation_count;\n"
	"uniform uint operation_mask;\n"
	"uint alpha_primitive(uint source, uint destination)\n"
	"{\n"
	"    uint a = source >> 24; uint srb = source & 0x00FF00FFu;\n"
	"    uint sg = source & 0x0000FF00u;\n"
	"    uint drb = destination & 0x00FF00FFu;\n"
	"    uint dg = destination & 0x0000FF00u;\n"
	"    srb = ((srb - drb) * a) >> 8; sg = ((sg - dg) * a) >> 8;\n"
	"    return ((drb + srb) & 0x00FF00FFu) |\n"
	"        ((dg + sg) & 0x0000FF00u) | (source & 0xFF000000u);\n"
	"}\n"
	"void write_pixel(ivec2 position, Point point)\n"
	"{\n"
	"    uint source = point.color & operation_mask;\n"
	"    if ((point.flags & 1u) == 0u) {\n"
	"        imageStore(target_image, position, uvec4(source, 0u, 0u, 0u));\n"
	"        return;\n"
	"    }\n"
	"    uint expected = imageLoad(target_image, position).r; uint prior;\n"
	"    do { prior = expected; expected = imageAtomicCompSwap(target_image,\n"
	"        position, prior, alpha_primitive(source, prior)); }\n"
	"    while (expected != prior);\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    uint index = gl_GlobalInvocationID.x;\n"
	"    if (index >= operation_count)\n"
	"        return;\n"
	"    ivec2 position = points[index].position;\n"
	"    if ((position.x < operation_clip.x) ||\n"
	"        (position.y < operation_clip.y) ||\n"
	"        (position.x > operation_clip.z) ||\n"
	"        (position.y > operation_clip.w))\n"
	"        return;\n"
	"    write_pixel(position, points[index]);\n"
	"}\n";

/*
	Built-in text and the graphical console produce short, opaque POINTS
	commands.  A command-level winner pass lets adjacent runs retain Basic's
	last-write-wins order without making a compute dispatch for every glyph.
	Point data occupies the first operation_count records; one colour record per
	Basic command follows it.  This compact layout also keeps the buffer useful
	for small console writes where dispatch overhead otherwise dominates.
*/
static const char opengl_points_batch_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 64) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D winner_image;\n"
	"layout(std430, binding = 2) readonly buffer PointData { uvec4 point_data[]; };\n"
	"uniform ivec4 operation_clip;\n"
	"uniform uint operation_count;\n"
	"uniform uint operation_batch_key;\n"
	"void main(void)\n"
	"{\n"
	"    uint index = gl_GlobalInvocationID.x;\n"
	"    if (index >= operation_count) return;\n"
	"    ivec2 position = ivec2(point_data[index].xy);\n"
	"    if ((position.x < operation_clip.x) || (position.y < operation_clip.y) ||\n"
	"        (position.x > operation_clip.z) || (position.y > operation_clip.w)) return;\n"
	"    imageAtomicMax(winner_image, position,\n"
	"        operation_batch_key + point_data[index].z);\n"
	"}\n";

static const char opengl_points_batch_resolve_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 16, local_size_y = 16) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D destination_image;\n"
	"layout(r32ui, binding = 1) uniform readonly uimage2D winner_image;\n"
	"layout(std430, binding = 2) readonly buffer PointData { uvec4 point_data[]; };\n"
	"uniform ivec4 operation_resolve_rect;\n"
	"uniform uint operation_count;\n"
	"uniform uint operation_batch_generation;\n"
	"uniform uint operation_mask;\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 offset = ivec2(gl_GlobalInvocationID.xy);\n"
	"    if ((offset.x >= operation_resolve_rect.z) ||\n"
	"        (offset.y >= operation_resolve_rect.w)) return;\n"
	"    ivec2 position = operation_resolve_rect.xy + offset;\n"
	"    uint winner = imageLoad(winner_image, position).r;\n"
	"    if ((winner >> 13) != operation_batch_generation) return;\n"
	"    uint command_index = (winner & 8191u) - 1u;\n"
	"    uint color = point_data[operation_count + command_index].w;\n"
	"    imageStore(destination_image, position,\n"
	"        uvec4(color & operation_mask, 0u, 0u, 0u));\n"
	"}\n";

/*
	Each workgroup owns one 8 by 8 destination tile. The host supplies glyph
	indices in command order, so every invocation can choose its final foreground
	or background value without atomics or a second resolve dispatch.
*/
static const char opengl_glyph_batch_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 8, local_size_y = 8) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D destination_image;\n"
	"struct Glyph { ivec2 position; uint foreground; uint background;\n"
	"    uvec4 format; uint rows[16]; };\n"
	"layout(std430, binding = 2) readonly buffer GlyphData { Glyph glyphs[]; };\n"
	"layout(std430, binding = 3) readonly buffer TileRanges { uvec2 ranges[]; };\n"
	"layout(std430, binding = 4) readonly buffer TileIndices { uint indices[]; };\n"
	"uniform ivec4 operation_clip; uniform uint operation_count;\n"
	"uniform ivec4 operation_tile_origin; uniform uint operation_tiles_x;\n"
	"uniform uint operation_mask;\n"
	"void main(void)\n"
	"{\n"
	"    uvec2 tile_position = uvec2(operation_tile_origin.xy) + gl_WorkGroupID.xy;\n"
	"    ivec2 position = ivec2(tile_position * 8u + gl_LocalInvocationID.xy);\n"
	"    uint tile = gl_WorkGroupID.y * operation_tiles_x + gl_WorkGroupID.x;\n"
	"    uvec2 range = ranges[tile]; uint color = 0u; bool wrote = false;\n"
	"    if ((position.x < operation_clip.x) || (position.y < operation_clip.y) ||\n"
	"        (position.x > operation_clip.z) || (position.y > operation_clip.w)) return;\n"
	"    for (uint offset = 0u; offset < range.y; ++offset) {\n"
	"        uint glyph_index = indices[range.x + offset];\n"
	"        if (glyph_index >= operation_count) continue; Glyph glyph = glyphs[glyph_index];\n"
	"        ivec2 local = position - glyph.position;\n"
	"        if ((local.x < 0) || (local.y < 0) || (uint(local.x) >= glyph.format.x) ||\n"
	"            (uint(local.y) >= glyph.format.y)) continue;\n"
	"        if ((glyph.rows[local.y] & (1u << uint(local.x))) != 0u) {\n"
	"            color = glyph.foreground; wrote = true;\n"
	"        } else if ((glyph.format.z & 1u) != 0u) {\n"
	"            color = glyph.background; wrote = true;\n"
	"        }\n"
	"    }\n"
	"    if (wrote) imageStore(destination_image, position,\n"
	"        uvec4(color & operation_mask, 0u, 0u, 0u));\n"
	"}\n";

static const char opengl_line_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 64) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D target_image;\n"
	"uniform ivec4 operation_endpoints;\n"
	"uniform ivec4 operation_clip;\n"
	"uniform uint operation_count;\n"
	"uniform uint operation_color;\n"
	"uniform uint operation_style;\n"
	"uniform uint operation_mask;\n"
	"uniform uint operation_flags;\n"
	"uint alpha_primitive(uint source, uint destination)\n"
	"{\n"
	"    uint a = source >> 24; uint srb = source & 0x00FF00FFu;\n"
	"    uint sg = source & 0x0000FF00u;\n"
	"    uint drb = destination & 0x00FF00FFu;\n"
	"    uint dg = destination & 0x0000FF00u;\n"
	"    srb = ((srb - drb) * a) >> 8; sg = ((sg - dg) * a) >> 8;\n"
	"    return ((drb + srb) & 0x00FF00FFu) |\n"
	"        ((dg + sg) & 0x0000FF00u) | (source & 0xFF000000u);\n"
	"}\n"
	"void write_pixel(ivec2 position)\n"
	"{\n"
	"    uint source = operation_color & operation_mask;\n"
	"    if ((operation_flags & 1u) == 0u) {\n"
	"        imageStore(target_image, position, uvec4(source, 0u, 0u, 0u));\n"
	"        return;\n"
	"    }\n"
	"    uint expected = imageLoad(target_image, position).r; uint prior;\n"
	"    do { prior = expected; expected = imageAtomicCompSwap(target_image,\n"
	"        position, prior, alpha_primitive(source, prior)); }\n"
	"    while (expected != prior);\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    uint index = gl_GlobalInvocationID.x;\n"
	"    if (index >= operation_count)\n"
	"        return;\n"
	"    ivec2 start = operation_endpoints.xy;\n"
	"    ivec2 finish = operation_endpoints.zw;\n"
	"    ivec2 difference = abs(finish - start);\n"
	"    ivec2 direction = ivec2((finish.x < start.x) ? -1 : 1,\n"
	"        (finish.y < start.y) ? -1 : 1);\n"
	"    int step_index = int(index);\n"
	"    ivec2 position;\n"
	"    if (difference.x >= difference.y) {\n"
	"        position.x = start.x + (direction.x * step_index);\n"
	"        position.y = start.y;\n"
	"        if (difference.x != 0)\n"
	"            position.y += direction.y * ((difference.y * step_index +\n"
	"                (difference.x / 2)) / difference.x);\n"
	"    } else {\n"
	"        position.y = start.y + (direction.y * step_index);\n"
	"        position.x = start.x + direction.x * ((difference.x *\n"
	"            step_index + (difference.y / 2)) / difference.y);\n"
	"    }\n"
	"    if ((position.x < operation_clip.x) ||\n"
	"        (position.y < operation_clip.y) ||\n"
	"        (position.x > operation_clip.z) ||\n"
	"        (position.y > operation_clip.w))\n"
	"        return;\n"
	"    uint style_bit = 0x8000u >> (index & 15u);\n"
	"    if ((operation_style & style_bit) != 0u)\n"
	"        write_pixel(position);\n"
	"}\n";

/*
	Opaque lines have no destination read.  The selection pass records the last
	Basic command reaching every pixel, then the resolve pass performs one write.
	That retains strict FIFO overlap behavior while allowing a DRAW command
	stream to reach OpenGL as two compute dispatches instead of one per line.
*/
static const char opengl_line_batch_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 64) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D winner_image;\n"
	"layout(std430, binding = 2) readonly buffer LineData {\n"
	"    ivec4 line_data[];\n"
	"};\n"
	"uniform ivec4 operation_clip;\n"
	"uniform uint operation_style;\n"
	"uniform uint operation_batch_key;\n"
	"void main(void)\n"
	"{\n"
	"    uint index = gl_GlobalInvocationID.x;\n"
	"    ivec4 line = line_data[gl_WorkGroupID.y * 2u];\n"
	"    ivec2 start = line.xy; ivec2 finish = line.zw;\n"
	"    ivec2 difference = abs(finish - start);\n"
	"    uint count = uint(max(difference.x, difference.y)) + 1u;\n"
	"    if (index >= count) return;\n"
	"    ivec2 direction = ivec2((finish.x < start.x) ? -1 : 1,\n"
	"        (finish.y < start.y) ? -1 : 1);\n"
	"    int step_index = int(index); ivec2 position;\n"
	"    if (difference.x >= difference.y) {\n"
	"        position.x = start.x + (direction.x * step_index);\n"
	"        position.y = start.y;\n"
	"        if (difference.x != 0) position.y += direction.y *\n"
	"            ((difference.y * step_index + (difference.x / 2)) / difference.x);\n"
	"    } else {\n"
	"        position.y = start.y + (direction.y * step_index);\n"
	"        position.x = start.x;\n"
	"        if (difference.y != 0) position.x += direction.x *\n"
	"            ((difference.x * step_index + (difference.y / 2)) / difference.y);\n"
	"    }\n"
	"    if ((position.x < operation_clip.x) || (position.y < operation_clip.y) ||\n"
	"        (position.x > operation_clip.z) || (position.y > operation_clip.w)) return;\n"
	"    if ((operation_style & (0x8000u >> (index & 15u))) == 0u) return;\n"
	"    imageAtomicMax(winner_image, position,\n"
	"        operation_batch_key + gl_WorkGroupID.y);\n"
	"}\n";

static const char opengl_line_batch_resolve_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 16, local_size_y = 16) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D destination_image;\n"
	"layout(r32ui, binding = 1) uniform readonly uimage2D winner_image;\n"
	"layout(std430, binding = 2) readonly buffer LineData {\n"
	"    ivec4 line_data[];\n"
	"};\n"
	"uniform ivec4 operation_resolve_rect;\n"
	"uniform uint operation_batch_generation;\n"
	"uniform uint operation_mask;\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 offset = ivec2(gl_GlobalInvocationID.xy);\n"
	"    if ((offset.x >= operation_resolve_rect.z) ||\n"
	"        (offset.y >= operation_resolve_rect.w)) return;\n"
	"    ivec2 position = operation_resolve_rect.xy + offset;\n"
	"    uint winner = imageLoad(winner_image, position).r;\n"
	"    if ((winner >> 13) != operation_batch_generation) return;\n"
	"    uint command_index = (winner & 8191u) - 1u;\n"
	"    uint color = uint(line_data[command_index * 2u + 1u].x);\n"
	"    imageStore(destination_image, position,\n"
	"        uvec4(color & operation_mask, 0u, 0u, 0u));\n"
	"}\n";

static const char opengl_rectangle_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 64) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D target_image;\n"
	"uniform ivec4 operation_box;\n"
	"uniform ivec4 operation_clip;\n"
	"uniform uint operation_width;\n"
	"uniform uint operation_height;\n"
	"uniform uint operation_count;\n"
	"uniform uint operation_color;\n"
	"uniform uint operation_style;\n"
	"uniform uint operation_mask;\n"
	"uniform uint operation_filled;\n"
	"uniform uint operation_flags;\n"
	"uint alpha_primitive(uint source, uint destination)\n"
	"{\n"
	"    uint a = source >> 24; uint srb = source & 0x00FF00FFu;\n"
	"    uint sg = source & 0x0000FF00u;\n"
	"    uint drb = destination & 0x00FF00FFu;\n"
	"    uint dg = destination & 0x0000FF00u;\n"
	"    srb = ((srb - drb) * a) >> 8; sg = ((sg - dg) * a) >> 8;\n"
	"    return ((drb + srb) & 0x00FF00FFu) |\n"
	"        ((dg + sg) & 0x0000FF00u) | (source & 0xFF000000u);\n"
	"}\n"
	"void write_pixel(ivec2 position)\n"
	"{\n"
	"    uint source = operation_color & operation_mask;\n"
	"    if ((operation_flags & 1u) == 0u) {\n"
	"        imageStore(target_image, position, uvec4(source, 0u, 0u, 0u));\n"
	"        return;\n"
	"    }\n"
	"    uint expected = imageLoad(target_image, position).r; uint prior;\n"
	"    do { prior = expected; expected = imageAtomicCompSwap(target_image,\n"
	"        position, prior, alpha_primitive(source, prior)); }\n"
	"    while (expected != prior);\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    uint index = gl_GlobalInvocationID.x;\n"
	"    if (index >= operation_count) return;\n"
	"    ivec2 position;\n"
	"    if (operation_filled != 0u) {\n"
	"        position = ivec2(operation_box.x + int(index % operation_width),\n"
	"            operation_box.y + int(index / operation_width));\n"
	"    } else if (index < operation_width) {\n"
	"        position = ivec2(operation_box.x + int(index),\n"
	"            operation_box.w);\n"
	"    } else if (index < (operation_width * 2u)) {\n"
	"        position = ivec2(operation_box.x +\n"
	"            int(index - operation_width), operation_box.y);\n"
	"    } else if (index < ((operation_width * 2u) +\n"
	"        operation_height)) {\n"
	"        position = ivec2(operation_box.z, operation_box.y +\n"
	"            int(index - (operation_width * 2u)));\n"
	"    } else {\n"
	"        position = ivec2(operation_box.x, operation_box.y +\n"
	"            int(index - (operation_width * 2u) - operation_height));\n"
	"    }\n"
	"    if ((position.x < operation_clip.x) ||\n"
	"        (position.y < operation_clip.y) ||\n"
	"        (position.x > operation_clip.z) ||\n"
	"        (position.y > operation_clip.w)) return;\n"
	"    uint style_bit = 0x8000u >> (index & 15u);\n"
	"    if ((operation_filled != 0u) ||\n"
	"        ((operation_style & style_bit) != 0u))\n"
	"        write_pixel(position);\n"
	"}\n";

/*
	The midpoint state is serial, but opaque filled spans are independent once
	each state transition is known. One lane advances the exact state machine and
	the other 63 lanes cooperatively fill the resulting horizontal spans. Alpha
	and outline paths stay serial because their writes can overlap and therefore
	must preserve destination-read order. Double values hold integer intermediates
	exactly without making GL_ARB_gpu_shader_int64 a backend requirement.
*/
static const char opengl_ellipse_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 64) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D target_image;\n"
	"uniform ivec4 operation_center;\n"
	"uniform ivec4 operation_clip;\n"
	"uniform vec2 operation_radii;\n"
	"uniform uint operation_color;\n"
	"uniform uint operation_filled;\n"
	"uniform uint operation_mask;\n"
	"uniform uint operation_flags;\n"
	"shared int parallel_x1; shared int parallel_x2;\n"
	"shared int parallel_y1; shared int parallel_y2; shared int parallel_d;\n"
	"shared double parallel_aq; shared double parallel_bq;\n"
	"shared double parallel_dx; shared double parallel_dy;\n"
	"shared double parallel_r; shared double parallel_rx; shared double parallel_ry;\n"
	"uint alpha_primitive(uint source, uint destination)\n"
	"{\n"
	"    uint a = source >> 24; uint srb = source & 0x00FF00FFu;\n"
	"    uint sg = source & 0x0000FF00u;\n"
	"    uint drb = destination & 0x00FF00FFu;\n"
	"    uint dg = destination & 0x0000FF00u;\n"
	"    srb = ((srb - drb) * a) >> 8; sg = ((sg - dg) * a) >> 8;\n"
	"    return ((drb + srb) & 0x00FF00FFu) |\n"
	"        ((dg + sg) & 0x0000FF00u) | (source & 0xFF000000u);\n"
	"}\n"
	"void write_pixel(ivec2 position)\n"
	"{\n"
	"    uint source = operation_color & operation_mask;\n"
	"    if ((operation_flags & 1u) != 0u)\n"
	"        source = alpha_primitive(source,\n"
	"            imageLoad(target_image, position).r);\n"
	"    imageStore(target_image, position, uvec4(source, 0u, 0u, 0u));\n"
	"}\n"
	"void draw_scanline(int y, int x1, int x2, bool filled)\n"
	"{\n"
	"    if ((y < operation_clip.y) || (y > operation_clip.w)) return;\n"
	"    if (filled) {\n"
	"        int first = max(x1, operation_clip.x);\n"
	"        int last = min(x2, operation_clip.z);\n"
	"        for (int x = first; x <= last; ++x)\n"
	"            write_pixel(ivec2(x, y));\n"
	"    } else {\n"
	"        if ((x1 >= operation_clip.x) && (x1 <= operation_clip.z))\n"
	"            write_pixel(ivec2(x1, y));\n"
	"        if ((x2 >= operation_clip.x) && (x2 <= operation_clip.z))\n"
	"            write_pixel(ivec2(x2, y));\n"
	"    }\n"
	"}\n"
	"void draw_opaque_filled_scanline(int y, int x1, int x2)\n"
	"{\n"
	"    if ((y < operation_clip.y) || (y > operation_clip.w)) return;\n"
	"    int first = max(x1, operation_clip.x);\n"
	"    int last = min(x2, operation_clip.z);\n"
	"    for (int x = first + int(gl_LocalInvocationID.x); x <= last;\n"
	"         x += int(gl_WorkGroupSize.x))\n"
	"        write_pixel(ivec2(x, y));\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    int x1 = int(float(operation_center.x) - operation_radii.x);\n"
	"    int x2 = int(float(operation_center.x) + operation_radii.x);\n"
	"    int y1 = operation_center.y;\n"
	"    int y2 = operation_center.y;\n"
	"    bool filled = operation_filled != 0u;\n"
	"    if (filled && ((operation_flags & 1u) == 0u)) {\n"
	"        if (gl_LocalInvocationID.x == 0u) {\n"
	"            parallel_x1 = x1; parallel_x2 = x2;\n"
	"            parallel_y1 = y1; parallel_y2 = y2;\n"
	"            parallel_d = int(operation_radii.x);\n"
	"            parallel_aq = trunc(double(operation_radii.x * operation_radii.x));\n"
	"            parallel_bq = trunc(double(operation_radii.y * operation_radii.y));\n"
	"            parallel_dx = parallel_aq * 2.0; parallel_dy = parallel_bq * 2.0;\n"
	"            parallel_r = trunc(double(operation_radii.x * float(parallel_bq)));\n"
	"            parallel_rx = parallel_r * 2.0; parallel_ry = 0.0;\n"
	"        }\n"
	"        barrier();\n"
	"        if (operation_radii.y == 0.0) {\n"
	"            draw_opaque_filled_scanline(parallel_y1, parallel_x1, parallel_x2);\n"
	"            return;\n"
	"        }\n"
	"        draw_opaque_filled_scanline(parallel_y1, parallel_x1, parallel_x2);\n"
	"        while (parallel_d > 0) {\n"
	"            if (gl_LocalInvocationID.x == 0u) {\n"
	"                if (parallel_r > 0.0) {\n"
	"                    ++parallel_y1; --parallel_y2;\n"
	"                    parallel_ry += parallel_dx; parallel_r -= parallel_ry;\n"
	"                }\n"
	"                if (parallel_r <= 0.0) {\n"
	"                    --parallel_d; ++parallel_x1; --parallel_x2;\n"
	"                    parallel_rx -= parallel_dy; parallel_r += parallel_rx;\n"
	"                }\n"
	"            }\n"
	"            barrier();\n"
	"            draw_opaque_filled_scanline(parallel_y1, parallel_x1, parallel_x2);\n"
	"            if (parallel_y2 != parallel_y1)\n"
	"                draw_opaque_filled_scanline(parallel_y2, parallel_x1, parallel_x2);\n"
	"            barrier();\n"
	"        }\n"
	"        return;\n"
	"    }\n"
	"    if (gl_LocalInvocationID.x != 0u) return;\n"
	"    if (operation_radii.y == 0.0) {\n"
	"        draw_scanline(y1, x1, x2, true); return;\n"
	"    }\n"
	"    draw_scanline(y1, x1, x2, filled);\n"
	"    double aq = trunc(double(operation_radii.x * operation_radii.x));\n"
	"    double bq = trunc(double(operation_radii.y * operation_radii.y));\n"
	"    double dx = aq * 2.0; double dy = bq * 2.0;\n"
	"    double r = trunc(double(operation_radii.x * float(bq)));\n"
	"    double rx = r * 2.0; double ry = 0.0;\n"
	"    int d = int(operation_radii.x);\n"
	"    while (d > 0) {\n"
	"        if (r > 0.0) {\n"
	"            ++y1; --y2; ry += dx; r -= ry;\n"
	"        }\n"
	"        if (r <= 0.0) {\n"
	"            --d; ++x1; --x2; rx -= dy; r += rx;\n"
	"        }\n"
	"        draw_scanline(y1, x1, x2, filled);\n"
	"        draw_scanline(y2, x1, x2, filled);\n"
	"    }\n"
	"}\n";

/*
	Filled CIRCLE and ELLIPSE calls often arrive in long opaque runs. One compute
	dispatch assigns one 64-lane workgroup to each exact midpoint state machine.
	The winner image gives overlapping shapes their required last-command result
	without serializing thousands of public commands on the render thread.
*/
static const char opengl_ellipse_batch_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 64) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D winner_image;\n"
	"layout(std430, binding = 2) readonly buffer ellipse_commands { ivec4 command_data[]; };\n"
	"uniform uint operation_batch_key;\n"
	"shared int shared_x1; shared int shared_x2;\n"
	"shared int shared_y1; shared int shared_y2; shared int shared_d;\n"
	"shared int shared_center_y; shared ivec4 shared_clip;\n"
	"shared double shared_aq; shared double shared_bq;\n"
	"shared double shared_dx; shared double shared_dy;\n"
	"shared double shared_r; shared double shared_rx; shared double shared_ry;\n"
	"void select_span(int y, int x1, int x2)\n"
	"{\n"
	"    int first = max(x1, shared_clip.x); int last = min(x2, shared_clip.z);\n"
	"    if ((y < shared_clip.y) || (y > shared_clip.w)) return;\n"
	"    for (int x = first + int(gl_LocalInvocationID.x); x <= last;\n"
	"         x += int(gl_WorkGroupSize.x))\n"
	"        imageAtomicMax(winner_image, ivec2(x, y),\n"
	"            operation_batch_key + gl_WorkGroupID.z);\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    int base = int(gl_WorkGroupID.z) * 3;\n"
	"    ivec4 geometry = command_data[base];\n"
	"    float radius_x = uintBitsToFloat(uint(geometry.z));\n"
	"    float radius_y = uintBitsToFloat(uint(geometry.w));\n"
	"    if (gl_LocalInvocationID.x == 0u) {\n"
	"        shared_x1 = int(float(geometry.x) - radius_x);\n"
	"        shared_x2 = int(float(geometry.x) + radius_x);\n"
	"        shared_y1 = geometry.y; shared_y2 = geometry.y;\n"
	"        shared_center_y = geometry.y; shared_clip = command_data[base + 1];\n"
	"        shared_d = int(radius_x);\n"
	"        shared_aq = trunc(double(radius_x * radius_x));\n"
	"        shared_bq = trunc(double(radius_y * radius_y));\n"
	"        shared_dx = shared_aq * 2.0; shared_dy = shared_bq * 2.0;\n"
	"        shared_r = trunc(double(radius_x * float(shared_bq)));\n"
	"        shared_rx = shared_r * 2.0; shared_ry = 0.0;\n"
	"    }\n"
	"    barrier();\n"
	"    if (radius_y == 0.0) { select_span(shared_center_y, shared_x1, shared_x2); return; }\n"
	"    select_span(shared_y1, shared_x1, shared_x2);\n"
	"    while (shared_d > 0) {\n"
	"        if (gl_LocalInvocationID.x == 0u) {\n"
	"            if (shared_r > 0.0) { ++shared_y1; --shared_y2; shared_ry += shared_dx; shared_r -= shared_ry; }\n"
	"            if (shared_r <= 0.0) { --shared_d; ++shared_x1; --shared_x2; shared_rx -= shared_dy; shared_r += shared_rx; }\n"
	"        }\n"
	"        barrier();\n"
	"        select_span(shared_y1, shared_x1, shared_x2);\n"
	"        if (shared_y2 != shared_y1) select_span(shared_y2, shared_x1, shared_x2);\n"
	"        barrier();\n"
	"    }\n"
	"}\n";

static const char opengl_ellipse_batch_resolve_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 16, local_size_y = 16) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D destination_image;\n"
	"layout(r32ui, binding = 1) uniform readonly uimage2D winner_image;\n"
	"layout(std430, binding = 2) readonly buffer ellipse_commands { ivec4 command_data[]; };\n"
	"uniform ivec4 operation_resolve_rect; uniform uint operation_batch_generation;\n"
	"uniform uint operation_mask;\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 offset = ivec2(gl_GlobalInvocationID.xy);\n"
	"    if ((offset.x >= operation_resolve_rect.z) || (offset.y >= operation_resolve_rect.w)) return;\n"
	"    ivec2 position = operation_resolve_rect.xy + offset;\n"
	"    uint winner = imageLoad(winner_image, position).r;\n"
	"    if ((winner >> 13) != operation_batch_generation) return;\n"
	"    uint color = uint(command_data[(int(winner & 8191u) - 1) * 3 + 2].x);\n"
	"    imageStore(destination_image, position, uvec4(color & operation_mask, 0u, 0u, 0u));\n"
	"}\n";

/*
	A game often alternates scanline-filled polygons with outline circles.
	Resolving each public packet separately leaves the pixel work on the GPU but
	still makes the renderer thread issue thousands of driver dispatches. This
	selection shader accepts both primitive types and stores their unified BASIC
	order in the shared winner image. One resolve then writes the final colors.

	Clipping remains a per-invocation GPU decision. The CPU computes only a
	conservative resolve rectangle so workgroups which cannot contain a result
	are not launched at all.
*/
static const char opengl_primitive_batch_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 64) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D winner_image;\n"
	"layout(std430, binding = 2) readonly buffer primitive_commands {\n"
	"    ivec4 command_data[];\n"
	"};\n"
	"uniform uint operation_batch_key;\n"
	"shared int ellipse_x1; shared int ellipse_x2;\n"
	"shared int ellipse_y1; shared int ellipse_y2; shared int ellipse_d;\n"
	"shared ivec4 ellipse_clip; shared uint ellipse_key;\n"
	"shared double ellipse_aq; shared double ellipse_bq;\n"
	"shared double ellipse_dx; shared double ellipse_dy;\n"
	"shared double ellipse_r; shared double ellipse_rx; shared double ellipse_ry;\n"
	"void select_ellipse_row(int y, int x1, int x2, bool filled)\n"
	"{\n"
	"    if ((y < ellipse_clip.y) || (y > ellipse_clip.w)) return;\n"
	"    if (filled) {\n"
	"        int first = max(x1, ellipse_clip.x);\n"
	"        int last = min(x2, ellipse_clip.z);\n"
	"        for (int x = first + int(gl_LocalInvocationID.x); x <= last;\n"
	"             x += int(gl_WorkGroupSize.x))\n"
	"            imageAtomicMax(winner_image, ivec2(x, y), ellipse_key);\n"
	"    } else if (gl_LocalInvocationID.x == 0u) {\n"
	"        if ((x1 >= ellipse_clip.x) && (x1 <= ellipse_clip.z))\n"
	"            imageAtomicMax(winner_image, ivec2(x1, y), ellipse_key);\n"
	"        if ((x2 != x1) && (x2 >= ellipse_clip.x) && (x2 <= ellipse_clip.z))\n"
	"            imageAtomicMax(winner_image, ivec2(x2, y), ellipse_key);\n"
	"    }\n"
	"}\n"
	"void select_line(ivec4 geometry, ivec4 clip, ivec4 metadata)\n"
	"{\n"
	"    uint index = gl_WorkGroupID.x * gl_WorkGroupSize.x +\n"
	"        gl_LocalInvocationID.x;\n"
	"    ivec2 start = geometry.xy; ivec2 finish = geometry.zw;\n"
	"    ivec2 difference = abs(finish - start);\n"
	"    uint count = uint(max(difference.x, difference.y)) + 1u;\n"
	"    if (index >= count) return;\n"
	"    ivec2 direction = ivec2((finish.x < start.x) ? -1 : 1,\n"
	"        (finish.y < start.y) ? -1 : 1);\n"
	"    int step_index = int(index); ivec2 position;\n"
	"    if (difference.x >= difference.y) {\n"
	"        position.x = start.x + direction.x * step_index;\n"
	"        position.y = start.y;\n"
	"        if (difference.x != 0) position.y += direction.y *\n"
	"            ((difference.y * step_index + difference.x / 2) / difference.x);\n"
	"    } else {\n"
	"        position.y = start.y + direction.y * step_index;\n"
	"        position.x = start.x;\n"
	"        if (difference.y != 0) position.x += direction.x *\n"
	"            ((difference.x * step_index + difference.y / 2) / difference.y);\n"
	"    }\n"
	"    if ((position.x < clip.x) || (position.y < clip.y) ||\n"
	"        (position.x > clip.z) || (position.y > clip.w)) return;\n"
	"    uint style = uint(metadata.z);\n"
	"    if ((style & (0x8000u >> (index & 15u))) == 0u) return;\n"
	"    imageAtomicMax(winner_image, position,\n"
	"        operation_batch_key + uint(metadata.w));\n"
	"}\n"
	"void select_ellipse(ivec4 geometry, ivec4 clip, ivec4 metadata)\n"
	"{\n"
	"    if (gl_WorkGroupID.x != 0u) return;\n"
	"    float radius_x = uintBitsToFloat(uint(geometry.z));\n"
	"    float radius_y = uintBitsToFloat(uint(geometry.w));\n"
	"    bool filled = metadata.z != 0;\n"
	"    if (gl_LocalInvocationID.x == 0u) {\n"
	"        ellipse_x1 = int(float(geometry.x) - radius_x);\n"
	"        ellipse_x2 = int(float(geometry.x) + radius_x);\n"
	"        ellipse_y1 = geometry.y; ellipse_y2 = geometry.y;\n"
	"        ellipse_clip = clip;\n"
	"        ellipse_key = operation_batch_key + uint(metadata.w);\n"
	"        ellipse_d = int(radius_x);\n"
	"        ellipse_aq = trunc(double(radius_x * radius_x));\n"
	"        ellipse_bq = trunc(double(radius_y * radius_y));\n"
	"        ellipse_dx = ellipse_aq * 2.0; ellipse_dy = ellipse_bq * 2.0;\n"
	"        ellipse_r = trunc(double(radius_x * float(ellipse_bq)));\n"
	"        ellipse_rx = ellipse_r * 2.0; ellipse_ry = 0.0;\n"
	"    }\n"
	"    barrier();\n"
	"    if (radius_y == 0.0) {\n"
	"        select_ellipse_row(ellipse_y1, ellipse_x1, ellipse_x2, true);\n"
	"        return;\n"
	"    }\n"
	"    select_ellipse_row(ellipse_y1, ellipse_x1, ellipse_x2, filled);\n"
	"    while (ellipse_d > 0) {\n"
	"        if (gl_LocalInvocationID.x == 0u) {\n"
	"            if (ellipse_r > 0.0) {\n"
	"                ++ellipse_y1; --ellipse_y2;\n"
	"                ellipse_ry += ellipse_dx; ellipse_r -= ellipse_ry;\n"
	"            }\n"
	"            if (ellipse_r <= 0.0) {\n"
	"                --ellipse_d; ++ellipse_x1; --ellipse_x2;\n"
	"                ellipse_rx -= ellipse_dy; ellipse_r += ellipse_rx;\n"
	"            }\n"
	"        }\n"
	"        barrier();\n"
	"        select_ellipse_row(ellipse_y1, ellipse_x1, ellipse_x2, filled);\n"
	"        if (ellipse_y2 != ellipse_y1)\n"
	"            select_ellipse_row(ellipse_y2, ellipse_x1, ellipse_x2, filled);\n"
	"        barrier();\n"
	"    }\n"
	"}\n"
	"void select_point(ivec4 geometry, ivec4 clip, ivec4 metadata)\n"
	"{\n"
	"    if ((gl_WorkGroupID.x != 0u) ||\n"
	"        (gl_LocalInvocationID.x != 0u)) return;\n"
	"    ivec2 position = geometry.xy;\n"
	"    if ((position.x < clip.x) || (position.y < clip.y) ||\n"
	"        (position.x > clip.z) || (position.y > clip.w)) return;\n"
	"    imageAtomicMax(winner_image, position,\n"
	"        operation_batch_key + uint(metadata.w));\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    int base = int(gl_WorkGroupID.y) * 3;\n"
	"    ivec4 geometry = command_data[base];\n"
	"    ivec4 clip = command_data[base + 1];\n"
	"    ivec4 metadata = command_data[base + 2];\n"
	"    if (metadata.y == 1) select_line(geometry, clip, metadata);\n"
	"    else if (metadata.y == 2) select_ellipse(geometry, clip, metadata);\n"
	"    else if (metadata.y == 3) select_point(geometry, clip, metadata);\n"
	"}\n";

static const char opengl_primitive_batch_resolve_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 16, local_size_y = 16) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D destination_image;\n"
	"layout(r32ui, binding = 1) uniform readonly uimage2D winner_image;\n"
	"layout(std430, binding = 2) readonly buffer primitive_commands {\n"
	"    ivec4 command_data[];\n"
	"};\n"
	"uniform ivec4 operation_resolve_rect;\n"
	"uniform uint operation_batch_generation;\n"
	"uniform uint operation_mask;\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 offset = ivec2(gl_GlobalInvocationID.xy);\n"
	"    if ((offset.x >= operation_resolve_rect.z) ||\n"
	"        (offset.y >= operation_resolve_rect.w)) return;\n"
	"    ivec2 position = operation_resolve_rect.xy + offset;\n"
	"    uint winner = imageLoad(winner_image, position).r;\n"
	"    if ((winner >> 13) != operation_batch_generation) return;\n"
	"    uint command_index = (winner & 8191u) - 1u;\n"
	"    uint color = uint(command_data[command_index * 3u + 2u].x);\n"
	"    imageStore(destination_image, position,\n"
	"        uvec4(color & operation_mask, 0u, 0u, 0u));\n"
	"}\n";

/*
	The common box-enclosed PAINT is discovered, verified, and filled in separate
	dispatches. The verification and write phases use a 16 by 16 grid across the
	clipped surface. Irregular regions use one seed per contiguous scanline run:
	lane zero owns the exact queue and four-neighbour topology, while all 256
	lanes mark and write the discovered span before the adjacent rows are tested.
	Scratch states distinguish queued pixels from completed spans, preventing the
	old per-pixel queue expansion without making the result scheduling-dependent.
*/
static const char opengl_paint_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 16, local_size_y = 16) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D target_image;\n"
	"layout(std430, binding = 1) buffer PaintScratch { uint entries[]; };\n"
	"uniform ivec4 operation_seed; uniform ivec4 operation_clip;\n"
	"uniform uint operation_color; uniform uint operation_border;\n"
	"uniform uint operation_mask; uniform uint operation_flags;\n"
	"uniform uint operation_mode; uniform uint operation_pattern_size;\n"
	"uniform ivec4 operation_pattern_origin; uniform uint operation_bytes_per_pixel;\n"
	"uniform uint operation_pattern[64];\n"
	"uniform uint operation_phase;\n"
	"shared uint queue_head; shared uint queue_tail; shared uint span_active;\n"
	"shared int span_y; shared int span_left; shared int span_right;\n"
	"uint alpha_primitive(uint source, uint destination)\n"
	"{ uint a = source >> 24; uint srb = source & 0x00FF00FFu; uint sg = source & 0x0000FF00u; uint drb = destination & 0x00FF00FFu; uint dg = destination & 0x0000FF00u; srb = ((srb - drb) * a) >> 8; sg = ((sg - dg) * a) >> 8; return ((drb + srb) & 0x00FF00FFu) | ((dg + sg) & 0x0000FF00u) | (source & 0xFF000000u); }\n"
	"uint pattern_byte(uint offset) { return (operation_pattern[offset >> 2u] >> ((offset & 3u) * 8u)) & 255u; }\n"
	"uint pattern_color(ivec2 position)\n"
	"{ uint offset = (((uint(position.y) & 7u) * 8u) + (uint(position.x) & 7u)) * operation_bytes_per_pixel; uint color = 0u; if ((offset + operation_bytes_per_pixel) > operation_pattern_size) return 0u; for (uint index = 0u; index < operation_bytes_per_pixel; ++index) color |= pattern_byte(offset + index) << (index * 8u); return color; }\n"
	"void write_pixel(ivec2 position)\n"
	"{ uint source = (operation_mode != 0u) ? pattern_color(position) : (operation_color & operation_mask); if (operation_mode != 0u || (operation_flags & 1u) == 0u) { imageStore(target_image, position, uvec4(source, 0u, 0u, 0u)); return; } imageStore(target_image, position, uvec4(alpha_primitive(source, imageLoad(target_image, position).r), 0u, 0u, 0u)); }\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 dimensions = imageSize(target_image); int width = dimensions.x;\n"
	"    uint total = uint(width * dimensions.y); uint lane = gl_LocalInvocationIndex;\n"
	"    uint lanes = gl_WorkGroupSize.x * gl_WorkGroupSize.y;\n"
	"    uint clip_width = uint(operation_clip.z - operation_clip.x + 1);\n"
	"    uint clip_height = uint(operation_clip.w - operation_clip.y + 1);\n"
	"    uint clip_total = clip_width * clip_height;\n"
	"    uint metadata = total * 2u;\n"
	"    if (operation_phase == 1u) {\n"
	"      if (lane == 0u) {\n"
	"        entries[metadata] = 0u;\n"
	"        if ((operation_seed.x >= operation_clip.x) && (operation_seed.y >= operation_clip.y) &&\n"
	"            (operation_seed.x <= operation_clip.z) && (operation_seed.y <= operation_clip.w) &&\n"
	"            (operation_seed.x >= 0) && (operation_seed.y >= 0) &&\n"
	"            (operation_seed.x < dimensions.x) && (operation_seed.y < dimensions.y) &&\n"
	"            (imageLoad(target_image, operation_seed.xy).r != operation_border)) {\n"
	"            int left = operation_seed.x; int right = operation_seed.x;\n"
	"            int top = operation_seed.y; int bottom = operation_seed.y;\n"
	"            while ((left > operation_clip.x) &&\n"
	"                (imageLoad(target_image, ivec2(left - 1, operation_seed.y)).r != operation_border)) --left;\n"
	"            while ((right < operation_clip.z) &&\n"
	"                (imageLoad(target_image, ivec2(right + 1, operation_seed.y)).r != operation_border)) ++right;\n"
	"            while ((top > operation_clip.y) &&\n"
	"                (imageLoad(target_image, ivec2(operation_seed.x, top - 1)).r != operation_border)) --top;\n"
	"            while ((bottom < operation_clip.w) &&\n"
	"                (imageLoad(target_image, ivec2(operation_seed.x, bottom + 1)).r != operation_border)) ++bottom;\n"
	"            entries[metadata] = 1u; entries[metadata + 1u] = uint(left);\n"
	"            entries[metadata + 2u] = uint(right); entries[metadata + 3u] = uint(top);\n"
	"            entries[metadata + 4u] = uint(bottom);\n"
	"        }\n"
	"      }\n"
	"      return;\n"
	"    }\n"
	"    if ((operation_phase == 2u) || (operation_phase == 3u)) {\n"
	"        if (entries[metadata] == 0u) return;\n"
	"        int left = int(entries[metadata + 1u]); int right = int(entries[metadata + 2u]);\n"
	"        int top = int(entries[metadata + 3u]); int bottom = int(entries[metadata + 4u]);\n"
	"        ivec2 position = operation_clip.xy + ivec2(gl_GlobalInvocationID.xy);\n"
	"        if ((position.x > operation_clip.z) || (position.y > operation_clip.w) ||\n"
	"            (position.x < left) || (position.x > right) ||\n"
	"            (position.y < top) || (position.y > bottom)) return;\n"
	"        if (operation_phase == 2u) {\n"
	"            if (imageLoad(target_image, position).r == operation_border)\n"
	"                atomicAnd(entries[metadata], 0u);\n"
	"            if ((position.y == top) && (top > operation_clip.y) &&\n"
	"                (imageLoad(target_image, position + ivec2(0, -1)).r != operation_border)) atomicAnd(entries[metadata], 0u);\n"
	"            if ((position.y == bottom) && (bottom < operation_clip.w) &&\n"
	"                (imageLoad(target_image, position + ivec2(0, 1)).r != operation_border)) atomicAnd(entries[metadata], 0u);\n"
	"            if ((position.x == left) && (left > operation_clip.x) &&\n"
	"                (imageLoad(target_image, position + ivec2(-1, 0)).r != operation_border)) atomicAnd(entries[metadata], 0u);\n"
	"            if ((position.x == right) && (right < operation_clip.z) &&\n"
	"                (imageLoad(target_image, position + ivec2(1, 0)).r != operation_border)) atomicAnd(entries[metadata], 0u);\n"
	"            return;\n"
	"        }\n"
	"        write_pixel(position);\n"
	"        return;\n"
	"    }\n"
	"    if ((operation_phase != 4u) || (entries[metadata] != 0u)) return;\n"
	"    for (uint offset = lane; offset < clip_total; offset += lanes) {\n"
	"        uint x = uint(operation_clip.x) + (offset % clip_width);\n"
	"        uint y = uint(operation_clip.y) + (offset / clip_width);\n"
	"        entries[y * uint(width) + x] = 0u;\n"
	"    }\n"
	"    memoryBarrierBuffer(); barrier();\n"
	"    if (lane == 0u) {\n"
	"        queue_head = 0u; queue_tail = 0u; span_active = 0u;\n"
	"        if ((operation_seed.x >= operation_clip.x) && (operation_seed.y >= operation_clip.y) &&\n"
	"            (operation_seed.x <= operation_clip.z) && (operation_seed.y <= operation_clip.w) &&\n"
	"            (operation_seed.x >= 0) && (operation_seed.y >= 0) &&\n"
	"            (operation_seed.x < dimensions.x) && (operation_seed.y < dimensions.y)) {\n"
	"            uint seed = uint(operation_seed.y * width + operation_seed.x);\n"
	"            if (imageLoad(target_image, operation_seed.xy).r != operation_border) {\n"
	"                entries[seed] = 1u; entries[total] = seed; queue_tail = 1u; span_active = 1u;\n"
	"            }\n"
	"        }\n"
	"    }\n"
	"    barrier(); if (span_active == 0u) return;\n"
	"    for (;;) {\n"
	"        if (lane == 0u) {\n"
	"            span_active = 0u;\n"
	"            while ((queue_head < queue_tail) && (span_active == 0u)) {\n"
	"                uint pixel = entries[total + queue_head++];\n"
	"                int y = int(pixel / uint(width)); int x = int(pixel % uint(width));\n"
	"                if ((entries[pixel] == 2u) ||\n"
	"                    (imageLoad(target_image, ivec2(x, y)).r == operation_border)) continue;\n"
	"                int left = x; int right = x;\n"
	"                while (left > operation_clip.x) { uint candidate = uint(y * width + left - 1);\n"
	"                    if ((entries[candidate] == 2u) ||\n"
	"                        (imageLoad(target_image, ivec2(left - 1, y)).r == operation_border)) break; --left; }\n"
	"                while (right < operation_clip.z) { uint candidate = uint(y * width + right + 1);\n"
	"                    if ((entries[candidate] == 2u) ||\n"
	"                        (imageLoad(target_image, ivec2(right + 1, y)).r == operation_border)) break; ++right; }\n"
	"                span_y = y; span_left = left; span_right = right; span_active = 1u;\n"
	"            }\n"
	"        }\n"
	"        barrier(); if (span_active == 0u) break;\n"
	"        for (int scan_x = span_left + int(lane); scan_x <= span_right;\n"
	"             scan_x += int(lanes)) {\n"
	"            uint candidate = uint(span_y * width + scan_x); entries[candidate] = 2u;\n"
	"            write_pixel(ivec2(scan_x, span_y));\n"
	"        }\n"
	"        memoryBarrierBuffer(); memoryBarrierImage(); barrier();\n"
	"        if (lane == 0u) {\n"
	"            for (int direction = -1; direction <= 1; direction += 2) {\n"
	"                int scan_y = span_y + direction; bool in_run = false;\n"
	"                if ((scan_y < operation_clip.y) || (scan_y > operation_clip.w)) continue;\n"
	"                for (int scan_x = span_left; scan_x <= span_right; ++scan_x) {\n"
	"                    uint candidate = uint(scan_y * width + scan_x); uint state = entries[candidate];\n"
	"                    bool fillable = (state != 2u) &&\n"
	"                        (imageLoad(target_image, ivec2(scan_x, scan_y)).r != operation_border);\n"
	"                    if (!fillable) { in_run = false; continue; }\n"
	"                    if (!in_run && (state == 0u)) entries[total + queue_tail++] = candidate;\n"
	"                    in_run = true; if (state == 0u) entries[candidate] = 1u;\n"
	"                }\n"
	"            }\n"
	"        }\n"
	"        memoryBarrierBuffer(); barrier();\n"
	"    }\n"
	"}\n";

static const char opengl_blit_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 16, local_size_y = 16) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D destination_image;\n"
	"layout(r32ui, binding = 1) uniform readonly uimage2D source_image;\n"
	"uniform ivec4 operation_source_rect;\n"
	"uniform ivec4 operation_clip;\n"
	"uniform int operation_destination_x;\n"
	"uniform int operation_destination_y;\n"
	"uniform uint operation_mode;\n"
	"uniform uint operation_alpha;\n"
	"uniform uint operation_depth;\n"
	"uniform uint operation_mask;\n"
	"bool blend_pixel(uint source, uint destination, out uint result_color)\n"
	"{\n"
	"    uint alpha; uint srb; uint sga; uint drb; uint dga;\n"
	"    uint temporary1; uint temporary2; uint overflow;\n"
	"    if (operation_mode == 1u) { result_color = source & operation_mask; }\n"
	"    else if (operation_mode == 2u) {\n"
	"        result_color = (~source) & operation_mask;\n"
	"    } else if (operation_mode == 3u) {\n"
	"        result_color = (source & destination) & operation_mask;\n"
	"    } else if (operation_mode == 4u) {\n"
	"        result_color = (source | destination) & operation_mask;\n"
	"    } else if (operation_mode == 5u) {\n"
	"        result_color = (source ^ destination) & operation_mask;\n"
	"    } else if (operation_mode == 0u) {\n"
	"        if (operation_depth <= 8u) {\n"
	"            result_color = source & operation_mask;\n"
	"            return result_color != 0u;\n"
	"        } else if (operation_depth == 16u) {\n"
	"            result_color = source & 0xFFFFu;\n"
	"            return result_color != 0xF81Fu;\n"
	"        }\n"
	"        result_color = source & 0x00FFFFFFu;\n"
	"        return result_color != 0x00FF00FFu;\n"
	"    } else if (operation_mode == 6u) {\n"
	"        if (operation_depth != 32u) {\n"
	"            result_color = source & operation_mask; return true;\n"
	"        }\n"
	"        alpha = (source >> 24) + 1u;\n"
	"        srb = source & 0x00FF00FFu; sga = source & 0xFF00FF00u;\n"
	"        drb = destination & 0x00FF00FFu;\n"
	"        dga = destination & 0xFF00FF00u;\n"
	"        srb = ((srb - drb) * alpha) >> 8;\n"
	"        sga = ((sga >> 8) - (dga >> 8)) * alpha;\n"
	"        result_color = ((drb + srb) & 0x00FF00FFu) |\n"
	"            ((dga + sga) & 0xFF00FF00u);\n"
	"    } else if (operation_mode == 7u) {\n"
	"        alpha = operation_alpha & 0xFFu;\n"
	"        if (operation_depth <= 8u) {\n"
	"            result_color = (source | destination) & operation_mask;\n"
	"        } else if (operation_depth == 16u) {\n"
	"            if ((source & 0xFFFFu) == 0xF81Fu) return false;\n"
	"            alpha = (alpha + 7u) >> 3;\n"
	"            source = ((source << 16) | source) & 0x07C0F81Fu;\n"
	"            source = ((source * alpha) >> 5) & 0x07C0F81Fu;\n"
	"            destination = ((destination << 16) | destination) &\n"
	"                0x07C0F81Fu;\n"
	"            source += destination; overflow = source & 0x08010020u;\n"
	"            overflow -= overflow >> 5; source |= overflow;\n"
	"            source &= 0x07C0F81Fu; source |= source >> 16;\n"
	"            result_color = source & 0xFFFFu;\n"
	"        } else {\n"
	"            if ((source & 0x00FFFFFFu) == 0x00FF00FFu) return false;\n"
	"            temporary1 = source & 0x00FF00FFu;\n"
	"            temporary2 = (source >> 8) & 0x00FF00FFu;\n"
	"            temporary1 = ((temporary1 * alpha) >> 8) & 0x00FF00FFu;\n"
	"            temporary2 = (temporary2 * alpha) & 0xFF00FF00u;\n"
	"            source = temporary1 | temporary2;\n"
	"            temporary1 = source & 0x80808080u;\n"
	"            temporary2 = destination & 0x80808080u;\n"
	"            source = (source & 0x7F7F7F7Fu) +\n"
	"                (destination & 0x7F7F7F7Fu);\n"
	"            destination = temporary1; temporary1 |= temporary2;\n"
	"            temporary2 = destination & temporary2;\n"
	"            destination = temporary1 & source;\n"
	"            source |= ((((temporary2 | destination) >> 7) +\n"
	"                0x7F7F7F7Fu) ^ 0x7F7F7F7Fu) | temporary1;\n"
	"            result_color = source;\n"
	"        }\n"
	"    } else if (operation_mode == 9u) {\n"
	"        alpha = operation_alpha & 0xFFu; if (alpha == 0u) return false;\n"
	"        if (operation_depth <= 8u) {\n"
	"            result_color = source & operation_mask;\n"
	"            return result_color != 0u;\n"
	"        } else if (operation_depth == 16u) {\n"
	"            if ((source & 0xFFFFu) == 0xF81Fu) return false;\n"
	"            alpha = (alpha + 7u) >> 3;\n"
	"            srb = source & 0xF81Fu; sga = source & 0x07E0u;\n"
	"            drb = destination & 0xF81Fu; dga = destination & 0x07E0u;\n"
	"            srb = ((srb - drb) * alpha) >> 5;\n"
	"            sga = ((sga - dga) * alpha) >> 5;\n"
	"            result_color = ((drb + srb) & 0xF81Fu) |\n"
	"                ((dga + sga) & 0x07E0u);\n"
	"        } else {\n"
	"            if ((source & 0x00FFFFFFu) == 0x00FF00FFu) return false;\n"
	"            alpha++; srb = source & 0x00FF00FFu;\n"
	"            sga = source & 0xFF00FF00u;\n"
	"            drb = destination & 0x00FF00FFu;\n"
	"            dga = destination & 0xFF00FF00u;\n"
	"            srb = ((srb - drb) * alpha) >> 8;\n"
	"            sga = ((sga >> 8) - (dga >> 8)) * alpha;\n"
	"            result_color = ((drb + srb) & 0x00FF00FFu) |\n"
	"                ((dga + sga) & 0xFF00FF00u);\n"
	"        }\n"
	"    } else { return false; }\n"
	"    return true;\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 offset = ivec2(gl_GlobalInvocationID.xy);\n"
	"    if ((offset.x >= operation_source_rect.z) ||\n"
	"        (offset.y >= operation_source_rect.w)) return;\n"
	"    ivec2 destination = ivec2(operation_destination_x,\n"
	"        operation_destination_y) + offset;\n"
	"    if ((destination.x < operation_clip.x) ||\n"
	"        (destination.y < operation_clip.y) ||\n"
	"        (destination.x > operation_clip.z) ||\n"
	"        (destination.y > operation_clip.w)) return;\n"
	"    uint source = imageLoad(source_image,\n"
	"        operation_source_rect.xy + offset).r;\n"
	"    uint old_destination = imageLoad(destination_image, destination).r;\n"
	"    uint result_color;\n"
	"    if (blend_pixel(source, old_destination, result_color))\n"
	"        imageStore(destination_image, destination,\n"
	"            uvec4(result_color, 0u, 0u, 0u));\n"
	"}\n";

/*
	Scaling, rotation, and Mode 7 share one inverse-mapped compute shader. Each
	destination pixel is independent, so coverage and sampling scale across the
	GPU without generating transformed geometry or source pixels on the CPU.
*/
static const char opengl_transform_blit_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 16, local_size_y = 16) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D destination_image;\n"
	"layout(r32ui, binding = 1) uniform readonly uimage2D source_image;\n"
	"uniform ivec4 operation_source_rect; uniform ivec4 operation_clip;\n"
	"uniform ivec4 operation_bounds; uniform mat3 operation_inverse;\n"
	"uniform uint operation_mode; uniform uint operation_alpha;\n"
	"uniform uint operation_depth; uniform uint operation_mask;\n"
	"uniform uint operation_filter; uniform uint operation_wrap;\n"
	"ivec2 source_coordinate(ivec2 p)\n"
	"{\n"
	"    ivec2 origin = operation_source_rect.xy;\n"
	"    ivec2 size = operation_source_rect.zw - origin + ivec2(1);\n"
	"    if (operation_wrap != 0u) { ivec2 relative = (p - origin) % size;\n"
	"        relative = (relative + size) % size; return origin + relative; }\n"
	"    return clamp(p, origin, operation_source_rect.zw);\n"
	"}\n"
	"uint source_pixel(ivec2 p)\n"
	"{ return imageLoad(source_image, source_coordinate(p)).r; }\n"
	"uvec4 unpack_color(uint color)\n"
	"{\n"
	"    if (operation_depth == 16u) return uvec4(\n"
	"        ((color >> 11) & 31u) * 255u / 31u,\n"
	"        ((color >> 5) & 63u) * 255u / 63u,\n"
	"        (color & 31u) * 255u / 31u, 255u);\n"
	"    return uvec4((color >> 16) & 255u, (color >> 8) & 255u,\n"
	"        color & 255u, (color >> 24) & 255u);\n"
	"}\n"
	"uint pack_color(uvec4 color)\n"
	"{\n"
	"    if (operation_depth == 16u) return ((color.r * 31u / 255u) << 11) |\n"
	"        ((color.g * 63u / 255u) << 5) | (color.b * 31u / 255u);\n"
	"    return (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;\n"
	"}\n"
	"uint sample_source(vec2 coordinate)\n"
	"{\n"
	"    if ((operation_filter == 0u) || (operation_depth <= 8u))\n"
	"        return source_pixel(ivec2(floor(coordinate)));\n"
	"    vec2 texel = coordinate - vec2(0.5); ivec2 base = ivec2(floor(texel));\n"
	"    vec2 fraction = fract(texel);\n"
	"    vec4 top = mix(vec4(unpack_color(source_pixel(base))),\n"
	"        vec4(unpack_color(source_pixel(base + ivec2(1, 0)))), fraction.x);\n"
	"    vec4 bottom = mix(vec4(unpack_color(source_pixel(base + ivec2(0, 1)))),\n"
	"        vec4(unpack_color(source_pixel(base + ivec2(1, 1)))), fraction.x);\n"
	"    return pack_color(uvec4(round(mix(top, bottom, fraction.y))));\n"
	"}\n"
	"bool blend_pixel(uint source, uint destination, out uint result_color)\n"
	"{\n"
	"    uint alpha; uint srb; uint sga; uint drb; uint dga;\n"
	"    uint temporary1; uint temporary2; uint overflow;\n"
	"    if (operation_mode == 1u) result_color = source & operation_mask;\n"
	"    else if (operation_mode == 2u) result_color = (~source) & operation_mask;\n"
	"    else if (operation_mode == 3u) result_color = (source & destination) & operation_mask;\n"
	"    else if (operation_mode == 4u) result_color = (source | destination) & operation_mask;\n"
	"    else if (operation_mode == 5u) result_color = (source ^ destination) & operation_mask;\n"
	"    else if (operation_mode == 0u) { result_color = source & operation_mask;\n"
	"        if (operation_depth <= 8u) return result_color != 0u;\n"
	"        if (operation_depth == 16u) return result_color != 0xF81Fu;\n"
	"        return (result_color & 0x00FFFFFFu) != 0x00FF00FFu;\n"
	"    } else if (operation_mode == 6u) {\n"
	"        if (operation_depth != 32u) { result_color = source & operation_mask; return true; }\n"
	"        alpha = (source >> 24) + 1u; srb = source & 0x00FF00FFu;\n"
	"        sga = source & 0xFF00FF00u; drb = destination & 0x00FF00FFu;\n"
	"        dga = destination & 0xFF00FF00u; srb = ((srb - drb) * alpha) >> 8;\n"
	"        sga = ((sga >> 8) - (dga >> 8)) * alpha;\n"
	"        result_color = ((drb + srb) & 0x00FF00FFu) | ((dga + sga) & 0xFF00FF00u);\n"
	"    } else if (operation_mode == 7u) {\n"
	"        alpha = operation_alpha & 255u;\n"
	"        if (operation_depth <= 8u) result_color = (source | destination) & operation_mask;\n"
	"        else if (operation_depth == 16u) {\n"
	"            if ((source & 0xFFFFu) == 0xF81Fu) return false;\n"
	"            alpha = (alpha + 7u) >> 3; source = ((source << 16) | source) & 0x07C0F81Fu;\n"
	"            source = ((source * alpha) >> 5) & 0x07C0F81Fu;\n"
	"            destination = ((destination << 16) | destination) & 0x07C0F81Fu;\n"
	"            source += destination; overflow = source & 0x08010020u; overflow -= overflow >> 5;\n"
	"            source |= overflow; source &= 0x07C0F81Fu; result_color = (source | (source >> 16)) & 0xFFFFu;\n"
	"        } else {\n"
	"            if ((source & 0x00FFFFFFu) == 0x00FF00FFu) return false;\n"
	"            temporary1 = source & 0x00FF00FFu; temporary2 = (source >> 8) & 0x00FF00FFu;\n"
	"            temporary1 = ((temporary1 * alpha) >> 8) & 0x00FF00FFu;\n"
	"            temporary2 = (temporary2 * alpha) & 0xFF00FF00u; source = temporary1 | temporary2;\n"
	"            temporary1 = source & 0x80808080u; temporary2 = destination & 0x80808080u;\n"
	"            source = (source & 0x7F7F7F7Fu) + (destination & 0x7F7F7F7Fu);\n"
	"            destination = temporary1; temporary1 |= temporary2; temporary2 = destination & temporary2;\n"
	"            destination = temporary1 & source; source |= ((((temporary2 | destination) >> 7) +\n"
	"                0x7F7F7F7Fu) ^ 0x7F7F7F7Fu) | temporary1; result_color = source;\n"
	"        }\n"
	"    } else if (operation_mode == 9u) {\n"
	"        alpha = operation_alpha & 255u; if (alpha == 0u) return false;\n"
	"        if (operation_depth <= 8u) { result_color = source & operation_mask; return result_color != 0u; }\n"
	"        if (operation_depth == 16u) {\n"
	"            if ((source & 0xFFFFu) == 0xF81Fu) return false; alpha = (alpha + 7u) >> 3;\n"
	"            srb = source & 0xF81Fu; sga = source & 0x07E0u; drb = destination & 0xF81Fu;\n"
	"            dga = destination & 0x07E0u; srb = ((srb - drb) * alpha) >> 5;\n"
	"            sga = ((sga - dga) * alpha) >> 5; result_color = ((drb + srb) & 0xF81Fu) |\n"
	"                ((dga + sga) & 0x07E0u);\n"
	"        } else {\n"
	"            if ((source & 0x00FFFFFFu) == 0x00FF00FFu) return false; alpha++;\n"
	"            srb = source & 0x00FF00FFu; sga = source & 0xFF00FF00u;\n"
	"            drb = destination & 0x00FF00FFu; dga = destination & 0xFF00FF00u;\n"
	"            srb = ((srb - drb) * alpha) >> 8; sga = ((sga >> 8) - (dga >> 8)) * alpha;\n"
	"            result_color = ((drb + srb) & 0x00FF00FFu) | ((dga + sga) & 0xFF00FF00u);\n"
	"        }\n"
	"    } else return false; return true;\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 offset = ivec2(gl_GlobalInvocationID.xy);\n"
	"    ivec2 extent = operation_bounds.zw - operation_bounds.xy + ivec2(1);\n"
	"    if (any(greaterThanEqual(offset, extent))) return;\n"
	"    ivec2 destination = operation_bounds.xy + offset;\n"
	"    if ((destination.x < operation_clip.x) || (destination.y < operation_clip.y) ||\n"
	"        (destination.x > operation_clip.z) || (destination.y > operation_clip.w)) return;\n"
	"    vec3 mapped = operation_inverse * vec3(vec2(destination) + vec2(0.5), 1.0);\n"
	"    if (mapped.z <= 0.000001) return; vec2 source_coordinate_value = mapped.xy / mapped.z;\n"
	"    if ((operation_wrap == 0u) && ((source_coordinate_value.x < float(operation_source_rect.x)) ||\n"
	"        (source_coordinate_value.y < float(operation_source_rect.y)) ||\n"
	"        (source_coordinate_value.x >= float(operation_source_rect.z + 1)) ||\n"
	"        (source_coordinate_value.y >= float(operation_source_rect.w + 1)))) return;\n"
	"    uint source = sample_source(source_coordinate_value);\n"
	"    uint old_destination = imageLoad(destination_image, destination).r; uint result_color;\n"
	"    if (blend_pixel(source, old_destination, result_color))\n"
	"        imageStore(destination_image, destination, uvec4(result_color & operation_mask, 0u, 0u, 0u));\n"
	"}\n";

/*
	One workgroup owns one destination tile. The CPU bins each PUT into
	the tiles it touches in command order, so each invocation can replay only
	the operations affecting its pixel without racing a neighbouring tile.
*/
static const char opengl_blit_alpha_tile_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 16, local_size_y = 16) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D destination_image;\n"
	"layout(r32ui, binding = 1) uniform readonly uimage2D source_image;\n"
	"layout(std430, binding = 2) readonly buffer TileRanges { uvec2 ranges[]; };\n"
	"layout(std430, binding = 3) readonly buffer TileIndices { uint indices[]; };\n"
	"layout(std430, binding = 4) readonly buffer CommandData { ivec4 data[]; };\n"
	"uniform uint operation_tiles_x;\n"
	"uniform uint operation_mode;\n"
	"uniform uint operation_alpha;\n"
	"uniform uint operation_depth;\n"
	"uniform uint operation_mask;\n"
	"bool blend_operation(uint source, uint destination, out uint result_color)\n"
	"{\n"
	"    uint alpha; uint srb; uint sga; uint drb; uint dga;\n"
	"    uint temporary1; uint temporary2; uint overflow;\n"
	"    if (operation_mode == 1u) result_color = source & operation_mask;\n"
	"    else if (operation_mode == 2u) result_color = (~source) & operation_mask;\n"
	"    else if (operation_mode == 3u) result_color = (source & destination) & operation_mask;\n"
	"    else if (operation_mode == 4u) result_color = (source | destination) & operation_mask;\n"
	"    else if (operation_mode == 5u) result_color = (source ^ destination) & operation_mask;\n"
	"    else if (operation_mode == 0u) {\n"
	"        if (operation_depth <= 8u) { result_color = source & operation_mask; return result_color != 0u; }\n"
	"        if (operation_depth == 16u) { result_color = source & 0xFFFFu; return result_color != 0xF81Fu; }\n"
	"        result_color = source & 0x00FFFFFFu; return result_color != 0x00FF00FFu;\n"
	"    } else if (operation_mode == 6u) {\n"
	"        if (operation_depth != 32u) { result_color = source & operation_mask; return true; }\n"
	"        alpha = (source >> 24) + 1u; srb = source & 0x00FF00FFu; sga = source & 0xFF00FF00u;\n"
	"        drb = destination & 0x00FF00FFu; dga = destination & 0xFF00FF00u;\n"
	"        srb = ((srb - drb) * alpha) >> 8; sga = ((sga >> 8) - (dga >> 8)) * alpha;\n"
	"        result_color = ((drb + srb) & 0x00FF00FFu) | ((dga + sga) & 0xFF00FF00u);\n"
	"    } else if (operation_mode == 7u) {\n"
	"        alpha = operation_alpha & 0xFFu;\n"
	"        if (operation_depth <= 8u) result_color = (source | destination) & operation_mask;\n"
	"        else if (operation_depth == 16u) {\n"
	"            if ((source & 0xFFFFu) == 0xF81Fu) return false; alpha = (alpha + 7u) >> 3;\n"
	"            source = ((source << 16) | source) & 0x07C0F81Fu; source = ((source * alpha) >> 5) & 0x07C0F81Fu;\n"
	"            destination = ((destination << 16) | destination) & 0x07C0F81Fu; source += destination;\n"
	"            overflow = source & 0x08010020u; overflow -= overflow >> 5; source |= overflow;\n"
	"            source &= 0x07C0F81Fu; source |= source >> 16; result_color = source & 0xFFFFu;\n"
	"        } else {\n"
	"            if ((source & 0x00FFFFFFu) == 0x00FF00FFu) return false;\n"
	"            temporary1 = source & 0x00FF00FFu; temporary2 = (source >> 8) & 0x00FF00FFu;\n"
	"            temporary1 = ((temporary1 * alpha) >> 8) & 0x00FF00FFu; temporary2 = (temporary2 * alpha) & 0xFF00FF00u;\n"
	"            source = temporary1 | temporary2; temporary1 = source & 0x80808080u; temporary2 = destination & 0x80808080u;\n"
	"            source = (source & 0x7F7F7F7Fu) + (destination & 0x7F7F7F7Fu); destination = temporary1; temporary1 |= temporary2;\n"
	"            temporary2 = destination & temporary2; destination = temporary1 & source;\n"
	"            result_color = source | ((((temporary2 | destination) >> 7) + 0x7F7F7F7Fu) ^ 0x7F7F7F7Fu) | temporary1;\n"
	"        }\n"
	"    } else if (operation_mode == 9u) {\n"
	"        alpha = operation_alpha & 0xFFu; if (alpha == 0u) return false;\n"
	"        if (operation_depth <= 8u) { result_color = source & operation_mask; return result_color != 0u; }\n"
	"        if (operation_depth == 16u) {\n"
	"            if ((source & 0xFFFFu) == 0xF81Fu) return false; alpha = (alpha + 7u) >> 3;\n"
	"            srb = source & 0xF81Fu; sga = source & 0x07E0u; drb = destination & 0xF81Fu; dga = destination & 0x07E0u;\n"
	"            srb = ((srb - drb) * alpha) >> 5; sga = ((sga - dga) * alpha) >> 5;\n"
	"            result_color = ((drb + srb) & 0xF81Fu) | ((dga + sga) & 0x07E0u);\n"
	"        } else {\n"
	"            if ((source & 0x00FFFFFFu) == 0x00FF00FFu) return false; alpha++;\n"
	"            srb = source & 0x00FF00FFu; sga = source & 0xFF00FF00u; drb = destination & 0x00FF00FFu; dga = destination & 0xFF00FF00u;\n"
	"            srb = ((srb - drb) * alpha) >> 8; sga = ((sga >> 8) - (dga >> 8)) * alpha;\n"
	"            result_color = ((drb + srb) & 0x00FF00FFu) | ((dga + sga) & 0xFF00FF00u);\n"
	"        }\n"
	"    } else return false;\n"
	"    return true;\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 position = ivec2(gl_GlobalInvocationID.xy);\n"
	"    ivec2 dimensions = imageSize(destination_image);\n"
	"    uint tile = gl_WorkGroupID.y * operation_tiles_x + gl_WorkGroupID.x;\n"
	"    if ((position.x >= dimensions.x) || (position.y >= dimensions.y)) return;\n"
	"    uvec2 range = ranges[tile]; uint destination = imageLoad(destination_image, position).r;\n"
	"    bool wrote = false;\n"
	"    for (uint offset = 0u; offset < range.y; ++offset) {\n"
	"        uint command = indices[range.x + offset]; uint base = command * 3u;\n"
	"        ivec4 source_rect = data[base]; ivec4 clip = data[base + 1u];\n"
	"        ivec4 target = data[base + 2u];\n"
	"        if ((position.x < clip.x) || (position.y < clip.y) ||\n"
	"            (position.x > clip.z) || (position.y > clip.w) ||\n"
	"            (position.x < target.x) || (position.y < target.y) ||\n"
	"            (position.x >= target.x + source_rect.z) ||\n"
	"            (position.y >= target.y + source_rect.w)) continue;\n"
	"        uint source = imageLoad(source_image, source_rect.xy + (position - target.xy)).r;\n"
	"        uint result_color; if (blend_operation(source, destination, result_color)) {\n"
	"            destination = result_color; wrote = true;\n"
	"        }\n"
	"    }\n"
	"    if (wrote) imageStore(destination_image, position, uvec4(destination, 0u, 0u, 0u));\n"
	"}\n";

/*
	The first batch pass records the highest ordered PSET or TRANS command for
	each destination pixel. The resolve pass then writes that winner, preserving
	legacy overlap semantics without one compute dispatch per sprite.
*/
static const char opengl_blit_batch_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 16, local_size_y = 16) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D destination_image;\n"
	"layout(r32ui, binding = 1) uniform readonly uimage2D source_image;\n"
	"layout(r32ui, binding = 2) uniform uimage2D winner_image;\n"
	"layout(std430, binding = 2) readonly buffer batch_commands {\n"
	"    ivec4 command_data[];\n"
	"};\n"
	"uniform uint operation_mode;\n"
	"uniform uint operation_depth;\n"
	"uniform uint operation_mask;\n"
	"uniform uint operation_batch_key;\n"
	"void main(void)\n"
	"{\n"
	"    int base = int(gl_WorkGroupID.z) * 3;\n"
	"    ivec4 source_rect = command_data[base];\n"
	"    ivec4 clip = command_data[base + 1];\n"
	"    ivec4 destination_offset = command_data[base + 2];\n"
	"    ivec2 offset = ivec2(gl_GlobalInvocationID.xy);\n"
	"    if ((offset.x >= source_rect.z) || (offset.y >= source_rect.w))\n"
	"        return;\n"
	"    ivec2 destination = destination_offset.xy + offset;\n"
	"    if ((destination.x < clip.x) || (destination.y < clip.y) ||\n"
	"        (destination.x > clip.z) || (destination.y > clip.w))\n"
	"        return;\n"
	"    uint source = imageLoad(source_image, source_rect.xy + offset).r;\n"
	"    uint result_color;\n"
	"    if (operation_mode == 1u) {\n"
	"        result_color = source & operation_mask;\n"
	"    } else if (operation_depth <= 8u) {\n"
	"        result_color = source & operation_mask;\n"
	"        if (result_color == 0u) return;\n"
	"    } else if (operation_depth == 16u) {\n"
	"        result_color = source & 0xFFFFu;\n"
	"        if (result_color == 0xF81Fu) return;\n"
	"    } else {\n"
	"        result_color = source & 0x00FFFFFFu;\n"
	"        if (result_color == 0x00FF00FFu) return;\n"
	"    }\n"
	"    imageAtomicMax(winner_image, destination,\n"
	"        operation_batch_key + gl_WorkGroupID.z);\n"
	"}\n";

static const char opengl_blit_batch_resolve_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 16, local_size_y = 16) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D destination_image;\n"
	"layout(r32ui, binding = 1) uniform readonly uimage2D source_image;\n"
	"layout(r32ui, binding = 2) uniform readonly uimage2D winner_image;\n"
	"layout(std430, binding = 2) readonly buffer batch_commands {\n"
	"    ivec4 command_data[];\n"
	"};\n"
	"uniform ivec4 operation_resolve_rect;\n"
	"uniform uint operation_batch_generation;\n"
	"uniform uint operation_mask;\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 offset = ivec2(gl_GlobalInvocationID.xy);\n"
	"    if ((offset.x >= operation_resolve_rect.z) ||\n"
	"        (offset.y >= operation_resolve_rect.w)) return;\n"
	"    ivec2 destination = operation_resolve_rect.xy + offset;\n"
	"    uint winner = imageLoad(winner_image, destination).r;\n"
	"    if ((winner >> 13) != operation_batch_generation) return;\n"
	"    int base = (int(winner & 8191u) - 1) * 3;\n"
	"    ivec4 source_rect = command_data[base];\n"
	"    ivec4 destination_offset = command_data[base + 2];\n"
	"    ivec2 source_coordinate = source_rect.xy +\n"
	"        (destination - destination_offset.xy);\n"
	"    uint source = imageLoad(source_image, source_coordinate).r;\n"
	"    imageStore(destination_image, destination,\n"
	"        uvec4(source & operation_mask, 0u, 0u, 0u));\n"
	"}\n";

/*
	Opaque filled rectangles have no destination read.  Selection atomically
	records the last BASIC command touching each pixel, then resolve performs one
	write.  This gives overlapping boxes their strict FIFO result without one GL
	dispatch per box.
*/
static const char opengl_rectangle_batch_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 16, local_size_y = 16) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D winner_image;\n"
	"layout(std430, binding = 2) readonly buffer rectangle_commands {\n"
	"    ivec4 command_box[];\n"
	"};\n"
	"uniform uint operation_batch_key;\n"
	"void main(void)\n"
	"{\n"
	"    ivec4 box = command_box[int(gl_WorkGroupID.z) * 2];\n"
	"    ivec2 offset = ivec2(gl_GlobalInvocationID.xy);\n"
	"    if ((offset.x >= (box.z - box.x + 1)) ||\n"
	"        (offset.y >= (box.w - box.y + 1))) return;\n"
	"    imageAtomicMax(winner_image, box.xy + offset,\n"
	"        operation_batch_key + gl_WorkGroupID.z);\n"
	"}\n";

static const char opengl_rectangle_batch_resolve_shader[] =
	"#version 430 core\n"
	"layout(local_size_x = 16, local_size_y = 16) in;\n"
	"layout(r32ui, binding = 0) uniform uimage2D destination_image;\n"
	"layout(r32ui, binding = 1) uniform readonly uimage2D winner_image;\n"
	"layout(std430, binding = 2) readonly buffer rectangle_commands {\n"
	"    ivec4 command_box[];\n"
	"};\n"
	"uniform ivec4 operation_resolve_rect;\n"
	"uniform uint operation_batch_generation;\n"
	"uniform uint operation_mask;\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 offset = ivec2(gl_GlobalInvocationID.xy);\n"
	"    if ((offset.x >= operation_resolve_rect.z) ||\n"
	"        (offset.y >= operation_resolve_rect.w)) return;\n"
	"    ivec2 position = operation_resolve_rect.xy + offset;\n"
	"    uint winner = imageLoad(winner_image, position).r;\n"
	"    if ((winner >> 13) != operation_batch_generation) return;\n"
	"    uint color = uint(command_box[(int(winner & 8191u) - 1) * 2 + 1].x);\n"
	"    imageStore(destination_image, position,\n"
	"        uvec4(color & operation_mask, 0u, 0u, 0u));\n"
	"}\n";

/*
	PSET, PRESET, and TRANS do not read their destination. AND, OR, and XOR use
	the matching fixed-function OpenGL logical operation. Raster order preserves
	overlap semantics without one compute dispatch per sprite.
*/
static const char opengl_blit_raster_batch_vertex_shader[] =
	"#version 430 core\n"
	"layout(location = 1) in ivec4 blit_source_rect;\n"
	"layout(location = 2) in ivec4 blit_clip;\n"
	"layout(location = 3) in ivec4 blit_destination;\n"
	"uniform vec2 operation_surface_size;\n"
	"flat out ivec2 raster_source_origin;\n"
	"flat out ivec2 raster_destination_origin;\n"
	"flat out ivec4 raster_clip;\n"
	"void main(void)\n"
	"{\n"
	"    const vec2 corner[6] = vec2[6](vec2(0.0, 0.0), vec2(1.0, 0.0),\n"
	"        vec2(0.0, 1.0), vec2(0.0, 1.0), vec2(1.0, 0.0), vec2(1.0, 1.0));\n"
	"    vec2 position = vec2(blit_destination.xy) +\n"
	"        (corner[gl_VertexID] * vec2(blit_source_rect.zw));\n"
	"    gl_Position = vec4((position / operation_surface_size) * 2.0 - 1.0,\n"
	"        0.0, 1.0);\n"
	"    raster_source_origin = blit_source_rect.xy;\n"
	"    raster_destination_origin = blit_destination.xy;\n"
	"    raster_clip = blit_clip;\n"
	"}\n";

static const char opengl_blit_raster_batch_fragment_shader[] =
	"#version 430 core\n"
	"layout(location = 0) out uint output_pixel;\n"
	"uniform usampler2D source_image;\n"
	"uniform uint operation_mode;\n"
	"uniform uint operation_depth;\n"
	"uniform uint operation_mask;\n"
	"flat in ivec2 raster_source_origin;\n"
	"flat in ivec2 raster_destination_origin;\n"
	"flat in ivec4 raster_clip;\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 destination = ivec2(gl_FragCoord.xy);\n"
	"    if ((destination.x < raster_clip.x) || (destination.y < raster_clip.y) ||\n"
	"        (destination.x > raster_clip.z) || (destination.y > raster_clip.w))\n"
	"        discard;\n"
	"    uint source = texelFetch(source_image, raster_source_origin +\n"
	"        (destination - raster_destination_origin), 0).r & operation_mask;\n"
	"    if (operation_mode == 0u) {\n"
	"        if (operation_depth <= 8u) { if (source == 0u) discard; }\n"
	"        else if (operation_depth == 16u) { if (source == 0xF81Fu) discard; }\n"
	"        else if ((source & 0x00FFFFFFu) == 0x00FF00FFu) discard;\n"
	"    }\n"
	"    else if (operation_mode == 2u) source = (~source) & operation_mask;\n"
	"    output_pixel = source;\n"
	"}\n";

/*
	Opaque filled boxes are ordinary integer framebuffer quads.  Raster primitive
	order provides the Basic last-writer rule directly, avoiding the expensive
	coverage winner texture needed by compute paths with overlapping workgroups.
*/
static const char opengl_rectangle_raster_batch_vertex_shader[] =
	"#version 430 core\n"
	"layout(location = 1) in ivec4 rectangle_box;\n"
	"layout(location = 2) in uint rectangle_color;\n"
	"uniform vec2 operation_surface_size;\n"
	"flat out uint raster_color;\n"
	"void main(void)\n"
	"{\n"
	"    const vec2 corner[6] = vec2[6](vec2(0.0, 0.0), vec2(1.0, 0.0),\n"
	"        vec2(0.0, 1.0), vec2(0.0, 1.0), vec2(1.0, 0.0), vec2(1.0, 1.0));\n"
	"    vec2 extent = vec2(rectangle_box.z - rectangle_box.x + 1,\n"
	"        rectangle_box.w - rectangle_box.y + 1);\n"
	"    vec2 position = vec2(rectangle_box.xy) +\n"
	"        (corner[gl_VertexID] * extent);\n"
	"    gl_Position = vec4((position / operation_surface_size) * 2.0 - 1.0,\n"
	"        0.0, 1.0);\n"
	"    raster_color = rectangle_color;\n"
	"}\n";

static const char opengl_rectangle_raster_batch_fragment_shader[] =
	"#version 430 core\n"
	"layout(location = 0) out uint output_pixel;\n"
	"uniform uint operation_mask;\n"
	"flat in uint raster_color;\n"
	"void main(void) { output_pixel = raster_color & operation_mask; }\n";

static const char opengl_present_vertex_shader[] =
	"#version 430 core\n"
	"out vec2 texture_coordinate;\n"
	"void main(void)\n"
	"{\n"
	"    const vec2 position[3] = vec2[3](vec2(-1.0, -1.0),\n"
	"        vec2(3.0, -1.0), vec2(-1.0, 3.0));\n"
	"    vec2 p = position[gl_VertexID];\n"
	"    gl_Position = vec4(p, 0.0, 1.0);\n"
	"    texture_coordinate = vec2((p.x + 1.0) * 0.5,\n"
	"        1.0 - ((p.y + 1.0) * 0.5));\n"
	"}\n";

static const char opengl_present_fragment_shader[] =
	"#version 430 core\n"
	"layout(location = 0) out vec4 output_color;\n"
	"in vec2 texture_coordinate;\n"
	"uniform usampler2D source_image;\n"
	"uniform uint operation_depth;\n"
	"uniform uint operation_palette[256];\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 image_size = textureSize(source_image, 0);\n"
	"    vec2 maximum = vec2(image_size - ivec2(1));\n"
	"    ivec2 coordinate = ivec2(clamp(texture_coordinate *\n"
	"        vec2(image_size), vec2(0.0), maximum));\n"
	"    uint pixel = texelFetch(source_image, coordinate, 0).r;\n"
	"    uint color;\n"
	"    if (operation_depth <= 8u) {\n"
	"        uint mask = (1u << operation_depth) - 1u;\n"
	"        color = operation_palette[pixel & mask];\n"
	"        output_color = vec4(float(color & 255u),\n"
	"            float((color >> 8) & 255u),\n"
	"            float((color >> 16) & 255u), 255.0) / 255.0;\n"
	"    } else if (operation_depth == 16u) {\n"
	"        output_color = vec4(float((pixel >> 11) & 31u) / 31.0,\n"
	"            float((pixel >> 5) & 63u) / 63.0,\n"
	"            float(pixel & 31u) / 31.0, 1.0);\n"
	"    } else {\n"
	"        output_color = vec4(float((pixel >> 16) & 255u),\n"
	"            float((pixel >> 8) & 255u), float(pixel & 255u),\n"
	"            255.0) / 255.0;\n"
	"    }\n"
	"}\n";

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

static int opengl_create_programs(FB_GFX3_OPENGL_STATE *state)
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

/* ------------------------------------------------------------------------- */
/* GPU surface storage and commands                                          */
/* ------------------------------------------------------------------------- */

static uint32_t opengl_color_mask(uint32_t depth)
{
	if (depth >= 32)
		return UINT32_MAX;
	return (1u << depth) - 1u;
}

static uint32_t opengl_bytes_per_pixel(uint32_t depth, GLenum *type)
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

static int opengl_check_error(FB_GFX3_OPENGL_STATE *state,
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

static int opengl_bind_color_framebuffer(FB_GFX3_OPENGL_STATE *state,
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

static int opengl_clip_rect(const FB_GFX3_OPENGL_SURFACE *surface,
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

static int opengl_surface_retain(FB_GFX3_OPENGL_STATE *state,
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

static int opengl_palette(FB_GFX3_OPENGL_STATE *state,
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

static int opengl_page_set(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_PAGE_SET_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *surface;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_PAGE_SET_COMMAND *)command->payload;
	result = opengl_surface_retain(state, command->target,
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

static int opengl_present_handle(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_HANDLE handle, uint64_t sequence)
{
	FB_GFX3_PRESENTATION_LAYOUT layout;
	FB_GFX3_OPENGL_SURFACE *surface;
	uint32_t width;
	uint32_t height;
	int viewport_y;
	int result;

	result = opengl_surface_retain(state, handle, sequence, &surface);
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
	result = opengl_check_error(state, "presentation draw");
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

static int opengl_present(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	if (fb_gfx3_command_payload_size(command) != 0)
		return FB_GFX3_INVALID;
	return opengl_present_handle(state, command->target,
		command->sequence);
}

static int opengl_window_title(FB_GFX3_OPENGL_STATE *state,
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

static int opengl_dispatch_clear(FB_GFX3_OPENGL_STATE *state,
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
		opengl_color_mask(surface->depth));
	state->gl.uniform_1ui(state->clear_flags_location,
		flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND);
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	return opengl_check_error(state, "clear compute dispatch");
}

static int opengl_surface_create(FB_GFX3_OPENGL_STATE *state,
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
	result = opengl_check_error(state, "surface texture allocation");
	if (result != FB_GFX3_OK) {
		opengl_surface_destroy(surface);
		return result;
	}

	full_rect.x1 = 0;
	full_rect.y1 = 0;
	full_rect.x2 = (int32_t)surface->width - 1;
	full_rect.y2 = (int32_t)surface->height - 1;
	result = opengl_dispatch_clear(state, surface, &full_rect,
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

static int opengl_surface_release(FB_GFX3_OPENGL_STATE *state,
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

static int opengl_surface_upload(FB_GFX3_OPENGL_STATE *state,
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
	result = opengl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	bytes_per_pixel = opengl_bytes_per_pixel(surface->depth, &type);
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
	result = opengl_check_error(state, "surface upload");

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int opengl_surface_download(FB_GFX3_OPENGL_STATE *state,
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
	result = opengl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	bytes_per_pixel = opengl_bytes_per_pixel(surface->depth, &type);
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
	result = opengl_bind_color_framebuffer(state, surface->texture);
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
	result = opengl_check_error(state, "surface download");

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int opengl_clear(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_CLEAR_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *surface;
	FB_GFX3_RECT clip;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_CLEAR_COMMAND *)command->payload;
	result = opengl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (opengl_clip_rect(surface, &payload->clip, &clip))
		result = opengl_dispatch_clear(state, surface, &clip, payload->color,
			payload->flags);
	else
		result = FB_GFX3_OK;
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int opengl_points(FB_GFX3_OPENGL_STATE *state,
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

	result = opengl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!opengl_clip_rect(surface, &payload->clip, &clip)) {
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
		opengl_color_mask(surface->depth));
	state->gl.dispatch_compute(groups, 1, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_BUFFER_UPDATE_BARRIER_BIT);
	result = opengl_check_error(state, "point compute dispatch");
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int opengl_line(FB_GFX3_OPENGL_STATE *state,
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

	result = opengl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!opengl_clip_rect(surface, &payload->clip, &clip)) {
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
		opengl_color_mask(surface->depth));
	state->gl.uniform_1ui(state->line_flags_location, payload->flags);
	state->gl.dispatch_compute(groups, 1, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	result = opengl_check_error(state, "line compute dispatch");
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int opengl_rectangle(FB_GFX3_OPENGL_STATE *state,
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
	result = opengl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!opengl_clip_rect(surface, &payload->clip, &clip) ||
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
		result = opengl_dispatch_clear(state, surface, &filled_rect,
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
		opengl_color_mask(surface->depth));
	state->gl.uniform_1ui(state->rectangle_filled_location,
		payload->filled != 0);
	state->gl.uniform_1ui(state->rectangle_flags_location, payload->flags);
	state->gl.dispatch_compute(groups, 1, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	result = opengl_check_error(state, "rectangle compute dispatch");
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int opengl_ellipse(FB_GFX3_OPENGL_STATE *state,
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
	result = opengl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!opengl_clip_rect(surface, &payload->clip, &clip)) {
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
		opengl_color_mask(surface->depth));
	state->gl.uniform_1ui(state->ellipse_flags_location, payload->flags);
	state->gl.dispatch_compute(1, 1, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	result = opengl_check_error(state, "ellipse compute dispatch");
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
	if (opengl_check_error(state, "paint scratch allocation") != FB_GFX3_OK)
		return FB_GFX3_OUT_OF_MEMORY;
	state->paint_scratch_bytes = required_bytes;
	return FB_GFX3_OK;
}

static int opengl_paint(FB_GFX3_OPENGL_STATE *state,
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
	result = opengl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!opengl_clip_rect(surface, &payload->clip, &clip) ||
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
		opengl_color_mask(surface->depth));
	state->gl.uniform_1ui(state->paint_flags_location, payload->flags);
	state->gl.uniform_1ui(state->paint_mode_location, payload->paint_mode);
	state->gl.uniform_1ui(state->paint_pattern_size_location,
		payload->pattern_size);
	state->gl.uniform_4i(state->paint_pattern_origin_location,
		(int)payload->pattern_origin_x, (int)payload->pattern_origin_y, 0, 0);
	state->gl.uniform_1ui(state->paint_bytes_per_pixel_location,
		opengl_bytes_per_pixel(surface->depth, NULL));
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
	result = opengl_check_error(state, "multi-dispatch paint compute");

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int opengl_blit_batch_winner_ensure(FB_GFX3_OPENGL_STATE *state,
	const FB_GFX3_OPENGL_SURFACE *destination, int reset_generation);

static uint32_t opengl_points_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available)
{
	const FB_GFX3_POINTS_COMMAND *first;
	uint32_t count = 1;
	size_t points_size;
	size_t expected_size;
	uint32_t point_index;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_POINTS) ||
	    (fb_gfx3_command_payload_size(commands[0]) <
	     offsetof(FB_GFX3_POINTS_COMMAND, point)))
		return 1;
	first = (const FB_GFX3_POINTS_COMMAND *)commands[0]->payload;
	if ((first->count == 0u) ||
	    (fb_gfx3_size_multiply(first->count, sizeof(first->point[0]),
	     &points_size) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point), points_size,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(commands[0])))
		return 1;
	for (point_index = 0; point_index < first->count; point_index++) {
		if ((first->point[point_index].flags != 0u) ||
		    (first->point[point_index].color != first->point[0].color))
			return 1;
	}
	if (available > FB_GFX3_OPENGL_POINTS_BATCH_LIMIT)
		available = FB_GFX3_OPENGL_POINTS_BATCH_LIMIT;
	while (count < available) {
		const FB_GFX3_POINTS_COMMAND *candidate;

		if ((commands[count] == NULL) ||
		    (commands[count]->type != FB_GFX3_COMMAND_POINTS) ||
		    (commands[count]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[count]) <
		     offsetof(FB_GFX3_POINTS_COMMAND, point)))
			break;
		candidate = (const FB_GFX3_POINTS_COMMAND *)commands[count]->payload;
		if ((candidate->count == 0u) ||
		    (memcmp(&candidate->clip, &first->clip,
		     sizeof(candidate->clip)) != 0) ||
		    (fb_gfx3_size_multiply(candidate->count,
		     sizeof(candidate->point[0]), &points_size) != FB_GFX3_OK) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point),
		     points_size, &expected_size) != FB_GFX3_OK) ||
		    (expected_size != fb_gfx3_command_payload_size(commands[count])))
			break;
		for (point_index = 0; point_index < candidate->count; point_index++) {
			if ((candidate->point[point_index].flags != 0u) ||
			    (candidate->point[point_index].color !=
			     candidate->point[0].color))
				break;
		}
		if (point_index != candidate->count)
			break;
		count++;
	}
	return count;
}

static int opengl_points_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count)
{
	FB_GFX3_OPENGL_POINTS_BATCH_ITEM *items = NULL;
	const FB_GFX3_POINTS_COMMAND *first;
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	FB_GFX3_RECT clip;
	size_t total_points = 0;
	size_t item_count;
	size_t item_bytes;
	uint32_t index;
	uint32_t item_index = 0;
	uint32_t generation;
	uint32_t groups_x;
	uint32_t groups_y;
	int32_t resolve_x1 = INT32_MAX;
	int32_t resolve_y1 = INT32_MAX;
	int32_t resolve_x2 = INT32_MIN;
	int32_t resolve_y2 = INT32_MIN;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_OPENGL_POINTS_BATCH_LIMIT) ||
	    (commands[0] == NULL) ||
	    (fb_gfx3_command_payload_size(commands[0]) <
	     offsetof(FB_GFX3_POINTS_COMMAND, point)))
		return FB_GFX3_INVALID;
	first = (const FB_GFX3_POINTS_COMMAND *)commands[0]->payload;
	result = opengl_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	if (!opengl_clip_rect(destination, &first->clip, &clip)) {
		result = FB_GFX3_OK;
		goto done;
	}
	for (index = 0; index < count; index++) {
		const FB_GFX3_POINTS_COMMAND *payload;
		size_t points_size;
		size_t expected_size;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_POINTS) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) <
		     offsetof(FB_GFX3_POINTS_COMMAND, point))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		payload = (const FB_GFX3_POINTS_COMMAND *)commands[index]->payload;
		if ((payload->count == 0u) ||
		    (memcmp(&payload->clip, &first->clip, sizeof(payload->clip)) != 0) ||
		    (fb_gfx3_size_multiply(payload->count, sizeof(payload->point[0]),
		     &points_size) != FB_GFX3_OK) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point),
		     points_size, &expected_size) != FB_GFX3_OK) ||
		    (expected_size != fb_gfx3_command_payload_size(commands[index])) ||
		    (fb_gfx3_size_add(total_points, payload->count,
		     &total_points) != FB_GFX3_OK)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
	}
	if ((total_points > UINT32_MAX) ||
	    (fb_gfx3_size_add(total_points, count, &item_count) != FB_GFX3_OK) ||
	    (fb_gfx3_size_multiply(item_count, sizeof(*items),
	     &item_bytes) != FB_GFX3_OK) ||
	    (item_bytes > state->maximum_storage_buffer_size)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	items = (FB_GFX3_OPENGL_POINTS_BATCH_ITEM *)malloc(item_bytes);
	if (items == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	for (index = 0; index < count; index++) {
		const FB_GFX3_POINTS_COMMAND *payload =
			(const FB_GFX3_POINTS_COMMAND *)commands[index]->payload;
		uint32_t point_index;

		for (point_index = 0; point_index < payload->count; point_index++) {
			const FB_GFX3_POINT *point = &payload->point[point_index];

			if ((point->flags != 0u) ||
			    (point->color != payload->point[0].color)) {
				result = FB_GFX3_INVALID;
				goto done;
			}
			items[item_index].x = point->x;
			items[item_index].y = point->y;
			items[item_index].command_index = index;
			items[item_index].reserved = 0;
			item_index++;
			if ((point->x >= clip.x1) && (point->x <= clip.x2) &&
			    (point->y >= clip.y1) && (point->y <= clip.y2)) {
				if (point->x < resolve_x1)
					resolve_x1 = point->x;
				if (point->y < resolve_y1)
					resolve_y1 = point->y;
				if (point->x > resolve_x2)
					resolve_x2 = point->x;
				if (point->y > resolve_y2)
					resolve_y2 = point->y;
			}
		}
		items[total_points + index].x = 0;
		items[total_points + index].y = 0;
		items[total_points + index].command_index = 0;
		items[total_points + index].reserved = payload->point[0].color;
	}
	if ((item_index != total_points) || (resolve_x1 > resolve_x2) ||
	    (resolve_y1 > resolve_y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	groups_x = ((uint32_t)total_points + FB_GFX3_OPENGL_POINT_GROUP_SIZE - 1u) /
		FB_GFX3_OPENGL_POINT_GROUP_SIZE;
	if (groups_x > state->maximum_compute_groups_x) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)item_bytes,
		items, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 2,
		state->blit_batch_buffer);
	if (state->blit_batch_generation >= (UINT32_MAX >> 13))
		result = opengl_blit_batch_winner_ensure(state, destination, TRUE);
	else
		result = opengl_blit_batch_winner_ensure(state, destination, FALSE);
	if (result != FB_GFX3_OK)
		goto done;
	generation = ++state->blit_batch_generation;
	state->gl.use_program(state->points_batch_program);
	state->gl.bind_image_texture(0, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_4i(state->points_batch_clip_location, clip.x1, clip.y1,
		clip.x2, clip.y2);
	state->gl.uniform_1ui(state->points_batch_count_location,
		(uint32_t)total_points);
	state->gl.uniform_1ui(state->points_batch_key_location,
		(generation << 13) | 1u);
	state->gl.dispatch_compute(groups_x, 1, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT);
	result = opengl_check_error(state, "point batch selection dispatch");
	if (result != FB_GFX3_OK)
		goto done;
	groups_x = ((uint32_t)(resolve_x2 - resolve_x1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_X - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_X;
	groups_y = ((uint32_t)(resolve_y2 - resolve_y1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_Y - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_Y;
	state->gl.use_program(state->points_batch_resolve_program);
	state->gl.bind_image_texture(0, destination->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.bind_image_texture(1, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
	state->gl.uniform_4i(state->points_batch_resolve_rect_location,
		resolve_x1, resolve_y1, resolve_x2 - resolve_x1 + 1,
		resolve_y2 - resolve_y1 + 1);
	state->gl.uniform_1ui(state->points_batch_resolve_count_location,
		(uint32_t)total_points);
	state->gl.uniform_1ui(state->points_batch_resolve_generation_location,
		generation);
	state->gl.uniform_1ui(state->points_batch_resolve_mask_location,
		opengl_color_mask(destination->depth));
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	result = opengl_check_error(state, "point batch resolve dispatch");

done:
	free(items);
	fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

static uint32_t opengl_glyph_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available)
{
	const FB_GFX3_GLYPHS_COMMAND *first;
	size_t glyph_bytes;
	size_t expected_size;
	uint32_t total;
	uint32_t count = 1u;

	if ((commands == NULL) || (available == 0u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_GLYPHS) ||
	    (fb_gfx3_command_payload_size(commands[0]) <
	     offsetof(FB_GFX3_GLYPHS_COMMAND, glyph)))
		return 1u;
	first = (const FB_GFX3_GLYPHS_COMMAND *)commands[0]->payload;
	if ((first->count == 0u) ||
	    (fb_gfx3_size_multiply(first->count, sizeof(first->glyph[0]),
	     &glyph_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_GLYPHS_COMMAND, glyph), glyph_bytes,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(commands[0])))
		return 1u;
	total = first->count;
	while ((count < available) &&
	       (total < FB_GFX3_OPENGL_GLYPH_BATCH_LIMIT)) {
		const FB_GFX3_GLYPHS_COMMAND *candidate;

		if ((commands[count] == NULL) ||
		    (commands[count]->type != FB_GFX3_COMMAND_GLYPHS) ||
		    (commands[count]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[count]) <
		     offsetof(FB_GFX3_GLYPHS_COMMAND, glyph)))
			break;
		candidate = (const FB_GFX3_GLYPHS_COMMAND *)commands[count]->payload;
		if ((candidate->count == 0u) ||
		    (candidate->count > FB_GFX3_OPENGL_GLYPH_BATCH_LIMIT - total) ||
		    (memcmp(&candidate->clip, &first->clip,
		     sizeof(candidate->clip)) != 0) ||
		    (fb_gfx3_size_multiply(candidate->count,
		     sizeof(candidate->glyph[0]), &glyph_bytes) != FB_GFX3_OK) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_GLYPHS_COMMAND, glyph),
		     glyph_bytes, &expected_size) != FB_GFX3_OK) ||
		    (expected_size != fb_gfx3_command_payload_size(commands[count])))
			break;
		total += candidate->count;
		count++;
	}
	return count;
}

static int opengl_glyph_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t command_count)
{
	const FB_GFX3_GLYPHS_COMMAND *first;
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	FB_GFX3_GLYPH *glyphs = NULL;
	uint32_t *tile_counts = NULL;
	uint32_t *tile_ranges = NULL;
	uint32_t *tile_cursors = NULL;
	uint32_t *tile_indices = NULL;
	FB_GFX3_RECT clip;
	size_t glyph_bytes;
	size_t expected_size;
	uint32_t total = 0u;
	uint32_t command_index;
	uint32_t glyph_index = 0u;
	uint32_t first_tile_x;
	uint32_t first_tile_y;
	uint32_t last_tile_x;
	uint32_t last_tile_y;
	uint32_t tiles_x;
	uint32_t tiles_y;
	uint32_t tile_count;
	uint32_t index_count = 0u;
	uint32_t index;
	uint32_t groups_x;
	uint32_t groups_y;
	int32_t resolve_x1 = INT32_MAX;
	int32_t resolve_y1 = INT32_MAX;
	int32_t resolve_x2 = INT32_MIN;
	int32_t resolve_y2 = INT32_MIN;
	int result;

	if ((state == NULL) || (commands == NULL) || (command_count == 0u) ||
	    (commands[0] == NULL) ||
	    (fb_gfx3_command_payload_size(commands[0]) <
	     offsetof(FB_GFX3_GLYPHS_COMMAND, glyph)))
		return FB_GFX3_INVALID;
	first = (const FB_GFX3_GLYPHS_COMMAND *)commands[0]->payload;
	result = opengl_surface_retain(state, commands[0]->target,
		commands[command_count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	if (!opengl_clip_rect(destination, &first->clip, &clip)) {
		result = FB_GFX3_OK;
		goto done;
	}
	for (command_index = 0u; command_index < command_count; ++command_index) {
		const FB_GFX3_GLYPHS_COMMAND *payload;

		if ((commands[command_index] == NULL) ||
		    (commands[command_index]->type != FB_GFX3_COMMAND_GLYPHS) ||
		    (commands[command_index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[command_index]) <
		     offsetof(FB_GFX3_GLYPHS_COMMAND, glyph))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		payload = (const FB_GFX3_GLYPHS_COMMAND *)
			commands[command_index]->payload;
		if ((payload->count == 0u) ||
		    (payload->count > FB_GFX3_OPENGL_GLYPH_BATCH_LIMIT - total) ||
		    (memcmp(&payload->clip, &first->clip, sizeof(payload->clip)) != 0) ||
		    (fb_gfx3_size_multiply(payload->count, sizeof(payload->glyph[0]),
		     &glyph_bytes) != FB_GFX3_OK) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_GLYPHS_COMMAND, glyph),
		     glyph_bytes, &expected_size) != FB_GFX3_OK) ||
		    (expected_size !=
		     fb_gfx3_command_payload_size(commands[command_index]))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		total += payload->count;
	}
	if ((total == 0u) || (total > FB_GFX3_OPENGL_GLYPH_BATCH_LIMIT) ||
	    (fb_gfx3_size_multiply(total, sizeof(glyphs[0]), &glyph_bytes) !=
	     FB_GFX3_OK) || (glyph_bytes > state->maximum_storage_buffer_size)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	glyphs = (FB_GFX3_GLYPH *)malloc(glyph_bytes);
	if (glyphs == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	for (command_index = 0u; command_index < command_count; ++command_index) {
		const FB_GFX3_GLYPHS_COMMAND *payload =
			(const FB_GFX3_GLYPHS_COMMAND *)commands[command_index]->payload;
		uint32_t item;

		for (item = 0u; item < payload->count; ++item) {
			const FB_GFX3_GLYPH *glyph = &payload->glyph[item];
			int64_t last_x;
			int64_t last_y;

			if ((glyph->width == 0u) || (glyph->width > 8u) ||
			    (glyph->height == 0u) || (glyph->height > 16u) ||
			    ((glyph->flags &
			      ~(uint32_t)FB_GFX3_GLYPH_BACKGROUND) != 0u)) {
				result = FB_GFX3_INVALID;
				goto done;
			}
			glyphs[glyph_index++] = *glyph;
			last_x = (int64_t)glyph->x + glyph->width - 1u;
			last_y = (int64_t)glyph->y + glyph->height - 1u;
			if ((last_x < clip.x1) || (last_y < clip.y1) ||
			    (glyph->x > clip.x2) || (glyph->y > clip.y2))
				continue;
			if (glyph->x < resolve_x1)
				resolve_x1 = glyph->x;
			if (glyph->y < resolve_y1)
				resolve_y1 = glyph->y;
			if (resolve_x1 < clip.x1)
				resolve_x1 = clip.x1;
			if (resolve_y1 < clip.y1)
				resolve_y1 = clip.y1;
			if (last_x > resolve_x2)
				resolve_x2 = (last_x > INT32_MAX) ? INT32_MAX : (int32_t)last_x;
			if (last_y > resolve_y2)
				resolve_y2 = (last_y > INT32_MAX) ? INT32_MAX : (int32_t)last_y;
			if (resolve_x2 > clip.x2)
				resolve_x2 = clip.x2;
			if (resolve_y2 > clip.y2)
				resolve_y2 = clip.y2;
		}
	}
	if ((glyph_index != total) || (resolve_x1 > resolve_x2) ||
	    (resolve_y1 > resolve_y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	first_tile_x = (uint32_t)resolve_x1 / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
	first_tile_y = (uint32_t)resolve_y1 / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
	last_tile_x = (uint32_t)resolve_x2 / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
	last_tile_y = (uint32_t)resolve_y2 / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
	tiles_x = last_tile_x - first_tile_x + 1u;
	tiles_y = last_tile_y - first_tile_y + 1u;
	if ((tiles_x == 0u) || (tiles_y == 0u) ||
	    (tiles_x > UINT32_MAX / tiles_y) ||
	    ((tile_count = tiles_x * tiles_y) >
	     FB_GFX3_OPENGL_ALPHA_TILE_MAX_TILES) ||
	    (tiles_x > state->maximum_compute_groups_x) ||
	    (tiles_y > state->maximum_compute_groups_y)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	tile_counts = (uint32_t *)calloc(tile_count, sizeof(*tile_counts));
	if (tile_counts == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	for (index = 0u; index < total; ++index) {
		const FB_GFX3_GLYPH *glyph = &glyphs[index];
		int64_t right64 = (int64_t)glyph->x + glyph->width - 1u;
		int64_t bottom64 = (int64_t)glyph->y + glyph->height - 1u;
		int32_t left;
		int32_t top;
		int32_t right;
		int32_t bottom;
		uint32_t tile_y;

		if ((right64 < clip.x1) || (bottom64 < clip.y1) ||
		    (glyph->x > clip.x2) || (glyph->y > clip.y2))
			continue;
		left = (glyph->x > clip.x1) ? glyph->x : clip.x1;
		top = (glyph->y > clip.y1) ? glyph->y : clip.y1;
		right = (right64 < clip.x2) ? (int32_t)right64 : clip.x2;
		bottom = (bottom64 < clip.y2) ? (int32_t)bottom64 : clip.y2;
		for (tile_y = (uint32_t)top / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
		     tile_y <= (uint32_t)bottom / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
		     ++tile_y) {
			uint32_t tile_x;

			for (tile_x = (uint32_t)left / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
			     tile_x <= (uint32_t)right / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
			     ++tile_x) {
				uint32_t tile = (tile_y - first_tile_y) * tiles_x +
					(tile_x - first_tile_x);

				if (tile_counts[tile] == UINT32_MAX) {
					result = FB_GFX3_OUT_OF_MEMORY;
					goto done;
				}
				tile_counts[tile]++;
			}
		}
	}
	for (index = 0u; index < tile_count; ++index) {
		if (tile_counts[index] > UINT32_MAX - index_count) {
			result = FB_GFX3_OUT_OF_MEMORY;
			goto done;
		}
		index_count += tile_counts[index];
	}
	/*
		The resolved glyph bounds guarantee at least one covered tile.
		Reject an inconsistent batch explicitly instead of asking malloc()
		or the OpenGL driver to process an empty index buffer.
	*/
	if (index_count == 0u) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	tile_ranges = (uint32_t *)calloc((size_t)tile_count * 2u,
		sizeof(*tile_ranges));
	tile_cursors = (uint32_t *)calloc(tile_count, sizeof(*tile_cursors));
	tile_indices = (uint32_t *)malloc((size_t)index_count *
		sizeof(*tile_indices));
	if ((tile_ranges == NULL) || (tile_cursors == NULL) ||
	    (tile_indices == NULL)) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	index_count = 0u;
	for (index = 0u; index < tile_count; ++index) {
		tile_ranges[index * 2u] = index_count;
		tile_ranges[index * 2u + 1u] = tile_counts[index];
		tile_cursors[index] = index_count;
		index_count += tile_counts[index];
	}
	for (index = 0u; index < total; ++index) {
		const FB_GFX3_GLYPH *glyph = &glyphs[index];
		int64_t right64 = (int64_t)glyph->x + glyph->width - 1u;
		int64_t bottom64 = (int64_t)glyph->y + glyph->height - 1u;
		int32_t left;
		int32_t top;
		int32_t right;
		int32_t bottom;
		uint32_t tile_y;

		if ((right64 < clip.x1) || (bottom64 < clip.y1) ||
		    (glyph->x > clip.x2) || (glyph->y > clip.y2))
			continue;
		left = (glyph->x > clip.x1) ? glyph->x : clip.x1;
		top = (glyph->y > clip.y1) ? glyph->y : clip.y1;
		right = (right64 < clip.x2) ? (int32_t)right64 : clip.x2;
		bottom = (bottom64 < clip.y2) ? (int32_t)bottom64 : clip.y2;
		for (tile_y = (uint32_t)top / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
		     tile_y <= (uint32_t)bottom / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
		     ++tile_y) {
			uint32_t tile_x;

			for (tile_x = (uint32_t)left / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
			     tile_x <= (uint32_t)right / FB_GFX3_OPENGL_GLYPH_TILE_SIZE;
			     ++tile_x) {
				uint32_t tile = (tile_y - first_tile_y) * tiles_x +
					(tile_x - first_tile_x);

				tile_indices[tile_cursors[tile]++] = index;
			}
		}
	}
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)glyph_bytes,
		glyphs, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 2,
		state->blit_batch_buffer);
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER,
		state->blit_tile_range_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)((size_t)tile_count * 2u * sizeof(*tile_ranges)),
		tile_ranges, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 3,
		state->blit_tile_range_buffer);
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER,
		state->blit_tile_index_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)((size_t)index_count * sizeof(*tile_indices)),
		tile_indices, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 4,
		state->blit_tile_index_buffer);
	state->gl.use_program(state->glyph_batch_program);
	state->gl.bind_image_texture(0, destination->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_4i(state->glyph_batch_clip_location, clip.x1, clip.y1,
		clip.x2, clip.y2);
	state->gl.uniform_1ui(state->glyph_batch_count_location, total);
	state->gl.uniform_4i(state->glyph_batch_tile_origin_location,
		(int32_t)first_tile_x, (int32_t)first_tile_y, 0, 0);
	state->gl.uniform_1ui(state->glyph_batch_tiles_x_location, tiles_x);
	state->gl.uniform_1ui(state->glyph_batch_mask_location,
		opengl_color_mask(destination->depth));
	groups_x = tiles_x;
	groups_y = tiles_y;
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	result = opengl_check_error(state, "ordered glyph tile dispatch");

done:
	free(tile_indices);
	free(tile_cursors);
	free(tile_ranges);
	free(tile_counts);
	free(glyphs);
	fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

static uint32_t opengl_line_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available)
{
	const FB_GFX3_LINE_COMMAND *first;
	uint32_t count = 1;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_LINE) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1;
	first = (const FB_GFX3_LINE_COMMAND *)commands[0]->payload;
	if ((first->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0)
		return 1;
	if ((llabs((long long)first->x2 - first->x1) > 32767) ||
	    (llabs((long long)first->y2 - first->y1) > 32767))
		return 1;
	if (available > FB_GFX3_OPENGL_LINE_BATCH_LIMIT)
		available = FB_GFX3_OPENGL_LINE_BATCH_LIMIT;
	while (count < available) {
		const FB_GFX3_LINE_COMMAND *candidate;

		if ((commands[count] == NULL) ||
		    (commands[count]->type != FB_GFX3_COMMAND_LINE) ||
		    (commands[count]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[count]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_LINE_COMMAND *)commands[count]->payload;
		if (((candidate->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0) ||
		    (candidate->style != first->style) ||
		    (memcmp(&candidate->clip, &first->clip,
		     sizeof(candidate->clip)) != 0) ||
		    (llabs((long long)candidate->x2 - candidate->x1) > 32767) ||
		    (llabs((long long)candidate->y2 - candidate->y1) > 32767))
			break;
		count++;
	}
	return count;
}

static uint32_t opengl_alpha_blit_batch_count(
	FB_GFX3_COMMAND *const *commands, uint32_t available)
{
	const FB_GFX3_BLIT_COMMAND *first;
	uint32_t count;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_BLIT) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1u;
	first = (const FB_GFX3_BLIT_COMMAND *)commands[0]->payload;
	if (((first->mode != FB_GFX3_BLIT_TRANS) &&
	    (first->mode != FB_GFX3_BLIT_PSET) &&
	    (first->mode != FB_GFX3_BLIT_PRESET) &&
	    (first->mode != FB_GFX3_BLIT_AND) &&
	    (first->mode != FB_GFX3_BLIT_OR) &&
	    (first->mode != FB_GFX3_BLIT_XOR) &&
	    (first->mode != FB_GFX3_BLIT_ALPHA) &&
	    (first->mode != FB_GFX3_BLIT_BLEND) &&
	    (first->mode != FB_GFX3_BLIT_ADD)) ||
	    (first->source == commands[0]->target))
		return 1u;
	if (available > FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT)
		available = FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT;
	for (count = 1u; count < available; ++count) {
		const FB_GFX3_BLIT_COMMAND *candidate;

		if ((commands[count] == NULL) ||
		    (commands[count]->type != FB_GFX3_COMMAND_BLIT) ||
		    (commands[count]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[count]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_BLIT_COMMAND *)commands[count]->payload;
		if ((candidate->mode != first->mode) ||
		    (candidate->source != first->source) ||
		    (((first->mode == FB_GFX3_BLIT_BLEND) ||
		      (first->mode == FB_GFX3_BLIT_ADD)) &&
		     (candidate->alpha != first->alpha)))
			break;
	}
	return count;
}

static uint32_t opengl_blit_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available)
{
	const FB_GFX3_BLIT_COMMAND *first;
	const FB_GFX3_BLIT_COMMAND *candidate;
	uint32_t index;

	if ((commands == NULL) || (available == 0) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_BLIT) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1;
	first = (const FB_GFX3_BLIT_COMMAND *)commands[0]->payload;
	if (((first->mode != FB_GFX3_BLIT_TRANS) &&
	     (first->mode != FB_GFX3_BLIT_PSET) &&
	     (first->mode != FB_GFX3_BLIT_PRESET) &&
	     (first->mode != FB_GFX3_BLIT_AND) &&
	     (first->mode != FB_GFX3_BLIT_OR) &&
	     (first->mode != FB_GFX3_BLIT_XOR)) ||
	    (first->source == commands[0]->target) ||
	    (first->source_rect.x1 > first->source_rect.x2) ||
	    (first->source_rect.y1 > first->source_rect.y2))
		return 1;

	if (available > FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT)
		available = FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT;
	for (index = 1; index < available; ++index) {
		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_BLIT) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_BLIT_COMMAND *)commands[index]->payload;
		if ((candidate->source != first->source) ||
		    (commands[index]->target != commands[0]->target) ||
		    (candidate->mode != first->mode) ||
		    (candidate->source_rect.x1 > candidate->source_rect.x2) ||
		    (candidate->source_rect.y1 > candidate->source_rect.y2))
			break;
	}
	return index;
}

static uint32_t opengl_rectangle_batch_count(
	FB_GFX3_COMMAND *const *commands, uint32_t available)
{
	const FB_GFX3_RECTANGLE_COMMAND *first;
	uint32_t index;

	if ((commands == NULL) || (available < 2) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_RECTANGLE) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1;
	first = (const FB_GFX3_RECTANGLE_COMMAND *)commands[0]->payload;
	if ((first->filled == 0) ||
	    ((first->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0))
		return 1;
	if (available > FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT)
		available = FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT;
	for (index = 1; index < available; ++index) {
		const FB_GFX3_RECTANGLE_COMMAND *candidate;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_RECTANGLE) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_RECTANGLE_COMMAND *)commands[index]->payload;
		if ((candidate->filled == 0) ||
		    ((candidate->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0))
			break;
	}
	return index;
}

static uint32_t opengl_ellipse_batch_count(
	FB_GFX3_COMMAND *const *commands, uint32_t available)
{
	const FB_GFX3_ELLIPSE_COMMAND *first;
	uint32_t index;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_ELLIPSE) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1u;
	first = (const FB_GFX3_ELLIPSE_COMMAND *)commands[0]->payload;
	if ((first->filled == 0u) ||
	    ((first->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u))
		return 1u;
	if (available > FB_GFX3_OPENGL_ELLIPSE_BATCH_LIMIT)
		available = FB_GFX3_OPENGL_ELLIPSE_BATCH_LIMIT;
	for (index = 1u; index < available; ++index) {
		const FB_GFX3_ELLIPSE_COMMAND *candidate;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_ELLIPSE) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_ELLIPSE_COMMAND *)commands[index]->payload;
		if ((candidate->filled == 0u) ||
		    ((candidate->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u))
			break;
	}
	return index;
}

/*
	Count an adjacent opaque POINTS/LINE/LINES/ELLIPSE stream and its flattened
	GPU operations. A single public command stays on its existing specialized
	path; the mixed winner pass is valuable when it removes at least one
	intermediate resolve.
*/
static uint32_t opengl_primitive_batch_count(
	FB_GFX3_COMMAND *const *commands, uint32_t available,
	uint32_t *primitive_count_result)
{
	FB_GFX3_HANDLE target;
	uint32_t command_count = 0u;
	uint32_t primitive_count = 0u;

	if (primitive_count_result != NULL)
		*primitive_count_result = 0u;
	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (primitive_count_result == NULL))
		return 1u;
	target = commands[0]->target;
	while ((command_count < available) &&
	       (primitive_count < FB_GFX3_OPENGL_LINE_BATCH_LIMIT)) {
		FB_GFX3_COMMAND *command = commands[command_count];
		uint32_t added_count = 0u;

		if ((command == NULL) || (command->target != target))
			break;
		if (command->type == FB_GFX3_COMMAND_POINTS) {
			const FB_GFX3_POINTS_COMMAND *points;
			size_t point_bytes;
			size_t expected_size;
			uint32_t index;

			if (fb_gfx3_command_payload_size(command) <
			    offsetof(FB_GFX3_POINTS_COMMAND, point))
				break;
			points = (const FB_GFX3_POINTS_COMMAND *)command->payload;
			if ((points->count == 0u) ||
			    (fb_gfx3_size_multiply(points->count,
			     sizeof(points->point[0]), &point_bytes) != FB_GFX3_OK) ||
			    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point),
			     point_bytes, &expected_size) != FB_GFX3_OK) ||
			    (expected_size != fb_gfx3_command_payload_size(command)))
				break;
			for (index = 0u; index < points->count; index++) {
				if ((points->point[index].flags &
				     FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u)
					break;
			}
			if (index != points->count)
				break;
			added_count = points->count;
		} else if (command->type == FB_GFX3_COMMAND_LINE) {
			const FB_GFX3_LINE_COMMAND *line;

			if (fb_gfx3_command_payload_size(command) != sizeof(*line))
				break;
			line = (const FB_GFX3_LINE_COMMAND *)command->payload;
			if (((line->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
			    (llabs((long long)line->x2 - line->x1) > 32767) ||
			    (llabs((long long)line->y2 - line->y1) > 32767))
				break;
			added_count = 1u;
		} else if (command->type == FB_GFX3_COMMAND_LINES) {
			const FB_GFX3_LINES_COMMAND *lines;
			size_t line_bytes;
			size_t expected_size;
			uint32_t index;

			if (fb_gfx3_command_payload_size(command) <
			    offsetof(FB_GFX3_LINES_COMMAND, line))
				break;
			lines = (const FB_GFX3_LINES_COMMAND *)command->payload;
			if ((lines->count == 0u) ||
			    (fb_gfx3_size_multiply(lines->count,
			     sizeof(lines->line[0]), &line_bytes) != FB_GFX3_OK) ||
			    (fb_gfx3_size_add(offsetof(FB_GFX3_LINES_COMMAND, line),
			     line_bytes, &expected_size) != FB_GFX3_OK) ||
			    (expected_size != fb_gfx3_command_payload_size(command)))
				break;
			for (index = 0u; index < lines->count; index++) {
				const FB_GFX3_LINE_COMMAND *line = &lines->line[index];

				if (((line->flags &
				      FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
				    (llabs((long long)line->x2 - line->x1) > 32767) ||
				    (llabs((long long)line->y2 - line->y1) > 32767))
					break;
			}
			if (index != lines->count)
				break;
			added_count = lines->count;
		} else if (command->type == FB_GFX3_COMMAND_ELLIPSE) {
			const FB_GFX3_ELLIPSE_COMMAND *ellipse;

			if (fb_gfx3_command_payload_size(command) != sizeof(*ellipse))
				break;
			ellipse = (const FB_GFX3_ELLIPSE_COMMAND *)command->payload;
			if (((ellipse->flags &
			      FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
			    !(ellipse->radius_x >= 0.0f) ||
			    !(ellipse->radius_x <= 32767.0f) ||
			    !(ellipse->radius_y >= 0.0f) ||
			    !(ellipse->radius_y <= 32767.0f))
				break;
			added_count = 1u;
		} else {
			break;
		}
		if (added_count >
		    FB_GFX3_OPENGL_LINE_BATCH_LIMIT - primitive_count)
			break;
		primitive_count += added_count;
		command_count++;
	}
	if (command_count < 2u)
		return 1u;
	*primitive_count_result = primitive_count;
	return command_count;
}

static int opengl_blit_batch_winner_ensure(FB_GFX3_OPENGL_STATE *state,
	const FB_GFX3_OPENGL_SURFACE *destination, int reset_generation)
{
	if ((state->blit_batch_winner_texture != 0) &&
	    (state->blit_batch_winner_width == destination->width) &&
	    (state->blit_batch_winner_height == destination->height) &&
	    !reset_generation)
		return FB_GFX3_OK;
	if (state->blit_batch_winner_texture != 0) {
		state->gl.delete_textures(1, &state->blit_batch_winner_texture);
		state->blit_batch_winner_texture = 0;
	}
	state->gl.generate_textures(1, &state->blit_batch_winner_texture);
	if (state->blit_batch_winner_texture == 0)
		return FB_GFX3_OUT_OF_MEMORY;
	state->gl.bind_texture(GL_TEXTURE_2D, state->blit_batch_winner_texture);
	state->gl.texture_storage_2d(GL_TEXTURE_2D, 1, GL_R32UI,
		(GLsizei)destination->width, (GLsizei)destination->height);
	if (opengl_check_error(state, "blit batch winner allocation") !=
	    FB_GFX3_OK) {
		state->gl.delete_textures(1, &state->blit_batch_winner_texture);
		state->blit_batch_winner_texture = 0;
		return FB_GFX3_OUT_OF_MEMORY;
	}
	state->blit_batch_winner_width = destination->width;
	state->blit_batch_winner_height = destination->height;
	state->blit_batch_generation = 0;
	return FB_GFX3_OK;
}

static void opengl_primitive_batch_expand_bounds(int32_t x1, int32_t y1,
	int32_t x2, int32_t y2, int32_t *resolve_x1, int32_t *resolve_y1,
	int32_t *resolve_x2, int32_t *resolve_y2)
{
	if ((resolve_x1 == NULL) || (resolve_y1 == NULL) ||
	    (resolve_x2 == NULL) || (resolve_y2 == NULL) ||
	    (x1 > x2) || (y1 > y2))
		return;
	if (x1 < *resolve_x1)
		*resolve_x1 = x1;
	if (y1 < *resolve_y1)
		*resolve_y1 = y1;
	if (x2 > *resolve_x2)
		*resolve_x2 = x2;
	if (y2 > *resolve_y2)
		*resolve_y2 = y2;
}

static int opengl_primitive_batch_add_line(
	const FB_GFX3_OPENGL_SURFACE *destination,
	const FB_GFX3_LINE_COMMAND *line,
	FB_GFX3_OPENGL_PRIMITIVE_BATCH_ITEM *item, uint32_t order,
	uint32_t *maximum_line_count, int32_t *resolve_x1,
	int32_t *resolve_y1, int32_t *resolve_x2, int32_t *resolve_y2)
{
	FB_GFX3_RECT clip;
	int64_t difference_x;
	int64_t difference_y;
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
	uint32_t line_count;

	if ((destination == NULL) || (line == NULL) || (item == NULL) ||
	    (order == 0u) || (maximum_line_count == NULL))
		return FB_GFX3_INVALID;
	difference_x = llabs((long long)line->x2 - line->x1);
	difference_y = llabs((long long)line->y2 - line->y1);
	if (((line->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
	    (difference_x > 32767) || (difference_y > 32767))
		return FB_GFX3_UNSUPPORTED;
	line_count = (uint32_t)((difference_x > difference_y) ?
		difference_x : difference_y) + 1u;
	if (line_count > *maximum_line_count)
		*maximum_line_count = line_count;
	item->geometry_x1 = line->x1;
	item->geometry_y1 = line->y1;
	item->geometry_x2_or_radius_x = line->x2;
	item->geometry_y2_or_radius_y = line->y2;
	item->color = line->color;
	item->type = FB_GFX3_OPENGL_PRIMITIVE_LINE;
	item->style_or_filled = line->style & 0xFFFFu;
	item->order = order;
	if (!opengl_clip_rect(destination, &line->clip, &clip)) {
		item->clip_x1 = 1;
		item->clip_y1 = 1;
		item->clip_x2 = 0;
		item->clip_y2 = 0;
		return FB_GFX3_OK;
	}
	item->clip_x1 = clip.x1;
	item->clip_y1 = clip.y1;
	item->clip_x2 = clip.x2;
	item->clip_y2 = clip.y2;
	x1 = (line->x1 < line->x2) ? line->x1 : line->x2;
	y1 = (line->y1 < line->y2) ? line->y1 : line->y2;
	x2 = (line->x1 > line->x2) ? line->x1 : line->x2;
	y2 = (line->y1 > line->y2) ? line->y1 : line->y2;
	if (x1 < clip.x1)
		x1 = clip.x1;
	if (y1 < clip.y1)
		y1 = clip.y1;
	if (x2 > clip.x2)
		x2 = clip.x2;
	if (y2 > clip.y2)
		y2 = clip.y2;
	opengl_primitive_batch_expand_bounds(x1, y1, x2, y2,
		resolve_x1, resolve_y1, resolve_x2, resolve_y2);
	return FB_GFX3_OK;
}

static int opengl_primitive_batch_add_ellipse(
	const FB_GFX3_OPENGL_SURFACE *destination,
	const FB_GFX3_ELLIPSE_COMMAND *ellipse,
	FB_GFX3_OPENGL_PRIMITIVE_BATCH_ITEM *item, uint32_t order,
	int32_t *resolve_x1, int32_t *resolve_y1,
	int32_t *resolve_x2, int32_t *resolve_y2)
{
	FB_GFX3_RECT clip;
	double x1;
	double y1;
	double x2;
	double y2;
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;

	if ((destination == NULL) || (ellipse == NULL) || (item == NULL) ||
	    (order == 0u))
		return FB_GFX3_INVALID;
	if (((ellipse->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
	    !(ellipse->radius_x >= 0.0f) ||
	    !(ellipse->radius_x <= 32767.0f) ||
	    !(ellipse->radius_y >= 0.0f) ||
	    !(ellipse->radius_y <= 32767.0f))
		return FB_GFX3_UNSUPPORTED;
	item->geometry_x1 = ellipse->center_x;
	item->geometry_y1 = ellipse->center_y;
	memcpy(&item->geometry_x2_or_radius_x, &ellipse->radius_x,
		sizeof(item->geometry_x2_or_radius_x));
	memcpy(&item->geometry_y2_or_radius_y, &ellipse->radius_y,
		sizeof(item->geometry_y2_or_radius_y));
	item->color = ellipse->color;
	item->type = FB_GFX3_OPENGL_PRIMITIVE_ELLIPSE;
	item->style_or_filled = ellipse->filled != 0u;
	item->order = order;
	if (!opengl_clip_rect(destination, &ellipse->clip, &clip)) {
		item->clip_x1 = 1;
		item->clip_y1 = 1;
		item->clip_x2 = 0;
		item->clip_y2 = 0;
		return FB_GFX3_OK;
	}
	item->clip_x1 = clip.x1;
	item->clip_y1 = clip.y1;
	item->clip_x2 = clip.x2;
	item->clip_y2 = clip.y2;
	x1 = (double)ellipse->center_x - ellipse->radius_x;
	y1 = (double)ellipse->center_y - ellipse->radius_y;
	x2 = (double)ellipse->center_x + ellipse->radius_x;
	y2 = (double)ellipse->center_y + ellipse->radius_y;
	if ((x1 < INT32_MIN) || (y1 < INT32_MIN) ||
	    (x2 > INT32_MAX) || (y2 > INT32_MAX))
		return FB_GFX3_UNSUPPORTED;
	left = (int32_t)x1;
	top = (int32_t)y1;
	right = (int32_t)x2;
	bottom = (int32_t)y2;
	if (left < clip.x1)
		left = clip.x1;
	if (top < clip.y1)
		top = clip.y1;
	if (right > clip.x2)
		right = clip.x2;
	if (bottom > clip.y2)
		bottom = clip.y2;
	opengl_primitive_batch_expand_bounds(left, top, right, bottom,
		resolve_x1, resolve_y1, resolve_x2, resolve_y2);
	return FB_GFX3_OK;
}

static int opengl_primitive_batch_add_point(
	const FB_GFX3_OPENGL_SURFACE *destination, const FB_GFX3_RECT *command_clip,
	const FB_GFX3_POINT *point, FB_GFX3_OPENGL_PRIMITIVE_BATCH_ITEM *item,
	uint32_t order, int32_t *resolve_x1, int32_t *resolve_y1,
	int32_t *resolve_x2, int32_t *resolve_y2)
{
	FB_GFX3_RECT clip;

	if ((destination == NULL) || (command_clip == NULL) || (point == NULL) ||
	    (item == NULL) || (order == 0u))
		return FB_GFX3_INVALID;
	if ((point->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u)
		return FB_GFX3_UNSUPPORTED;
	item->geometry_x1 = point->x;
	item->geometry_y1 = point->y;
	item->color = point->color;
	item->type = FB_GFX3_OPENGL_PRIMITIVE_POINT;
	item->order = order;
	if (!opengl_clip_rect(destination, command_clip, &clip)) {
		item->clip_x1 = 1;
		item->clip_y1 = 1;
		item->clip_x2 = 0;
		item->clip_y2 = 0;
		return FB_GFX3_OK;
	}
	item->clip_x1 = clip.x1;
	item->clip_y1 = clip.y1;
	item->clip_x2 = clip.x2;
	item->clip_y2 = clip.y2;
	if ((point->x >= clip.x1) && (point->x <= clip.x2) &&
	    (point->y >= clip.y1) && (point->y <= clip.y2))
		opengl_primitive_batch_expand_bounds(point->x, point->y,
			point->x, point->y, resolve_x1, resolve_y1,
			resolve_x2, resolve_y2);
	return FB_GFX3_OK;
}

static int opengl_primitive_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t command_count,
	uint32_t primitive_count)
{
	FB_GFX3_OPENGL_PRIMITIVE_BATCH_ITEM *items = NULL;
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	uint32_t item_index = 0u;
	uint32_t command_index;
	uint32_t maximum_line_count = 1u;
	uint32_t generation;
	uint32_t groups_x;
	uint32_t groups_y;
	int32_t resolve_x1 = INT32_MAX;
	int32_t resolve_y1 = INT32_MAX;
	int32_t resolve_x2 = INT32_MIN;
	int32_t resolve_y2 = INT32_MIN;
	int result;

	if ((state == NULL) || (commands == NULL) || (command_count < 2u) ||
	    (primitive_count < 2u) ||
	    (primitive_count > FB_GFX3_OPENGL_LINE_BATCH_LIMIT) ||
	    (commands[0] == NULL) || (commands[command_count - 1u] == NULL))
		return FB_GFX3_INVALID;
	items = (FB_GFX3_OPENGL_PRIMITIVE_BATCH_ITEM *)calloc(primitive_count,
		sizeof(*items));
	if (items == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	result = opengl_surface_retain(state, commands[0]->target,
		commands[command_count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		goto done;
	for (command_index = 0u; command_index < command_count; command_index++) {
		FB_GFX3_COMMAND *command = commands[command_index];

		if ((command == NULL) ||
		    (command->target != commands[0]->target) ||
		    ((command_index > 0u) &&
		     (command->sequence <= commands[command_index - 1u]->sequence))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (command->type == FB_GFX3_COMMAND_POINTS) {
			const FB_GFX3_POINTS_COMMAND *points =
				(const FB_GFX3_POINTS_COMMAND *)command->payload;
			uint32_t point_index;

			result = FB_GFX3_OK;
			for (point_index = 0u;
			     (point_index < points->count) && (result == FB_GFX3_OK);
			     point_index++) {
				result = opengl_primitive_batch_add_point(destination,
					&points->clip, &points->point[point_index],
					&items[item_index], item_index + 1u,
					&resolve_x1, &resolve_y1, &resolve_x2,
					&resolve_y2);
				item_index++;
			}
		} else if (command->type == FB_GFX3_COMMAND_LINE) {
			const FB_GFX3_LINE_COMMAND *line =
				(const FB_GFX3_LINE_COMMAND *)command->payload;

			result = opengl_primitive_batch_add_line(destination, line,
				&items[item_index], item_index + 1u, &maximum_line_count,
				&resolve_x1, &resolve_y1, &resolve_x2, &resolve_y2);
			item_index++;
		} else if (command->type == FB_GFX3_COMMAND_LINES) {
			const FB_GFX3_LINES_COMMAND *lines =
				(const FB_GFX3_LINES_COMMAND *)command->payload;
			uint32_t line_index;

			result = FB_GFX3_OK;
			for (line_index = 0u;
			     (line_index < lines->count) && (result == FB_GFX3_OK);
			     line_index++) {
				result = opengl_primitive_batch_add_line(destination,
					&lines->line[line_index], &items[item_index],
					item_index + 1u, &maximum_line_count,
					&resolve_x1, &resolve_y1, &resolve_x2,
					&resolve_y2);
				item_index++;
			}
		} else if (command->type == FB_GFX3_COMMAND_ELLIPSE) {
			const FB_GFX3_ELLIPSE_COMMAND *ellipse =
				(const FB_GFX3_ELLIPSE_COMMAND *)command->payload;

			result = opengl_primitive_batch_add_ellipse(destination, ellipse,
				&items[item_index], item_index + 1u, &resolve_x1,
				&resolve_y1, &resolve_x2, &resolve_y2);
			item_index++;
		} else {
			result = FB_GFX3_INVALID;
		}
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (item_index != primitive_count) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	if ((resolve_x1 > resolve_x2) || (resolve_y1 > resolve_y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	groups_x = (maximum_line_count + FB_GFX3_OPENGL_POINT_GROUP_SIZE - 1u) /
		FB_GFX3_OPENGL_POINT_GROUP_SIZE;
	if ((groups_x > state->maximum_compute_groups_x) ||
	    (primitive_count > state->maximum_compute_groups_y) ||
	    ((uint64_t)primitive_count * sizeof(items[0]) >
	     state->maximum_storage_buffer_size)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)((size_t)primitive_count * sizeof(items[0])),
		items, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 2,
		state->blit_batch_buffer);
	if (state->blit_batch_generation >= (UINT32_MAX >> 13))
		result = opengl_blit_batch_winner_ensure(state, destination, TRUE);
	else
		result = opengl_blit_batch_winner_ensure(state, destination, FALSE);
	if (result != FB_GFX3_OK)
		goto done;
	generation = ++state->blit_batch_generation;
	state->gl.use_program(state->primitive_batch_program);
	state->gl.bind_image_texture(0, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_1ui(state->primitive_batch_key_location,
		generation << 13);
	state->gl.dispatch_compute(groups_x, primitive_count, 1u);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT);
	result = opengl_check_error(state,
		"mixed primitive selection dispatch");
	if (result != FB_GFX3_OK)
		goto done;
	groups_x = ((uint32_t)(resolve_x2 - resolve_x1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_X - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_X;
	groups_y = ((uint32_t)(resolve_y2 - resolve_y1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_Y - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_Y;
	state->gl.use_program(state->primitive_batch_resolve_program);
	state->gl.bind_image_texture(0, destination->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.bind_image_texture(1, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
	state->gl.uniform_4i(state->primitive_batch_resolve_rect_location,
		resolve_x1, resolve_y1, resolve_x2 - resolve_x1 + 1,
		resolve_y2 - resolve_y1 + 1);
	state->gl.uniform_1ui(
		state->primitive_batch_resolve_generation_location, generation);
	state->gl.uniform_1ui(state->primitive_batch_resolve_mask_location,
		opengl_color_mask(destination->depth));
	state->gl.dispatch_compute(groups_x, groups_y, 1u);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	result = opengl_check_error(state, "mixed primitive resolve dispatch");

done:
	if (destination != NULL)
		fb_gfx3_resource_release(state->resources, commands[0]->target);
	free(items);
	return result;
}

static int opengl_line_batch_common(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands,
	const FB_GFX3_LINE_COMMAND *packed_lines, FB_GFX3_HANDLE target,
	uint64_t sequence, uint32_t count)
{
	FB_GFX3_OPENGL_LINE_BATCH_ITEM items[FB_GFX3_OPENGL_LINE_BATCH_LIMIT];
	const FB_GFX3_LINE_COMMAND *first;
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	FB_GFX3_RECT clip;
	uint32_t maximum_count = 0;
	uint32_t index;
	uint32_t generation;
	uint32_t groups_x;
	uint32_t groups_y;
	int32_t resolve_x1 = INT32_MAX;
	int32_t resolve_y1 = INT32_MAX;
	int32_t resolve_x2 = INT32_MIN;
	int32_t resolve_y2 = INT32_MIN;
	int result;

	if ((state == NULL) || (target == 0) || (sequence == 0u) ||
	    (count == 0u) || (count > FB_GFX3_OPENGL_LINE_BATCH_LIMIT) ||
	    ((commands == NULL) == (packed_lines == NULL)))
		return FB_GFX3_INVALID;
	if (packed_lines != NULL) {
		first = &packed_lines[0];
	} else {
		if ((commands[0] == NULL) ||
		    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
			return FB_GFX3_INVALID;
		first = (const FB_GFX3_LINE_COMMAND *)commands[0]->payload;
	}
	result = opengl_surface_retain(state, target, sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	if (!opengl_clip_rect(destination, &first->clip, &clip)) {
		fb_gfx3_resource_release(state->resources, target);
		return FB_GFX3_OK;
	}
	for (index = 0; index < count; index++) {
		const FB_GFX3_LINE_COMMAND *payload;
		int64_t difference_x;
		int64_t difference_y;
		int32_t x1;
		int32_t y1;
		int32_t x2;
		int32_t y2;
		uint32_t line_count;

		if (packed_lines != NULL) {
			payload = &packed_lines[index];
		} else {
			if ((commands[index] == NULL) ||
			    (commands[index]->type != FB_GFX3_COMMAND_LINE) ||
			    (commands[index]->target != target) ||
			    (fb_gfx3_command_payload_size(commands[index]) !=
			     sizeof(*payload))) {
				result = FB_GFX3_INVALID;
				goto done;
			}
			payload =
				(const FB_GFX3_LINE_COMMAND *)commands[index]->payload;
		}
		if ((payload->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0 ||
		    (payload->style != first->style) ||
		    (memcmp(&payload->clip, &first->clip, sizeof(payload->clip)) != 0)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		difference_x = (int64_t)payload->x2 - payload->x1;
		difference_y = (int64_t)payload->y2 - payload->y1;
		if (difference_x < 0)
			difference_x = -difference_x;
		if (difference_y < 0)
			difference_y = -difference_y;
		if ((difference_x > 32767) || (difference_y > 32767)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		line_count = (uint32_t)((difference_x > difference_y) ?
			difference_x : difference_y) + 1u;
		if (line_count > maximum_count)
			maximum_count = line_count;
		items[index].x1 = payload->x1;
		items[index].y1 = payload->y1;
		items[index].x2 = payload->x2;
		items[index].y2 = payload->y2;
		items[index].color = payload->color;
		items[index].reserved1 = 0;
		items[index].reserved2 = 0;
		items[index].reserved3 = 0;
		x1 = (payload->x1 < payload->x2) ? payload->x1 : payload->x2;
		x2 = (payload->x1 > payload->x2) ? payload->x1 : payload->x2;
		y1 = (payload->y1 < payload->y2) ? payload->y1 : payload->y2;
		y2 = (payload->y1 > payload->y2) ? payload->y1 : payload->y2;
		if (x1 < clip.x1)
			x1 = clip.x1;
		if (y1 < clip.y1)
			y1 = clip.y1;
		if (x2 > clip.x2)
			x2 = clip.x2;
		if (y2 > clip.y2)
			y2 = clip.y2;
		if ((x1 <= x2) && (y1 <= y2)) {
			if (x1 < resolve_x1)
				resolve_x1 = x1;
			if (y1 < resolve_y1)
				resolve_y1 = y1;
			if (x2 > resolve_x2)
				resolve_x2 = x2;
			if (y2 > resolve_y2)
				resolve_y2 = y2;
		}
	}
	if ((resolve_x1 > resolve_x2) || (resolve_y1 > resolve_y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	groups_x = (maximum_count + FB_GFX3_OPENGL_POINT_GROUP_SIZE - 1u) /
		FB_GFX3_OPENGL_POINT_GROUP_SIZE;
	if ((groups_x > state->maximum_compute_groups_x) ||
	    (count > state->maximum_compute_groups_y) ||
	    ((uint64_t)count * sizeof(items[0]) >
	     state->maximum_storage_buffer_size)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)(count * sizeof(items[0])), items, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 2,
		state->blit_batch_buffer);
	if (state->blit_batch_generation >= (UINT32_MAX >> 13))
		result = opengl_blit_batch_winner_ensure(state, destination, TRUE);
	else
		result = opengl_blit_batch_winner_ensure(state, destination, FALSE);
	if (result != FB_GFX3_OK)
		goto done;
	generation = ++state->blit_batch_generation;
	state->gl.use_program(state->line_batch_program);
	state->gl.bind_image_texture(0, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_4i(state->line_batch_clip_location, clip.x1, clip.y1,
		clip.x2, clip.y2);
	state->gl.uniform_1ui(state->line_batch_style_location,
		first->style & 0xFFFFu);
	state->gl.uniform_1ui(state->line_batch_key_location,
		(generation << 13) | 1u);
	state->gl.dispatch_compute(groups_x, count, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT);
	result = opengl_check_error(state, "line batch selection dispatch");
	if (result != FB_GFX3_OK)
		goto done;
	groups_x = ((uint32_t)(resolve_x2 - resolve_x1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_X - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_X;
	groups_y = ((uint32_t)(resolve_y2 - resolve_y1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_Y - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_Y;
	state->gl.use_program(state->line_batch_resolve_program);
	state->gl.bind_image_texture(0, destination->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.bind_image_texture(1, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
	state->gl.uniform_4i(state->line_batch_resolve_rect_location,
		resolve_x1, resolve_y1, resolve_x2 - resolve_x1 + 1,
		resolve_y2 - resolve_y1 + 1);
	state->gl.uniform_1ui(state->line_batch_resolve_generation_location,
		generation);
	state->gl.uniform_1ui(state->line_batch_resolve_mask_location,
		opengl_color_mask(destination->depth));
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	result = opengl_check_error(state, "line batch resolve dispatch");

done:
	fb_gfx3_resource_release(state->resources, target);
	return result;
}

static int opengl_line_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count)
{
	if ((commands == NULL) || (count == 0u) || (commands[0] == NULL) ||
	    (commands[count - 1u] == NULL))
		return FB_GFX3_INVALID;
	return opengl_line_batch_common(state, commands, NULL,
		commands[0]->target, commands[count - 1u]->sequence, count);
}

static int opengl_lines(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_LINES_COMMAND *payload;
	size_t line_bytes;
	size_t expected_size;

	if ((state == NULL) || (command == NULL) ||
	    (fb_gfx3_command_payload_size(command) <
	     offsetof(FB_GFX3_LINES_COMMAND, line)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_LINES_COMMAND *)command->payload;
	if ((payload->count == 0u) ||
	    (payload->count > FB_GFX3_OPENGL_LINE_BATCH_LIMIT) ||
	    (fb_gfx3_size_multiply(payload->count, sizeof(payload->line[0]),
	     &line_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_LINES_COMMAND, line), line_bytes,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)))
		return FB_GFX3_INVALID;
	return opengl_line_batch_common(state, NULL, payload->line,
		command->target, command->sequence, payload->count);
}

static int opengl_blit_raster_batch(FB_GFX3_OPENGL_STATE *state,
	const FB_GFX3_OPENGL_SURFACE *destination,
	const FB_GFX3_OPENGL_SURFACE *source,
	const FB_GFX3_OPENGL_BLIT_BATCH_ITEM *items, uint32_t count,
	uint32_t mode, GLenum logic_operation)
{
	int result;

	if ((state == NULL) || (destination == NULL) || (source == NULL) ||
	    (items == NULL) || (count == 0u) ||
	    (source == destination) || (state->blit_raster_batch_program == 0))
		return FB_GFX3_INVALID;
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
	result = opengl_bind_color_framebuffer(state, destination->texture);
	if (result != FB_GFX3_OK)
		return result;
	state->gl.viewport(0, 0, (GLsizei)destination->width,
		(GLsizei)destination->height);
	state->gl.use_program(state->blit_raster_batch_program);
	state->gl.active_texture(GL_TEXTURE0);
	state->gl.bind_texture(GL_TEXTURE_2D, source->texture);
	state->gl.uniform_1i(state->blit_raster_batch_source_location, 0);
	state->gl.uniform_2f(state->blit_raster_batch_size_location,
		(GLfloat)destination->width,
		(GLfloat)destination->height);
	state->gl.uniform_1ui(state->blit_raster_batch_mode_location, mode);
	state->gl.uniform_1ui(state->blit_raster_batch_depth_location,
		destination->depth);
	state->gl.uniform_1ui(state->blit_raster_batch_mask_location,
		opengl_color_mask(destination->depth));
	state->gl.bind_vertex_array(state->blit_batch_vertex_array);
	state->gl.bind_buffer(GL_ARRAY_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_ARRAY_BUFFER, (GLsizeiptr)(count * sizeof(*items)),
		items, GL_STREAM_DRAW);
	if (logic_operation != 0u) {
		state->gl.enable(GL_COLOR_LOGIC_OP);
		state->gl.logic_op(logic_operation);
	}
	state->gl.draw_arrays_instanced(GL_TRIANGLES, 0, 6, (GLsizei)count);
	if (logic_operation != 0u)
		state->gl.disable(GL_COLOR_LOGIC_OP);
	state->gl.memory_barrier(GL_FRAMEBUFFER_BARRIER_BIT |
		GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	result = opengl_check_error(state, "ordered blit raster batch");
	return result;
}

static int opengl_ellipse_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count)
{
	FB_GFX3_OPENGL_ELLIPSE_BATCH_ITEM items[
		FB_GFX3_OPENGL_ELLIPSE_BATCH_LIMIT];
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	int32_t resolve_x1 = INT32_MAX;
	int32_t resolve_y1 = INT32_MAX;
	int32_t resolve_x2 = INT32_MIN;
	int32_t resolve_y2 = INT32_MIN;
	uint32_t generation;
	uint32_t groups_x;
	uint32_t groups_y;
	uint32_t index;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_OPENGL_ELLIPSE_BATCH_LIMIT) ||
	    (commands[0] == NULL))
		return FB_GFX3_INVALID;
	result = opengl_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	for (index = 0u; index < count; ++index) {
		const FB_GFX3_ELLIPSE_COMMAND *payload;
		FB_GFX3_RECT clip;
		double x1;
		double y1;
		double x2;
		double y2;
		int32_t left;
		int32_t top;
		int32_t right;
		int32_t bottom;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_ELLIPSE) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) != sizeof(*payload))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		payload = (const FB_GFX3_ELLIPSE_COMMAND *)commands[index]->payload;
		if ((payload->filled == 0u) ||
		    ((payload->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
		    !(payload->radius_x >= 0.0f) ||
		    !(payload->radius_x <= 32767.0f) ||
		    !(payload->radius_y >= 0.0f) ||
		    !(payload->radius_y <= 32767.0f)) {
			result = FB_GFX3_UNSUPPORTED;
			goto done;
		}
		if (!opengl_clip_rect(destination, &payload->clip, &clip)) {
			items[index].center_x = payload->center_x;
			items[index].center_y = payload->center_y;
			memcpy(&items[index].radius_x_bits, &payload->radius_x,
				sizeof(items[index].radius_x_bits));
			memcpy(&items[index].radius_y_bits, &payload->radius_y,
				sizeof(items[index].radius_y_bits));
			items[index].clip_x1 = 1;
			items[index].clip_y1 = 1;
			items[index].clip_x2 = 0;
			items[index].clip_y2 = 0;
			items[index].color = payload->color;
			items[index].reserved1 = 0u;
			items[index].reserved2 = 0u;
			items[index].reserved3 = 0u;
			continue;
		}
		x1 = (double)payload->center_x - payload->radius_x;
		y1 = (double)payload->center_y - payload->radius_y;
		x2 = (double)payload->center_x + payload->radius_x;
		y2 = (double)payload->center_y + payload->radius_y;
		if ((x1 < INT32_MIN) || (y1 < INT32_MIN) ||
		    (x2 > INT32_MAX) || (y2 > INT32_MAX)) {
			result = FB_GFX3_UNSUPPORTED;
			goto done;
		}
		left = (int32_t)x1;
		top = (int32_t)y1;
		right = (int32_t)x2;
		bottom = (int32_t)y2;
		if (left < clip.x1)
			left = clip.x1;
		if (top < clip.y1)
			top = clip.y1;
		if (right > clip.x2)
			right = clip.x2;
		if (bottom > clip.y2)
			bottom = clip.y2;
		if ((left <= right) && (top <= bottom)) {
			if (left < resolve_x1)
				resolve_x1 = left;
			if (top < resolve_y1)
				resolve_y1 = top;
			if (right > resolve_x2)
				resolve_x2 = right;
			if (bottom > resolve_y2)
				resolve_y2 = bottom;
		}
		items[index].center_x = payload->center_x;
		items[index].center_y = payload->center_y;
		memcpy(&items[index].radius_x_bits, &payload->radius_x,
			sizeof(items[index].radius_x_bits));
		memcpy(&items[index].radius_y_bits, &payload->radius_y,
			sizeof(items[index].radius_y_bits));
		items[index].clip_x1 = clip.x1;
		items[index].clip_y1 = clip.y1;
		items[index].clip_x2 = clip.x2;
		items[index].clip_y2 = clip.y2;
		items[index].color = payload->color;
		items[index].reserved1 = 0u;
		items[index].reserved2 = 0u;
		items[index].reserved3 = 0u;
	}
	if ((resolve_x1 > resolve_x2) || (resolve_y1 > resolve_y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	if ((count > state->maximum_compute_groups_z) ||
	    ((uint64_t)count * sizeof(items[0]) >
	     state->maximum_storage_buffer_size)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)(count * sizeof(items[0])), items, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 2,
		state->blit_batch_buffer);
	if (state->blit_batch_generation >= (UINT32_MAX >> 13))
		result = opengl_blit_batch_winner_ensure(state, destination, TRUE);
	else
		result = opengl_blit_batch_winner_ensure(state, destination, FALSE);
	if (result != FB_GFX3_OK)
		goto done;
	generation = ++state->blit_batch_generation;
	state->gl.use_program(state->ellipse_batch_program);
	state->gl.bind_image_texture(0, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	state->gl.uniform_1ui(state->ellipse_batch_key_location,
		(generation << 13) | 1u);
	state->gl.dispatch_compute(1, 1, count);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT);
	result = opengl_check_error(state, "ellipse batch selection dispatch");
	if (result != FB_GFX3_OK)
		goto done;
	groups_x = ((uint32_t)(resolve_x2 - resolve_x1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_X - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_X;
	groups_y = ((uint32_t)(resolve_y2 - resolve_y1 + 1) +
		FB_GFX3_OPENGL_LOCAL_SIZE_Y - 1u) / FB_GFX3_OPENGL_LOCAL_SIZE_Y;
	state->gl.use_program(state->ellipse_batch_resolve_program);
	state->gl.bind_image_texture(0, destination->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.bind_image_texture(1, state->blit_batch_winner_texture, 0,
		GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
	state->gl.uniform_4i(state->ellipse_batch_resolve_rect_location,
		resolve_x1, resolve_y1, resolve_x2 - resolve_x1 + 1,
		resolve_y2 - resolve_y1 + 1);
	state->gl.uniform_1ui(state->ellipse_batch_resolve_generation_location,
		generation);
	state->gl.uniform_1ui(state->ellipse_batch_resolve_mask_location,
		opengl_color_mask(destination->depth));
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	result = opengl_check_error(state, "ellipse batch resolve dispatch");

done:
	fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

static int opengl_blit_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count)
{
	const FB_GFX3_BLIT_COMMAND *payload;
	FB_GFX3_OPENGL_BLIT_BATCH_ITEM items[
		FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT];
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	FB_GFX3_OPENGL_SURFACE *source = NULL;
	FB_GFX3_RECT clip;
	uint32_t item_count = 0;
	uint32_t index;
	GLenum logic_operation = 0;
	int64_t draw_x2;
	int64_t draw_y2;
	int result = FB_GFX3_OK;

	if ((count < 2) || (count > FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	if ((commands[0] == NULL) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	for (index = 1u; index < count; index++) {
		if ((commands[index] == NULL) ||
		    (commands[index]->sequence <= commands[index - 1u]->sequence) ||
		    (fb_gfx3_command_payload_size(commands[index]) != sizeof(*payload)))
			return FB_GFX3_INVALID;
	}
	payload = (const FB_GFX3_BLIT_COMMAND *)commands[0]->payload;
	/*
		The coalescer accepts only one source and destination handle. Retaining
		them at the batch tail protects every command in this FIFO run while
		avoiding two registry operations for every individual sprite.
	*/
	result = opengl_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_surface_retain(state, payload->source,
		commands[count - 1u]->sequence, &source);
	if (result != FB_GFX3_OK)
		goto done;
	if ((source == destination) || (source->depth != destination->depth)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	for (index = 0; index < count; ++index) {
		if ((commands[index] == NULL) ||
		    ((index > 0u) && (commands[index]->sequence <=
		     commands[index - 1u]->sequence)) ||
		    (fb_gfx3_command_payload_size(commands[index]) != sizeof(*payload))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		payload = (const FB_GFX3_BLIT_COMMAND *)commands[index]->payload;
		if ((commands[index]->target != commands[0]->target) ||
		    (payload->source != ((const FB_GFX3_BLIT_COMMAND *)
			commands[0]->payload)->source) ||
		    ((index > 0u) &&
		     (commands[index]->sequence <= commands[index - 1u]->sequence)) ||
		    (payload->source_rect.x1 < 0) ||
		    (payload->source_rect.y1 < 0) ||
		    (payload->source_rect.x1 > payload->source_rect.x2) ||
		    (payload->source_rect.y1 > payload->source_rect.y2) ||
		    (payload->source_rect.x2 >= (int32_t)source->width) ||
		    (payload->source_rect.y2 >= (int32_t)source->height)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (!opengl_clip_rect(destination, &payload->clip, &clip))
			continue;
		draw_x2 = (int64_t)payload->destination_x +
			payload->source_rect.x2 - payload->source_rect.x1;
		draw_y2 = (int64_t)payload->destination_y +
			payload->source_rect.y2 - payload->source_rect.y1;
		if ((draw_x2 < clip.x1) || (draw_y2 < clip.y1) ||
		    (payload->destination_x > clip.x2) ||
		    (payload->destination_y > clip.y2))
			continue;
		items[item_count].source_x = payload->source_rect.x1;
		items[item_count].source_y = payload->source_rect.y1;
		items[item_count].width = payload->source_rect.x2 -
			payload->source_rect.x1 + 1;
		items[item_count].height = payload->source_rect.y2 -
			payload->source_rect.y1 + 1;
		items[item_count].clip_x1 = clip.x1;
		items[item_count].clip_y1 = clip.y1;
		items[item_count].clip_x2 = clip.x2;
		items[item_count].clip_y2 = clip.y2;
		items[item_count].destination_x = payload->destination_x;
		items[item_count].destination_y = payload->destination_y;
		items[item_count].reserved1 = 0;
		items[item_count].reserved2 = 0;
		item_count++;
	}
	if (item_count != 0u) {
		payload = (const FB_GFX3_BLIT_COMMAND *)commands[0]->payload;
		switch (payload->mode) {
		case FB_GFX3_BLIT_AND:
			logic_operation = GL_AND;
			break;
		case FB_GFX3_BLIT_OR:
			logic_operation = GL_OR;
			break;
		case FB_GFX3_BLIT_XOR:
			logic_operation = GL_XOR;
			break;
		default:
			break;
		}
		result = opengl_blit_raster_batch(state, destination, source, items,
			item_count, payload->mode, logic_operation);
	}

done:
	if (source != NULL)
		fb_gfx3_resource_release(state->resources,
			((const FB_GFX3_BLIT_COMMAND *)commands[0]->payload)->source);
	if (destination != NULL)
		fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

/*
	Execute one adjacent source/mode run from a heterogeneous BLITS packet. The
	packet preserves BASIC order, while each run still uses the existing
	instanced GPU path which binds one source texture.
*/
static int opengl_blits_run(FB_GFX3_OPENGL_STATE *state,
	const FB_GFX3_COMMAND *command, FB_GFX3_OPENGL_SURFACE *destination,
	const FB_GFX3_BLIT_COMMAND *blits, uint32_t count)
{
	FB_GFX3_OPENGL_BLIT_BATCH_ITEM items[
		FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT];
	FB_GFX3_OPENGL_SURFACE *source = NULL;
	FB_GFX3_HANDLE source_handle;
	uint32_t mode;
	uint32_t alpha;
	uint32_t item_count = 0u;
	uint32_t index;
	GLenum logic_operation = 0;
	int result;

	if ((state == NULL) || (command == NULL) || (destination == NULL) ||
	    (blits == NULL) || (count == 0u) ||
	    (count > FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	source_handle = blits[0].source;
	mode = blits[0].mode;
	alpha = blits[0].alpha;
	if ((source_handle == 0) || (source_handle == command->target))
		return FB_GFX3_INVALID;
	result = opengl_surface_retain(state, source_handle, command->sequence,
		&source);
	if (result != FB_GFX3_OK)
		return result;
	if (source->depth != destination->depth) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	for (index = 0u; index < count; index++) {
		const FB_GFX3_BLIT_COMMAND *blit = &blits[index];

		if ((blit->source != source_handle) || (blit->mode != mode) ||
		    (blit->alpha != alpha) ||
		    (blit->source_rect.x1 < 0) || (blit->source_rect.y1 < 0) ||
		    (blit->source_rect.x1 > blit->source_rect.x2) ||
		    (blit->source_rect.y1 > blit->source_rect.y2) ||
		    (blit->source_rect.x2 >= (int32_t)source->width) ||
		    (blit->source_rect.y2 >= (int32_t)source->height)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		items[item_count].source_x = blit->source_rect.x1;
		items[item_count].source_y = blit->source_rect.y1;
		items[item_count].width = blit->source_rect.x2 -
			blit->source_rect.x1 + 1;
		items[item_count].height = blit->source_rect.y2 -
			blit->source_rect.y1 + 1;
		/*
			The vertex and fragment stages carry the public geometry and clip.
			Framebuffer clipping rejects offscreen triangles before fragment work;
			the fragment comparison applies a smaller VIEW without CPU trimming.
		*/
		items[item_count].clip_x1 = blit->clip.x1;
		items[item_count].clip_y1 = blit->clip.y1;
		items[item_count].clip_x2 = blit->clip.x2;
		items[item_count].clip_y2 = blit->clip.y2;
		items[item_count].destination_x = blit->destination_x;
		items[item_count].destination_y = blit->destination_y;
		items[item_count].reserved1 = 0u;
		items[item_count].reserved2 = 0u;
		item_count++;
	}
	switch (mode) {
	case FB_GFX3_BLIT_AND:
		logic_operation = GL_AND;
		break;
	case FB_GFX3_BLIT_OR:
		logic_operation = GL_OR;
		break;
	case FB_GFX3_BLIT_XOR:
		logic_operation = GL_XOR;
		break;
	default:
		break;
	}
	if (item_count != 0u)
		result = opengl_blit_raster_batch(state, destination, source, items,
			item_count, mode, logic_operation);

done:
	if (source != NULL)
		fb_gfx3_resource_release(state->resources, source_handle);
	return result;
}

/*
	The producer keeps an entire ordered sprite stream in one allocation even
	when adjacent PUTs use different cached images. Split that stream only at
	texture-binding boundaries on the renderer thread. This removes command and
	queue overhead without reordering overlapping sprites.
*/
static int opengl_blits(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_BLITS_COMMAND *payload;
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	size_t blit_bytes;
	size_t expected_size;
	uint32_t run_start;
	int result;

	if ((state == NULL) || (command == NULL) ||
	    (fb_gfx3_command_payload_size(command) <
	     offsetof(FB_GFX3_BLITS_COMMAND, blit)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_BLITS_COMMAND *)command->payload;
	if ((payload->count == 0u) ||
	    (payload->count > FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT) ||
	    (fb_gfx3_size_multiply(payload->count, sizeof(payload->blit[0]),
	     &blit_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_BLITS_COMMAND, blit), blit_bytes,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)) ||
	    (payload->source != payload->blit[0].source) ||
	    (payload->mode != payload->blit[0].mode) ||
	    (payload->alpha != payload->blit[0].alpha))
		return FB_GFX3_INVALID;
	result = opengl_surface_retain(state, command->target, command->sequence,
		&destination);
	if (result != FB_GFX3_OK)
		return result;
	run_start = 0u;
	while (run_start < payload->count) {
		uint32_t run_end = run_start + 1u;
		const FB_GFX3_BLIT_COMMAND *first = &payload->blit[run_start];

		while ((run_end < payload->count) &&
		       (payload->blit[run_end].source == first->source) &&
		       (payload->blit[run_end].mode == first->mode) &&
		       (payload->blit[run_end].alpha == first->alpha))
			run_end++;
		result = opengl_blits_run(state, command, destination,
			payload->blit + run_start, run_end - run_start);
		if (result != FB_GFX3_OK)
			break;
		run_start = run_end;
	}
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int opengl_rectangle_raster_batch(FB_GFX3_OPENGL_STATE *state,
	const FB_GFX3_OPENGL_SURFACE *destination,
	const FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM *items, uint32_t count)
{
	int result;

	if ((state == NULL) || (destination == NULL) || (items == NULL) ||
	    (count == 0u) || (state->rectangle_raster_batch_program == 0u))
		return FB_GFX3_INVALID;
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
	result = opengl_bind_color_framebuffer(state, destination->texture);
	if (result != FB_GFX3_OK)
		return result;
	state->gl.viewport(0, 0, (GLsizei)destination->width,
		(GLsizei)destination->height);
	state->gl.use_program(state->rectangle_raster_batch_program);
	state->gl.uniform_2f(state->rectangle_raster_batch_size_location,
		(GLfloat)destination->width, (GLfloat)destination->height);
	state->gl.uniform_1ui(state->rectangle_raster_batch_mask_location,
		opengl_color_mask(destination->depth));
	state->gl.bind_vertex_array(state->rectangle_batch_vertex_array);
	state->gl.bind_buffer(GL_ARRAY_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_ARRAY_BUFFER, (GLsizeiptr)(count * sizeof(*items)),
		items, GL_STREAM_DRAW);
	state->gl.draw_arrays_instanced(GL_TRIANGLES, 0, 6, (GLsizei)count);
	state->gl.memory_barrier(GL_FRAMEBUFFER_BARRIER_BIT |
		GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	result = opengl_check_error(state, "ordered rectangle raster batch");
	return result;
}

static int opengl_alpha_blit_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count)
{
	FB_GFX3_OPENGL_BLIT_BATCH_ITEM *items = NULL;
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	FB_GFX3_OPENGL_SURFACE *source = NULL;
	uint32_t *tile_counts = NULL;
	uint32_t *tile_ranges = NULL;
	uint32_t *tile_cursors = NULL;
	uint32_t *tile_indices = NULL;
	const FB_GFX3_BLIT_COMMAND *first;
	uint32_t tiles_x;
	uint32_t tiles_y;
	uint32_t tile_count;
	uint32_t index_count = 0;
	uint32_t index;
	uint32_t item_count = 0;
	uint32_t groups_x;
	uint32_t groups_y;
	int result = FB_GFX3_OK;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_OPENGL_RASTER_BLIT_BATCH_LIMIT) ||
	    (commands[0] == NULL) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return FB_GFX3_INVALID;
	first = (const FB_GFX3_BLIT_COMMAND *)commands[0]->payload;
	if (((first->mode != FB_GFX3_BLIT_TRANS) &&
	     (first->mode != FB_GFX3_BLIT_PSET) &&
	     (first->mode != FB_GFX3_BLIT_PRESET) &&
	     (first->mode != FB_GFX3_BLIT_AND) &&
	     (first->mode != FB_GFX3_BLIT_OR) &&
	     (first->mode != FB_GFX3_BLIT_XOR) &&
	     (first->mode != FB_GFX3_BLIT_ALPHA) &&
	     (first->mode != FB_GFX3_BLIT_BLEND) &&
	     (first->mode != FB_GFX3_BLIT_ADD)) ||
	    (first->source == commands[0]->target))
		return FB_GFX3_UNSUPPORTED;
	result = opengl_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_surface_retain(state, first->source,
		commands[count - 1u]->sequence, &source);
	if (result != FB_GFX3_OK)
		goto done;
	if ((source == destination) || (source->depth != destination->depth)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	tiles_x = (destination->width + FB_GFX3_OPENGL_LOCAL_SIZE_X - 1u) /
		FB_GFX3_OPENGL_LOCAL_SIZE_X;
	tiles_y = (destination->height + FB_GFX3_OPENGL_LOCAL_SIZE_Y - 1u) /
		FB_GFX3_OPENGL_LOCAL_SIZE_Y;
	if ((tiles_x == 0u) || (tiles_y == 0u) ||
	    (tiles_x > UINT32_MAX / tiles_y) ||
	    ((tile_count = tiles_x * tiles_y) > FB_GFX3_OPENGL_ALPHA_TILE_MAX_TILES)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	items = (FB_GFX3_OPENGL_BLIT_BATCH_ITEM *)calloc(count, sizeof(*items));
	tile_counts = (uint32_t *)calloc(tile_count, sizeof(*tile_counts));
	if ((items == NULL) || (tile_counts == NULL)) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	for (index = 0u; index < count; ++index) {
		const FB_GFX3_BLIT_COMMAND *payload;
		FB_GFX3_RECT clip;
		int64_t right;
		int64_t bottom;
		int32_t left;
		int32_t top;
		int32_t clipped_right;
		int32_t clipped_bottom;
		uint32_t first_tile_x;
		uint32_t first_tile_y;
		uint32_t last_tile_x;
		uint32_t last_tile_y;
		uint32_t tile_y;

		if ((commands[index] == NULL) ||
		    ((index > 0u) && (commands[index]->sequence <=
		     commands[index - 1u]->sequence)) ||
		    (fb_gfx3_command_payload_size(commands[index]) != sizeof(*payload))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		payload = (const FB_GFX3_BLIT_COMMAND *)commands[index]->payload;
		if ((commands[index]->target != commands[0]->target) ||
		    (payload->source != first->source) ||
			(payload->mode != first->mode) ||
			(((first->mode == FB_GFX3_BLIT_BLEND) ||
			  (first->mode == FB_GFX3_BLIT_ADD)) &&
			 (payload->alpha != first->alpha)) ||
		    (payload->source_rect.x1 < 0) || (payload->source_rect.y1 < 0) ||
		    (payload->source_rect.x1 > payload->source_rect.x2) ||
		    (payload->source_rect.y1 > payload->source_rect.y2) ||
		    (payload->source_rect.x2 >= (int32_t)source->width) ||
		    (payload->source_rect.y2 >= (int32_t)source->height)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (!opengl_clip_rect(destination, &payload->clip, &clip))
			continue;
		right = (int64_t)payload->destination_x +
			payload->source_rect.x2 - payload->source_rect.x1;
		bottom = (int64_t)payload->destination_y +
			payload->source_rect.y2 - payload->source_rect.y1;
		if ((right < clip.x1) || (bottom < clip.y1) ||
		    (payload->destination_x > clip.x2) ||
		    (payload->destination_y > clip.y2))
			continue;
		left = (payload->destination_x > clip.x1) ? payload->destination_x : clip.x1;
		top = (payload->destination_y > clip.y1) ? payload->destination_y : clip.y1;
		clipped_right = (right < clip.x2) ? (int32_t)right : clip.x2;
		clipped_bottom = (bottom < clip.y2) ? (int32_t)bottom : clip.y2;
		items[item_count].source_x = payload->source_rect.x1;
		items[item_count].source_y = payload->source_rect.y1;
		items[item_count].width = payload->source_rect.x2 - payload->source_rect.x1 + 1;
		items[item_count].height = payload->source_rect.y2 - payload->source_rect.y1 + 1;
		items[item_count].clip_x1 = clip.x1;
		items[item_count].clip_y1 = clip.y1;
		items[item_count].clip_x2 = clip.x2;
		items[item_count].clip_y2 = clip.y2;
		items[item_count].destination_x = payload->destination_x;
		items[item_count].destination_y = payload->destination_y;
		first_tile_x = (uint32_t)left / FB_GFX3_OPENGL_LOCAL_SIZE_X;
		first_tile_y = (uint32_t)top / FB_GFX3_OPENGL_LOCAL_SIZE_Y;
		last_tile_x = (uint32_t)clipped_right / FB_GFX3_OPENGL_LOCAL_SIZE_X;
		last_tile_y = (uint32_t)clipped_bottom / FB_GFX3_OPENGL_LOCAL_SIZE_Y;
		for (tile_y = first_tile_y; tile_y <= last_tile_y; ++tile_y) {
			uint32_t tile_x;
			for (tile_x = first_tile_x; tile_x <= last_tile_x; ++tile_x) {
				uint32_t tile = tile_y * tiles_x + tile_x;
				if (tile_counts[tile] == UINT32_MAX) {
					result = FB_GFX3_OUT_OF_MEMORY;
					goto done;
				}
				tile_counts[tile]++;
			}
		}
		item_count++;
	}
	if (item_count == 0u)
		goto done;
	for (index = 0u; index < tile_count; ++index) {
		if (tile_counts[index] > UINT32_MAX - index_count) {
			result = FB_GFX3_OUT_OF_MEMORY;
			goto done;
		}
		index_count += tile_counts[index];
	}
	/*
		Every retained item intersects at least one tile.  Keep the
		invariant explicit so zero-sized allocations cannot reach either
		the C allocator or the OpenGL buffer upload path.
	*/
	if (index_count == 0u) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	tile_ranges = (uint32_t *)calloc((size_t)tile_count * 2u, sizeof(*tile_ranges));
	tile_cursors = (uint32_t *)calloc(tile_count, sizeof(*tile_cursors));
	tile_indices = (uint32_t *)malloc((size_t)index_count * sizeof(*tile_indices));
	if ((tile_ranges == NULL) || (tile_cursors == NULL) ||
	    (tile_indices == NULL)) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	index_count = 0u;
	for (index = 0u; index < tile_count; ++index) {
		tile_ranges[index * 2u] = index_count;
		tile_ranges[index * 2u + 1u] = tile_counts[index];
		tile_cursors[index] = index_count;
		index_count += tile_counts[index];
	}
	for (index = 0u; index < item_count; ++index) {
		const FB_GFX3_OPENGL_BLIT_BATCH_ITEM *item = &items[index];
		int32_t left = (item->destination_x > item->clip_x1) ? item->destination_x : item->clip_x1;
		int32_t top = (item->destination_y > item->clip_y1) ? item->destination_y : item->clip_y1;
		int32_t right = ((int64_t)item->destination_x + item->width - 1 < item->clip_x2) ?
			(int32_t)((int64_t)item->destination_x + item->width - 1) : item->clip_x2;
		int32_t bottom = ((int64_t)item->destination_y + item->height - 1 < item->clip_y2) ?
			(int32_t)((int64_t)item->destination_y + item->height - 1) : item->clip_y2;
		uint32_t tile_y;
		for (tile_y = (uint32_t)top / FB_GFX3_OPENGL_LOCAL_SIZE_Y;
		     tile_y <= (uint32_t)bottom / FB_GFX3_OPENGL_LOCAL_SIZE_Y; ++tile_y) {
			uint32_t tile_x;
			for (tile_x = (uint32_t)left / FB_GFX3_OPENGL_LOCAL_SIZE_X;
			     tile_x <= (uint32_t)right / FB_GFX3_OPENGL_LOCAL_SIZE_X; ++tile_x) {
				uint32_t tile = tile_y * tiles_x + tile_x;
				tile_indices[tile_cursors[tile]++] = index;
			}
		}
	}
	groups_x = tiles_x;
	groups_y = tiles_y;
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_tile_range_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)((size_t)tile_count * 2u * sizeof(*tile_ranges)), tile_ranges,
		GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 2, state->blit_tile_range_buffer);
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_tile_index_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)((size_t)index_count * sizeof(*tile_indices)), tile_indices,
		GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 3, state->blit_tile_index_buffer);
	state->gl.bind_buffer(GL_SHADER_STORAGE_BUFFER, state->blit_batch_buffer);
	state->gl.buffer_data(GL_SHADER_STORAGE_BUFFER,
		(GLsizeiptr)((size_t)item_count * sizeof(*items)), items, GL_STREAM_DRAW);
	state->gl.bind_buffer_base(GL_SHADER_STORAGE_BUFFER, 4, state->blit_batch_buffer);
	state->gl.use_program(state->blit_alpha_tile_program);
	state->gl.bind_image_texture(0, destination->texture, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_R32UI);
	state->gl.bind_image_texture(1, source->texture, 0, GL_FALSE, 0,
		GL_READ_ONLY, GL_R32UI);
	state->gl.uniform_1ui(state->blit_alpha_tile_count_location, tiles_x);
	state->gl.uniform_1ui(state->blit_alpha_tile_mode_location, first->mode);
	state->gl.uniform_1ui(state->blit_alpha_tile_alpha_location, first->alpha);
	state->gl.uniform_1ui(state->blit_alpha_tile_depth_location,
		destination->depth);
	state->gl.uniform_1ui(state->blit_alpha_tile_mask_location,
		opengl_color_mask(destination->depth));
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
	result = opengl_check_error(state, "ordered tile blit compute dispatch");

done:
	free(tile_indices);
	free(tile_cursors);
	free(tile_ranges);
	free(tile_counts);
	free(items);
	if (source != NULL)
		fb_gfx3_resource_release(state->resources, first->source);
	if (destination != NULL)
		fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

static int opengl_rectangle_item_append(
	FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM *items, uint32_t *count,
	const FB_GFX3_RECT *clip, int32_t x1, int32_t y1, int32_t x2,
	int32_t y2, uint32_t color)
{
	FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM *item;

	if ((items == NULL) || (count == NULL) || (clip == NULL) ||
	    (x1 > x2) || (y1 > y2))
		return FB_GFX3_INVALID;
	if ((x2 < clip->x1) || (y2 < clip->y1) ||
	    (x1 > clip->x2) || (y1 > clip->y2))
		return FB_GFX3_OK;
	if (*count >= FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT)
		return FB_GFX3_OUT_OF_MEMORY;
	item = &items[(*count)++];
	item->x1 = (x1 < clip->x1) ? clip->x1 : x1;
	item->y1 = (y1 < clip->y1) ? clip->y1 : y1;
	item->x2 = (x2 > clip->x2) ? clip->x2 : x2;
	item->y2 = (y2 > clip->y2) ? clip->y2 : y2;
	item->color = color;
	item->reserved1 = 0u;
	item->reserved2 = 0u;
	item->reserved3 = 0u;
	return FB_GFX3_OK;
}

/*
	Producer-side rectangle packets remove one heap command per ordinary
	LINE ... B/BF. Their payload is already FIFO ordered, and framebuffer
	raster order preserves that same last-writer behavior. A solid outline is
	four non-overlapping quads, so it can share the filled-box instanced draw
	without asking the CPU to rasterize any pixels.
*/
static int opengl_rectangles(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_RECTANGLES_COMMAND *payload;
	FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM items[
		FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT];
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	size_t rectangle_bytes;
	size_t expected_size;
	uint32_t item_count = 0u;
	uint32_t index;
	int result;

	if ((state == NULL) || (command == NULL) ||
	    (fb_gfx3_command_payload_size(command) <
	     offsetof(FB_GFX3_RECTANGLES_COMMAND, rectangle)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_RECTANGLES_COMMAND *)command->payload;
	if ((payload->count == 0u) ||
	    (payload->count > FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT) ||
	    (fb_gfx3_size_multiply(payload->count, sizeof(payload->rectangle[0]),
	     &rectangle_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_RECTANGLES_COMMAND, rectangle),
	     rectangle_bytes, &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)))
		return FB_GFX3_INVALID;
	result = opengl_surface_retain(state, command->target, command->sequence,
		&destination);
	if (result != FB_GFX3_OK)
		return result;
	for (index = 0u; index < payload->count; ++index) {
		const FB_GFX3_RECTANGLE_COMMAND *rectangle = &payload->rectangle[index];
		FB_GFX3_RECT clip;

		if (((rectangle->filled == 0u) &&
		     ((rectangle->style & 0xFFFFu) != 0xFFFFu)) ||
		    ((rectangle->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
		    (rectangle->x1 > rectangle->x2) ||
		    (rectangle->y1 > rectangle->y2)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (!opengl_clip_rect(destination, &rectangle->clip, &clip) ||
		    (rectangle->x2 < clip.x1) || (rectangle->y2 < clip.y1) ||
		    (rectangle->x1 > clip.x2) || (rectangle->y1 > clip.y2))
			continue;
		if (rectangle->filled != 0u) {
			result = opengl_rectangle_item_append(items, &item_count, &clip,
				rectangle->x1, rectangle->y1, rectangle->x2,
				rectangle->y2, rectangle->color);
		} else {
			result = opengl_rectangle_item_append(items, &item_count, &clip,
				rectangle->x1, rectangle->y2, rectangle->x2,
				rectangle->y2, rectangle->color);
			if ((result == FB_GFX3_OK) &&
			    (rectangle->y1 != rectangle->y2))
				result = opengl_rectangle_item_append(items, &item_count,
					&clip, rectangle->x1, rectangle->y1,
					rectangle->x2, rectangle->y1, rectangle->color);
			if ((result == FB_GFX3_OK) &&
			    ((int64_t)rectangle->y2 - rectangle->y1 > 1))
				result = opengl_rectangle_item_append(items, &item_count,
					&clip, rectangle->x2, rectangle->y1 + 1,
					rectangle->x2, rectangle->y2 - 1,
					rectangle->color);
			if ((result == FB_GFX3_OK) &&
			    (rectangle->x1 != rectangle->x2) &&
			    ((int64_t)rectangle->y2 - rectangle->y1 > 1))
				result = opengl_rectangle_item_append(items, &item_count,
					&clip, rectangle->x1, rectangle->y1 + 1,
					rectangle->x1, rectangle->y2 - 1,
					rectangle->color);
		}
		if (result != FB_GFX3_OK)
			goto done;
	}
	if (item_count != 0u)
		result = opengl_rectangle_raster_batch(state, destination, items,
			item_count);

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int opengl_rectangle_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count)
{
	FB_GFX3_OPENGL_RECTANGLE_BATCH_ITEM items[
		FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT];
	FB_GFX3_OPENGL_SURFACE *destination = NULL;
	const FB_GFX3_RECTANGLE_COMMAND *payload;
	FB_GFX3_RECT clip;
	uint32_t item_count = 0;
	uint32_t index;
	int result = FB_GFX3_OK;

	if ((count < 2) || (count > FB_GFX3_OPENGL_RECTANGLE_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	/*
		The batch counter accepts one destination handle only. Retain that
		surface through the final sequence once, rather than performing a
		registry retain/release pair for every small filled box.
	*/
	result = opengl_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	for (index = 0; index < count; ++index) {
		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_RECTANGLE) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*payload))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		payload = (const FB_GFX3_RECTANGLE_COMMAND *)commands[index]->payload;
		if ((payload->filled == 0) ||
		    ((payload->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0) ||
		    (payload->x1 > payload->x2) || (payload->y1 > payload->y2)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (commands[index]->target != commands[0]->target) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (!opengl_clip_rect(destination, &payload->clip, &clip) ||
		    (payload->x2 < clip.x1) || (payload->y2 < clip.y1) ||
		    (payload->x1 > clip.x2) || (payload->y1 > clip.y2))
			continue;
		items[item_count].x1 = (payload->x1 < clip.x1) ? clip.x1 : payload->x1;
		items[item_count].y1 = (payload->y1 < clip.y1) ? clip.y1 : payload->y1;
		items[item_count].x2 = (payload->x2 > clip.x2) ? clip.x2 : payload->x2;
		items[item_count].y2 = (payload->y2 > clip.y2) ? clip.y2 : payload->y2;
		items[item_count].color = payload->color;
		items[item_count].reserved1 = 0;
		items[item_count].reserved2 = 0;
		items[item_count].reserved3 = 0;
		item_count++;
	}
	if (item_count == 0)
		goto done;
	if ((uint64_t)item_count * sizeof(items[0]) >
	    state->maximum_storage_buffer_size) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	result = opengl_rectangle_raster_batch(state, destination, items, item_count);

done:
	if (destination != NULL)
		fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

static int opengl_blit(FB_GFX3_OPENGL_STATE *state,
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

	result = opengl_surface_retain(state, command->target,
		command->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_surface_retain(state, payload->source,
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
	if (!opengl_clip_rect(destination, &payload->clip, &clip)) {
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
		result = opengl_check_error(state, "surface PSET image copy");
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
		result = opengl_check_error(state,
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
		result = opengl_check_error(state, "overlapping surface blit copy");
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
		opengl_color_mask(destination->depth));
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	result = opengl_check_error(state, "surface blit compute dispatch");

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
static int opengl_transform_blit(FB_GFX3_OPENGL_STATE *state,
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
	result = opengl_surface_retain(state, command->target, command->sequence,
		&destination);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_surface_retain(state, payload->source, command->sequence,
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
	if (!opengl_clip_rect(destination, &payload->clip, &clip)) {
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
		result = opengl_check_error(state,
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
		result = opengl_check_error(state, "self-transform surface copy");
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
		opengl_color_mask(destination->depth));
	state->gl.uniform_1ui(state->transform_blit_filter_location,
		payload->filter);
	state->gl.uniform_1ui(state->transform_blit_wrap_location, payload->wrap);
	state->gl.dispatch_compute(groups_x, groups_y, 1);
	state->gl.memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT);
	result = opengl_check_error(state, "transform blit compute dispatch");

done:
	if (temporary_texture != 0)
		state->gl.delete_textures(1, &temporary_texture);
	if (source != NULL)
		fb_gfx3_resource_release(state->resources, payload->source);
	if (destination != NULL)
		fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int opengl_read_pixel(FB_GFX3_OPENGL_STATE *state,
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
	result = opengl_surface_retain(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if ((payload->x >= 0) && (payload->y >= 0) &&
	    (payload->x < (int32_t)surface->width) &&
	    (payload->y < (int32_t)surface->height)) {
		state->gl.memory_barrier(GL_FRAMEBUFFER_BARRIER_BIT |
			GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		result = opengl_bind_color_framebuffer(state, surface->texture);
		if (result != FB_GFX3_OK)
			goto done;
		state->gl.read_buffer(GL_COLOR_ATTACHMENT0);
		state->gl.read_pixels(payload->x, payload->y, 1, 1,
			GL_RED_INTEGER, GL_UNSIGNED_INT, &color);
		result = opengl_check_error(state, "single-pixel readback");
		if (result != FB_GFX3_OK)
			goto done;
	}
	result = fb_gfx3_completion_set_value(command->completion, 0, color);

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

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
	result = opengl_load_functions(state);
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

	result = opengl_create_programs(state);
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
	result = opengl_check_error(state, "backend object allocation");
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
	result = opengl_surface_retain(state, command->target,
		command->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_surface_retain(state, description.source,
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
	result = opengl_check_error(state, "full page image copy");
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
	result = opengl_surface_retain(state, command->target,
		command->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = opengl_surface_retain(state, description.source,
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
		    (command->type == FB_GFX3_COMMAND_ELLIPSE)) {
			batch_count = opengl_primitive_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX :
				count - i), &primitive_count);
			if (batch_count > 1u) {
				result = opengl_primitive_batch(state, commands + i,
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
			batch_count = opengl_alpha_blit_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX :
				(count - i)));
			if (batch_count > 1) {
				result = opengl_alpha_blit_batch(state, commands + i, batch_count);
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
			batch_count = opengl_blit_batch_count(commands + i,
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
				result = opengl_blit_batch(state, commands + i, batch_count);
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
			batch_count = opengl_glyph_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX :
				(count - i)));
			for (batch_index = 1u; batch_index < batch_count;
			     ++batch_index) {
				if ((commands[i + batch_index] == NULL) ||
				    (commands[i + batch_index]->sequence <=
				     commands[i + batch_index - 1u]->sequence))
					return FB_GFX3_INVALID;
			}
			result = opengl_glyph_batch(state, commands + i, batch_count);
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
			batch_count = opengl_points_batch_count(commands + i,
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
				result = opengl_points_batch(state, commands + i, batch_count);
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
			batch_count = opengl_line_batch_count(commands + i,
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
				result = opengl_line_batch(state, commands + i, batch_count);
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
			batch_count = opengl_rectangle_batch_count(commands + i,
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
				result = opengl_rectangle_batch(state, commands + i,
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
				result = opengl_clear(state, commands[i + batch_count - 1]);
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
				result = opengl_paint(state,
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
				result = opengl_palette(state, commands[i + batch_count - 1u]);
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
			result = opengl_surface_create(state, command);
			break;
		case FB_GFX3_COMMAND_SURFACE_DESTROY:
			if (command->target == state->visible_surface) {
				state->visible_surface = 0;
				state->presentation_dirty = FALSE;
			}
			result = opengl_surface_release(state, command);
			break;
		case FB_GFX3_COMMAND_SURFACE_UPLOAD:
			result = opengl_surface_upload(state, command);
			break;
		case FB_GFX3_COMMAND_SURFACE_DOWNLOAD:
			result = opengl_surface_download(state, command);
			break;
		case FB_GFX3_COMMAND_CLEAR:
			result = opengl_clear(state, command);
			break;
		case FB_GFX3_COMMAND_POINTS:
			result = opengl_points(state, command);
			break;
		case FB_GFX3_COMMAND_LINE:
			result = opengl_line(state, command);
			break;
		case FB_GFX3_COMMAND_LINES:
			result = opengl_lines(state, command);
			break;
		case FB_GFX3_COMMAND_RECTANGLE:
			result = opengl_rectangle(state, command);
			break;
		case FB_GFX3_COMMAND_RECTANGLES:
			result = opengl_rectangles(state, command);
			break;
		case FB_GFX3_COMMAND_ELLIPSE:
			batch_count = opengl_ellipse_batch_count(commands + i,
				(uint32_t)(((count - i) > UINT32_MAX) ? UINT32_MAX :
				count - i));
			if (batch_count > 1u) {
				result = opengl_ellipse_batch(state, commands + i, batch_count);
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
			result = opengl_ellipse(state, command);
			break;
		case FB_GFX3_COMMAND_PAINT:
			result = opengl_paint(state, command);
			break;
		case FB_GFX3_COMMAND_BLIT:
			result = opengl_blit(state, command);
			break;
		case FB_GFX3_COMMAND_BLITS:
			result = opengl_blits(state, command);
			break;
		case FB_GFX3_COMMAND_TRANSFORM_BLIT:
			result = opengl_transform_blit(state, command);
			break;
		case FB_GFX3_COMMAND_READ_PIXEL:
			result = opengl_read_pixel(state, command);
			break;
		case FB_GFX3_COMMAND_PALETTE:
			result = opengl_palette(state, command);
			break;
		case FB_GFX3_COMMAND_PAGE_SET:
			result = opengl_page_set(state, command);
			break;
		case FB_GFX3_COMMAND_PRESENT:
			/*
				PAGESET and SCREENCOPY queue asynchronous presentation requests.
				When another command follows in this renderer drain, no BASIC
					caller can observe this intermediate front buffer before the
					batch reaches its ordered completion point.  Retain the final
					visible surface and let the normal end-of-batch presentation
					perform one swap. A synchronous PRESENT is a one-command batch
					and still calls opengl_present() immediately below.
			*/
			if ((i + 1u < count) &&
			    (command->target == state->visible_surface)) {
				state->presentation_dirty = TRUE;
				result = FB_GFX3_OK;
			} else
				result = opengl_present(state, command);
			break;
		case FB_GFX3_COMMAND_WINDOW_TITLE:
			result = opengl_window_title(state, command);
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
		result = opengl_present_handle(state, state->visible_surface,
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
	return opengl_check_error(state, "wait for idle");
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
