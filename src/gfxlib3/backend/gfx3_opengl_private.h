/*
 * FreeBASIC gfxlib3: gfx3_opengl_private.h
 * Purpose: Share renderer-private types and cross-unit helpers.
 * Responsibilities: backend state layout, checked helpers and render-thread ownership.
 * This file intentionally does NOT contain public ABI declarations or native window policy.
 */

#ifndef FB_GFX3_OPENGL_PRIVATE_H
#define FB_GFX3_OPENGL_PRIVATE_H

#include "../gfx3_backend_opengl.h"
#include "../gfx3_platform.h"
#include "../gfx3_protocol.h"
#include "../gfx3_resource.h"
#include "../gfx3_debug.h"

#if (defined(HOST_WIN32) || defined(HOST_LINUX)) && !defined(DISABLE_OPENGL)

/*
 * Only this backend's implementation units may include this header. The
 * renderer thread owns the device/context and all fields below. Producer
 * threads communicate through copied commands and completion barriers.
 * These layouts describe backend storage, not the public FreeBASIC ABI.
 */

#if defined(HOST_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glext.h>

#ifndef APIENTRY
#define APIENTRY
#endif

/* ------------------------------------------------------------------------- */
/* Shader limits and ordered batch keys                                     */
/* ------------------------------------------------------------------------- */

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

/* ------------------------------------------------------------------------- */
/* OpenGL entry points                                                      */
/* ------------------------------------------------------------------------- */

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

/* ------------------------------------------------------------------------- */
/* Renderer-owned resources and shader records                              */
/* ------------------------------------------------------------------------- */

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
/* Cross-unit operations                                                    */
/* ------------------------------------------------------------------------- */

/* Cross-unit helpers retain one definition. All other helpers stay static. */
int fb_gfx3_gl_alpha_blit_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count);
uint32_t fb_gfx3_gl_alpha_blit_batch_count(
	FB_GFX3_COMMAND *const *commands, uint32_t available);
int fb_gfx3_gl_bind_color_framebuffer(FB_GFX3_OPENGL_STATE *state,
	GLuint texture);
int fb_gfx3_gl_blit(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_blit_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count);
uint32_t fb_gfx3_gl_blit_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available);
int fb_gfx3_gl_blits(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
uint32_t fb_gfx3_gl_bytes_per_pixel(uint32_t depth, GLenum *type);
int fb_gfx3_gl_check_error(FB_GFX3_OPENGL_STATE *state,
	const char *operation);
int fb_gfx3_gl_clear(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_clip_rect(const FB_GFX3_OPENGL_SURFACE *surface,
	const FB_GFX3_RECT *requested, FB_GFX3_RECT *clipped);
uint32_t fb_gfx3_gl_color_mask(uint32_t depth);
int fb_gfx3_gl_create_programs(FB_GFX3_OPENGL_STATE *state);
int fb_gfx3_gl_dispatch_clear(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_OPENGL_SURFACE *surface, const FB_GFX3_RECT *rect,
	uint32_t color, uint32_t flags);
int fb_gfx3_gl_ellipse(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_ellipse_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count);
uint32_t fb_gfx3_gl_ellipse_batch_count(
	FB_GFX3_COMMAND *const *commands, uint32_t available);
int fb_gfx3_gl_glyph_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t command_count);
uint32_t fb_gfx3_gl_glyph_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available);
int fb_gfx3_gl_line(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_line_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count);
uint32_t fb_gfx3_gl_line_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available);
int fb_gfx3_gl_lines(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_load_functions(FB_GFX3_OPENGL_STATE *state);
int fb_gfx3_gl_page_set(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_paint(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_palette(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_points(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_points_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count);
uint32_t fb_gfx3_gl_points_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available);
int fb_gfx3_gl_present(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_present_handle(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_HANDLE handle, uint64_t sequence);
int fb_gfx3_gl_primitive_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t command_count,
	uint32_t primitive_count);
uint32_t fb_gfx3_gl_primitive_batch_count(
	FB_GFX3_COMMAND *const *commands, uint32_t available,
	uint32_t *primitive_count_result);
int fb_gfx3_gl_read_pixel(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_rectangle(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_rectangle_batch(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count);
uint32_t fb_gfx3_gl_rectangle_batch_count(
	FB_GFX3_COMMAND *const *commands, uint32_t available);
int fb_gfx3_gl_rectangles(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_surface_create(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_surface_download(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_surface_release(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_surface_retain(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_HANDLE handle, uint64_t sequence,
	FB_GFX3_OPENGL_SURFACE **surface);
int fb_gfx3_gl_surface_upload(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_transform_blit(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);
int fb_gfx3_gl_window_title(FB_GFX3_OPENGL_STATE *state,
	FB_GFX3_COMMAND *command);

#endif
#endif

/* end of gfx3_opengl_private.h */
