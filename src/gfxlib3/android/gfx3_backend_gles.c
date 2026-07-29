/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: android/gfx3_backend_gles.c

    Purpose:

        Execute gfxlib3 commands on OpenGL ES 3.0 devices which do not expose
        Vulkan or desktop OpenGL compute shaders.

    Responsibilities:

        - keep logical screen pages in GPU-resident integer textures
        - rasterize points, lines, rectangles, and ellipses with fragment shaders
        - perform PUT and projective transform operations through GPU textures
        - transfer pixels to CPU memory only for explicit synchronization
        - present indexed, RGB565, and 32-bit pages through an EGL surface
        - track asynchronous completion with GLES fence objects

    This file intentionally does NOT contain:

        - NativeActivity callbacks or EGL window lifecycle
		- FreeBASIC coordinate and QB compatibility behavior
		- CPU reference rasterizers
*/

#include "../gfx3_backend_gles.h"

#include "../gfx3_debug.h"
#include "../gfx3_platform.h"
#include "../gfx3_protocol.h"
#include "../gfx3_resource.h"

#if defined(HOST_ANDROID) && !defined(DISABLE_OPENGL)

#include <GLES3/gl3.h>

typedef struct FB_GFX3_GLES_FENCE {
	GLsync sync;
	uint64_t sequence;
	struct FB_GFX3_GLES_FENCE *next;
} FB_GFX3_GLES_FENCE;

struct FB_GFX3_GLES_STATE;

typedef struct FB_GFX3_GLES_SURFACE {
	struct FB_GFX3_GLES_STATE *state;
	GLuint texture;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
} FB_GFX3_GLES_SURFACE;

typedef struct FB_GFX3_GLES_STATE {
	const FB_GFX3_PLATFORM_VTABLE *platform_vtable;
	void *platform;
	FB_GFX3_RESOURCE_REGISTRY *resources;
	FB_GFX3_LOGGER *logger;
	GLuint framebuffer;
	GLuint read_framebuffer;
	GLuint draw_framebuffer;
	GLuint vertex_array;
	GLuint vertex_buffer;
	GLuint primitive_program;
	GLuint paint_program;
	GLuint blit_program;
	GLuint transform_blit_program;
	GLuint transform_blit_batch_program;
	GLuint blit_batch_program;
	GLuint blit_batch_trans_program[3];
	GLuint ellipse_span_batch_program;
	GLuint rectangle_batch_program;
	GLuint line_batch_program;
	GLuint present_program;
	GLuint readback_program;
	GLuint readback_texture;
	uint32_t readback_width;
	uint32_t readback_height;
	GLint readback_source_location;
	GLint readback_origin_location;
	GLint blit_source_image_location;
	GLint blit_source_origin_location;
	GLint blit_destination_image_location;
	GLint blit_destination_location;
	GLint blit_size_location;
	GLint blit_mode_location;
	GLint blit_alpha_location;
	GLint blit_depth_location;
	GLint blit_mask_location;
	GLint transform_blit_source_image_location;
	GLint transform_blit_destination_image_location;
	GLint transform_blit_source_rect_location;
	GLint transform_blit_bounds_location;
	GLint transform_blit_inverse_location;
	GLint transform_blit_mode_location;
	GLint transform_blit_alpha_location;
	GLint transform_blit_depth_location;
	GLint transform_blit_mask_location;
	GLint transform_blit_filter_location;
	GLint transform_blit_wrap_location;
	GLint transform_blit_batch_source_location;
	GLint transform_blit_batch_size_location;
	/*
		A stable sprite batch is issued once for many PUTs.  These locations
		belong to its linked program and never change during the renderer's
		lifetime, so resolving them at every frame only adds driver work on
		older mobile GPUs.
	*/
	GLint blit_batch_source_location;
	GLint blit_batch_size_location;
	GLint blit_batch_mode_location;
	GLint blit_batch_depth_location;
	GLint blit_batch_mask_location;
	GLint blit_batch_trans_source_location[3];
	GLint blit_batch_trans_size_location[3];
	GLint ellipse_span_batch_size_location;
	GLint rectangle_batch_size_location;
	GLint line_batch_size_location;
	GLuint palette_texture;
	uint32_t palette[256];
	FB_GFX3_HANDLE visible_surface;
	uint32_t maximum_texture_size;
	uint64_t submitted_sequence;
	uint64_t completed_sequence;
	FB_GFX3_ANDROID_KEYBOARD_OVERLAY keyboard_overlay;
	int keyboard_overlay_state;
	int keyboard_overlay_known;
	int presentation_dirty;
	int synchronous_fence_fallback;
	FB_GFX3_GLES_FENCE *first_fence;
	FB_GFX3_GLES_FENCE *last_fence;
} FB_GFX3_GLES_STATE;

/*
	An ES 3.0 fragment program cannot advance a writable frontier texture more
	than once in one draw.  Keep a batch short enough for older mobile drivers,
	but query the aggregate batch rather than forcing the CPU to wait after every
	pixel-distance expansion.  A connected maze still takes one expansion per
	edge, as required for exact four-neighbour PAINT semantics, but it never
	requires a target download or a CPU flood queue merely because it is large.
*/
#define FB_GFX3_GLES_PAINT_QUERY_BATCH 32u

/*
	A single screenful of small sprites must not allocate a source and
	destination snapshot for every PUT.  The ordered raster batch uses ordinary
	GPU primitive order, which is already the legacy last-writer rule for PSET
	and TRANS blits whose source differs from the destination.
*/
/*
	Four ordinary 1,024-sprite frames fit comfortably in the renderer thread's
	stack and one ES instanced draw.  A deeper packet removes three driver draws,
	vertex uploads, and state transitions from uninterrupted sprite streams.
*/
#define FB_GFX3_GLES_BLIT_BATCH_LIMIT 4096u

/*
	The graphical console emits opaque background and glyph pixels as POINTS.
	Keep the mobile batch bounded so a long PRINT stream does not require one
	transient vertex allocation as large as the complete command queue.
*/
#define FB_GFX3_GLES_POINTS_BATCH_LIMIT 256u

/*
	The exact midpoint path emits one initial span and no more than two spans
	for each horizontal or vertical midpoint step. Keep the common mobile
	CIRCLE range stack-only; larger shapes retain the existing bounded
	compatibility path.
*/
#define FB_GFX3_GLES_ELLIPSE_SPAN_BATCH_LIMIT 1025u

/*
	A public CIRCLE benchmark normally submits many small opaque filled circles.
	Each circle has at most 1,025 midpoint spans, so this bounded aggregate keeps
	the entire ordered run in one GPU draw without requiring unbounded storage.
*/
#define FB_GFX3_GLES_ELLIPSE_BATCH_LIMIT 64u
#define FB_GFX3_GLES_ELLIPSE_BATCH_SPAN_LIMIT \
	(FB_GFX3_GLES_ELLIPSE_SPAN_BATCH_LIMIT * FB_GFX3_GLES_ELLIPSE_BATCH_LIMIT)

/*
	Opaque LINE ... , BF commands need no destination read.  Keep their mobile
	batch bounded so the renderer can preserve submission order without making
	a single command drain allocate an unbounded transient vertex upload.
*/
#define FB_GFX3_GLES_RECTANGLE_BATCH_LIMIT 1024u

/*
	Line batches use the same bounded upload rule. The fragment shader performs
	the legacy Bresenham coverage test inside each line's own bounding quad.
*/
#define FB_GFX3_GLES_LINE_BATCH_LIMIT 1024u

/*
	A pathological public line can span the full signed coordinate range. Such
	a command remains on the bounded-quad fallback; do not turn it into an
	unbounded point draw merely because it is syntactically valid.
*/
#define FB_GFX3_GLES_LINE_BATCH_MAX_STEPS 4096u

typedef struct FB_GFX3_GLES_BLIT_BATCH_ITEM {
	int32_t source_x;
	int32_t source_y;
	int32_t width;
	int32_t height;
	int32_t destination_x;
	int32_t destination_y;
} FB_GFX3_GLES_BLIT_BATCH_ITEM;

typedef struct FB_GFX3_GLES_TRANSFORM_BATCH_ITEM {
	int32_t source_rect[4];
	int32_t bounds[4];
	float inverse_row_0[4];
	float inverse_row_1[4];
	float inverse_row_2[4];
	uint32_t options[4];
} FB_GFX3_GLES_TRANSFORM_BATCH_ITEM;

typedef struct FB_GFX3_GLES_ELLIPSE_SPAN_ITEM {
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
	uint32_t color;
	uint32_t reserved1;
	uint32_t reserved2;
	uint32_t reserved3;
} FB_GFX3_GLES_ELLIPSE_SPAN_ITEM;

typedef struct FB_GFX3_GLES_RECTANGLE_BATCH_ITEM {
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
	uint32_t color;
} FB_GFX3_GLES_RECTANGLE_BATCH_ITEM;

typedef struct FB_GFX3_GLES_LINE_BATCH_ITEM {
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
	uint32_t color;
	uint32_t style;
} FB_GFX3_GLES_LINE_BATCH_ITEM;

/* ------------------------------------------------------------------------- */
/* GLES shader programs                                                      */
/* ------------------------------------------------------------------------- */

static const char gles_vertex_shader[] =
	"#version 300 es\n"
	"layout(location = 1) in highp ivec2 point_position;\n"
	"layout(location = 2) in highp uint point_color;\n"
	"layout(location = 3) in highp uint point_flags;\n"
	"uniform int operation_points;\n"
	"uniform highp vec2 operation_surface_size;\n"
	"flat out highp uint raster_point_color;\n"
	"flat out highp uint raster_point_flags;\n"
	"void main(void)\n"
	"{\n"
	"    if (operation_points != 0) {\n"
	"        vec2 position = (vec2(point_position) + vec2(0.5)) /\n"
	"            operation_surface_size;\n"
	"        gl_Position = vec4((position * 2.0) - vec2(1.0), 0.0, 1.0);\n"
	"        gl_PointSize = 1.0; raster_point_color = point_color;\n"
	"        raster_point_flags = point_flags; return;\n"
	"    }\n"
	"    const vec2 position[3] = vec2[3](vec2(-1.0, -1.0),\n"
	"        vec2(3.0, -1.0), vec2(-1.0, 3.0));\n"
	"    raster_point_color = 0u; raster_point_flags = 0u;\n"
	"    gl_Position = vec4(position[gl_VertexID], 0.0, 1.0);\n"
	"}\n";

/*
	This is intentionally a separate vertex program from the fullscreen
	primitive program.  ES 3.0 vertex attributes have fixed types, so reusing
	the POINTS attribute layout for sprite rectangles would make either PSET or
	PUT depend on undefined attribute conversion.
*/
static const char gles_blit_batch_vertex_shader[] =
	"#version 300 es\n"
	"layout(location = 1) in highp ivec4 blit_source_rect;\n"
	"layout(location = 2) in highp ivec2 blit_destination;\n"
	"uniform highp vec2 operation_surface_size;\n"
	"flat out highp ivec2 raster_source_origin;\n"
	"flat out highp ivec2 raster_destination_origin;\n"
	"void main(void)\n"
	"{\n"
	"    const vec2 corner[6] = vec2[6](vec2(0.0, 0.0), vec2(1.0, 0.0),\n"
	"        vec2(0.0, 1.0), vec2(0.0, 1.0), vec2(1.0, 0.0), vec2(1.0, 1.0));\n"
	"    vec2 position = vec2(blit_destination) +\n"
	"        (corner[gl_VertexID] * vec2(blit_source_rect.zw));\n"
	"    gl_Position = vec4((position / operation_surface_size) * 2.0 - 1.0,\n"
	"        0.0, 1.0);\n"
	"    raster_source_origin = blit_source_rect.xy;\n"
	"    raster_destination_origin = blit_destination;\n"
	"}\n";

static const char gles_blit_batch_fragment_shader[] =
	"#version 300 es\n"
	"precision highp float;\n"
	"precision highp int;\n"
	"layout(location = 0) out highp uvec4 output_pixel;\n"
	"uniform highp usampler2D source_image;\n"
	"uniform uint operation_mode;\n"
	"uniform uint operation_depth;\n"
	"uniform uint operation_mask;\n"
	"flat in highp ivec2 raster_source_origin;\n"
	"flat in highp ivec2 raster_destination_origin;\n"
	"uint unpack_pixel(uvec4 value)\n"
	"{ return value.r | (value.g << 8) | (value.b << 16) | (value.a << 24); }\n"
	"uvec4 pack_pixel(uint value)\n"
	"{ return uvec4(value & 255u, (value >> 8) & 255u,\n"
	"    (value >> 16) & 255u, (value >> 24) & 255u); }\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 source_coordinate = raster_source_origin +\n"
	"        (ivec2(gl_FragCoord.xy) - raster_destination_origin);\n"
	"    uint source = unpack_pixel(texelFetch(source_image, source_coordinate, 0));\n"
	"    source &= operation_mask;\n"
	"    if (operation_mode == 0u) {\n"
	"        if (operation_depth <= 8u) { if (source == 0u) discard; }\n"
	"        else if (operation_depth == 16u) { if (source == 0xF81Fu) discard; }\n"
	"        else if ((source & 0x00FFFFFFu) == 0x00FF00FFu) discard;\n"
	"    }\n"
	"    else if (operation_mode == 2u) source = (~source) & operation_mask;\n"
	"    output_pixel = pack_pixel(source);\n"
	"}\n";

/*
	Transparent sprite batches dominate many 8-bit, RGB565, and 32-bit games.
	Their output is already stored as byte lanes in the RGBA8UI source texture.
	Dedicated programs avoid unpacking and repacking a uint, and remove the
	general PUT mode tree from every fragment on older mobile shader cores.
*/
static const char gles_blit_batch_trans8_fragment_shader[] =
	"#version 300 es\n"
	"precision highp int;\n"
	"layout(location = 0) out highp uvec4 output_pixel;\n"
	"uniform highp usampler2D source_image;\n"
	"flat in highp ivec2 raster_source_origin;\n"
	"flat in highp ivec2 raster_destination_origin;\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 source_coordinate = raster_source_origin +\n"
	"        (ivec2(gl_FragCoord.xy) - raster_destination_origin);\n"
	"    uint source = texelFetch(source_image, source_coordinate, 0).r;\n"
	"    if (source == 0u) discard;\n"
	"    output_pixel = uvec4(source, 0u, 0u, 0u);\n"
	"}\n";

static const char gles_blit_batch_trans16_fragment_shader[] =
	"#version 300 es\n"
	"precision highp int;\n"
	"layout(location = 0) out highp uvec4 output_pixel;\n"
	"uniform highp usampler2D source_image;\n"
	"flat in highp ivec2 raster_source_origin;\n"
	"flat in highp ivec2 raster_destination_origin;\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 source_coordinate = raster_source_origin +\n"
	"        (ivec2(gl_FragCoord.xy) - raster_destination_origin);\n"
	"    uvec4 source = texelFetch(source_image, source_coordinate, 0);\n"
	"    if ((source.r == 0x1Fu) && (source.g == 0xF8u)) discard;\n"
	"    output_pixel = uvec4(source.rg, 0u, 0u);\n"
	"}\n";

static const char gles_blit_batch_trans32_fragment_shader[] =
	"#version 300 es\n"
	"precision highp int;\n"
	"layout(location = 0) out highp uvec4 output_pixel;\n"
	"uniform highp usampler2D source_image;\n"
	"flat in highp ivec2 raster_source_origin;\n"
	"flat in highp ivec2 raster_destination_origin;\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 source_coordinate = raster_source_origin +\n"
	"        (ivec2(gl_FragCoord.xy) - raster_destination_origin);\n"
	"    uvec4 source = texelFetch(source_image, source_coordinate, 0);\n"
	"    if (all(equal(source.rgb, uvec3(0xFFu, 0u, 0xFFu)))) discard;\n"
	"    output_pixel = source;\n"
	"}\n";

/*
	A midpoint ellipse is a list of horizontal spans.  ES 3.0 lacks compute
	shaders, but its instanced raster path can still convert that exact list
	into pixels in one driver submission instead of one submission per span.
*/
static const char gles_ellipse_span_batch_vertex_shader[] =
	"#version 300 es\n"
	"layout(location = 1) in highp ivec4 span_box;\n"
	"layout(location = 2) in highp uint span_color;\n"
	"uniform highp vec2 operation_surface_size;\n"
	"flat out highp uint raster_color;\n"
	"void main(void)\n"
	"{\n"
	"    const vec2 corner[6] = vec2[6](vec2(0.0, 0.0), vec2(1.0, 0.0),\n"
	"        vec2(0.0, 1.0), vec2(0.0, 1.0), vec2(1.0, 0.0), vec2(1.0, 1.0));\n"
	"    vec2 size = vec2(span_box.z - span_box.x + 1,\n"
	"        span_box.w - span_box.y + 1);\n"
	"    vec2 position = vec2(span_box.xy) + (corner[gl_VertexID] * size);\n"
	"    gl_Position = vec4((position / operation_surface_size) * 2.0 - 1.0,\n"
	"        0.0, 1.0);\n"
	"    raster_color = span_color;\n"
	"}\n";

static const char gles_ellipse_span_batch_fragment_shader[] =
	"#version 300 es\n"
	"precision highp int;\n"
	"layout(location = 0) out highp uvec4 output_pixel;\n"
	"flat in highp uint raster_color;\n"
	"uvec4 pack_pixel(uint value)\n"
	"{ return uvec4(value & 255u, (value >> 8) & 255u,\n"
	"    (value >> 16) & 255u, (value >> 24) & 255u); }\n"
	"void main(void)\n"
	"{ output_pixel = pack_pixel(raster_color); }\n";

/*
	This is intentionally a separate shader from the exact primitive fallback.
	A filled opaque rectangle has no coverage test or destination dependency;
	instanced quads let the GPU process many public BOX BF commands in ordered
	draw order instead of launching one fullscreen shader draw per box.
*/
static const char gles_rectangle_batch_vertex_shader[] =
	"#version 300 es\n"
	"layout(location = 1) in highp ivec4 rectangle_box;\n"
	"layout(location = 2) in highp uint rectangle_color;\n"
	"uniform highp vec2 operation_surface_size;\n"
	"flat out highp uint raster_color;\n"
	"void main(void)\n"
	"{\n"
	"    const vec2 corner[6] = vec2[6](vec2(0.0, 0.0), vec2(1.0, 0.0),\n"
	"        vec2(0.0, 1.0), vec2(0.0, 1.0), vec2(1.0, 0.0), vec2(1.0, 1.0));\n"
	"    vec2 size = vec2(rectangle_box.z - rectangle_box.x + 1,\n"
	"        rectangle_box.w - rectangle_box.y + 1);\n"
	"    vec2 position = vec2(rectangle_box.xy) + (corner[gl_VertexID] * size);\n"
	"    gl_Position = vec4((position / operation_surface_size) * 2.0 - 1.0,\n"
	"        0.0, 1.0);\n"
	"    raster_color = rectangle_color;\n"
	"}\n";

static const char gles_rectangle_batch_fragment_shader[] =
	"#version 300 es\n"
	"precision highp int;\n"
	"layout(location = 0) out highp uvec4 output_pixel;\n"
	"flat in highp uint raster_color;\n"
	"uvec4 pack_pixel(uint value)\n"
	"{ return uvec4(value & 255u, (value >> 8) & 255u,\n"
	"    (value >> 16) & 255u, (value >> 24) & 255u); }\n"
	"void main(void)\n"
	"{ output_pixel = pack_pixel(raster_color); }\n";

/*
    The shared primitive shader already defines gfxlib3's exact integer line
    coverage. This batch variant generates that Bresenham sequence in the
    vertex stage and emits one GL point for each covered pixel. It is therefore
    an optimization of the same rule rather than an OpenGL line approximation.
*/
static const char gles_line_batch_vertex_shader[] =
	"#version 300 es\n"
	"layout(location = 1) in highp ivec4 line_endpoints;\n"
	"layout(location = 2) in highp uint line_color;\n"
	"layout(location = 3) in highp uint line_style;\n"
	"uniform highp vec2 operation_surface_size;\n"
	"flat out highp uint raster_color;\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 start = line_endpoints.xy; ivec2 finish = line_endpoints.zw;\n"
	"    ivec2 difference = abs(finish - start);\n"
	"    ivec2 direction = ivec2((finish.x < start.x) ? -1 : 1,\n"
	"        (finish.y < start.y) ? -1 : 1);\n"
	"    int index = gl_VertexID; int maximum = max(difference.x, difference.y);\n"
	"    ivec2 position;\n"
	"    if ((index > maximum) ||\n"
	"        ((line_style & (0x8000u >> (uint(index) & 15u))) == 0u)) {\n"
	"        gl_Position = vec4(2.0, 2.0, 0.0, 1.0); return;\n"
	"    }\n"
	"    if (difference.x >= difference.y) {\n"
	"        position.x = start.x + direction.x * index; position.y = start.y;\n"
	"        if (difference.x != 0) position.y += direction.y *\n"
	"            ((difference.y * index + (difference.x / 2)) / difference.x);\n"
	"    } else {\n"
	"        position.y = start.y + direction.y * index; position.x = start.x;\n"
	"        if (difference.y != 0) position.x += direction.x *\n"
	"            ((difference.x * index + (difference.y / 2)) / difference.y);\n"
	"    }\n"
	"    gl_Position = vec4(((vec2(position) + vec2(0.5)) /\n"
	"        operation_surface_size) * 2.0 - 1.0, 0.0, 1.0);\n"
	"    gl_PointSize = 1.0; raster_color = line_color;\n"
	"}\n";

static const char gles_line_batch_fragment_shader[] =
	"#version 300 es\n"
	"precision highp int;\n"
	"layout(location = 0) out highp uvec4 output_pixel;\n"
	"flat in highp uint raster_color;\n"
	"uvec4 pack_pixel(uint value)\n"
	"{ return uvec4(value & 255u, (value >> 8) & 255u,\n"
	"    (value >> 16) & 255u, (value >> 24) & 255u); }\n"
	"void main(void)\n"
	"{ output_pixel = pack_pixel(raster_color); }\n";

static const char gles_primitive_fragment_shader[] =
	"#version 300 es\n"
	"precision highp float;\n"
	"precision highp int;\n"
	"layout(location = 0) out highp uvec4 output_pixel;\n"
	"flat in highp uint raster_point_color;\n"
	"flat in highp uint raster_point_flags;\n"
	"uniform highp usampler2D destination_snapshot;\n"
	"uniform int operation_type;\n"
	"uniform ivec4 operation_clip;\n"
	"uniform ivec4 operation_data;\n"
	"uniform ivec2 operation_snapshot_origin;\n"
	"uniform vec2 operation_radii;\n"
	"uniform uint operation_color;\n"
	"uniform uint operation_style;\n"
	"uniform uint operation_mask;\n"
	"uniform uint operation_filled;\n"
	"uniform uint operation_flags;\n"
	"uniform int operation_points_alpha;\n"
	"uint unpack_pixel(uvec4 value)\n"
	"{ return value.r | (value.g << 8) | (value.b << 16) |\n"
	"    (value.a << 24); }\n"
	"uvec4 pack_pixel(uint value)\n"
	"{\n"
	"    return uvec4(value & 255u, (value >> 8) & 255u,\n"
	"        (value >> 16) & 255u, (value >> 24) & 255u);\n"
	"}\n"
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
	"bool line_pixel(ivec2 pixel)\n"
	"{\n"
	"    ivec2 start = operation_data.xy;\n"
	"    ivec2 finish = operation_data.zw;\n"
	"    ivec2 difference = abs(finish - start);\n"
	"    ivec2 direction = ivec2((finish.x < start.x) ? -1 : 1,\n"
	"        (finish.y < start.y) ? -1 : 1);\n"
	"    int index; ivec2 expected;\n"
	"    if (difference.x >= difference.y) {\n"
	"        index = (pixel.x - start.x) * direction.x;\n"
	"        if ((index < 0) || (index > difference.x)) return false;\n"
	"        expected.x = pixel.x; expected.y = start.y;\n"
	"        if (difference.x != 0) expected.y += direction.y *\n"
	"            ((difference.y * index + (difference.x / 2)) / difference.x);\n"
	"    } else {\n"
	"        index = (pixel.y - start.y) * direction.y;\n"
	"        if ((index < 0) || (index > difference.y)) return false;\n"
	"        expected.y = pixel.y; expected.x = start.x + direction.x *\n"
	"            ((difference.x * index + (difference.y / 2)) / difference.y);\n"
	"    }\n"
	"    return all(equal(pixel, expected)) &&\n"
	"        ((operation_style & (0x8000u >> (uint(index) & 15u))) != 0u);\n"
	"}\n"
	"bool rectangle_pixel(ivec2 pixel)\n"
	"{\n"
	"    int width = operation_data.z - operation_data.x + 1;\n"
	"    int height = operation_data.w - operation_data.y + 1;\n"
	"    int index = -1;\n"
	"    if (operation_filled != 0u)\n"
	"        return (pixel.x >= operation_data.x) &&\n"
	"            (pixel.x <= operation_data.z) &&\n"
	"            (pixel.y >= operation_data.y) && (pixel.y <= operation_data.w);\n"
	"    if ((pixel.y == operation_data.w) && (pixel.x >= operation_data.x) &&\n"
	"        (pixel.x <= operation_data.z)) index = pixel.x - operation_data.x;\n"
	"    if ((pixel.y == operation_data.y) && (pixel.x >= operation_data.x) &&\n"
	"        (pixel.x <= operation_data.z)) index = width + pixel.x - operation_data.x;\n"
	"    if ((pixel.x == operation_data.z) && (pixel.y >= operation_data.y) &&\n"
	"        (pixel.y <= operation_data.w)) index = (width * 2) + pixel.y - operation_data.y;\n"
	"    if ((pixel.x == operation_data.x) && (pixel.y >= operation_data.y) &&\n"
	"        (pixel.y <= operation_data.w)) index = (width * 2) + height +\n"
	"            pixel.y - operation_data.y;\n"
	"    return (index >= 0) &&\n"
	"        ((operation_style & (0x8000u >> (uint(index) & 15u))) != 0u);\n"
	"}\n"
	"bool ellipse_pixel(ivec2 pixel)\n"
	"{\n"
	"    vec2 radii = max(operation_radii, vec2(0.0));\n"
	"    vec2 delta = abs(vec2(pixel - operation_data.xy));\n"
	"    if (radii.y == 0.0)\n"
	"        return (pixel.y == operation_data.y) && (delta.x <= radii.x);\n"
	"    if (radii.x == 0.0)\n"
	"        return (pixel.x == operation_data.x) && (delta.y <= radii.y);\n"
	"    float distance_squared = dot(delta / radii, delta / radii);\n"
	"    if (operation_filled != 0u) return distance_squared <= 1.0;\n"
	"    vec2 inner_radii = max(radii - vec2(1.0), vec2(0.001));\n"
	"    float inner_squared = dot(delta / inner_radii, delta / inner_radii);\n"
	"    return (distance_squared <= 1.08) && (inner_squared >= 1.0);\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 pixel = ivec2(gl_FragCoord.xy);\n"
	"    if ((pixel.x < operation_clip.x) || (pixel.y < operation_clip.y) ||\n"
	"        (pixel.x > operation_clip.z) || (pixel.y > operation_clip.w)) discard;\n"
	"    bool covered = (operation_type == 0) || (operation_type == 4);\n"
	"    if (operation_type == 1) covered = line_pixel(pixel);\n"
	"    else if (operation_type == 2) covered = rectangle_pixel(pixel);\n"
	"    else if (operation_type == 3) covered = ellipse_pixel(pixel);\n"
	"    if (!covered) discard;\n"
	"    uint result_color = (operation_type == 4) ? raster_point_color :\n"
	"        operation_color; result_color &= operation_mask;\n"
	"    uint flags = (operation_type == 4) ? raster_point_flags :\n"
	"        operation_flags;\n"
	"    if (((flags & 1u) != 0u) &&\n"
	"        ((operation_type != 4) || (operation_points_alpha != 0))) {\n"
	"        uint destination = unpack_pixel(texelFetch(destination_snapshot,\n"
	"            pixel - operation_snapshot_origin, 0));\n"
	"        result_color = alpha_primitive(result_color, destination);\n"
	"    }\n"
	"    output_pixel = pack_pixel(result_color);\n"
	"}\n";

/*
	ES 3.0 cannot use the desktop storage-image FIFO. This shader therefore
	pings a one-bit frontier mask between textures. The host bounds the number
	of passes to the full pixel count, which is exact for every connected region
	inside the intentionally small GLES acceleration limit.
*/
static const char gles_paint_fragment_shader[] =
	"#version 300 es\n"
	"precision highp float;\n"
	"precision highp int;\n"
	"layout(location = 0) out highp uvec4 output_pixel;\n"
	"uniform highp usampler2D surface_image;\n"
	"uniform highp usampler2D mask_image;\n"
	"uniform int operation_mode;\n"
	"uniform ivec2 operation_seed;\n"
	"uniform ivec4 operation_clip;\n"
	"uniform uint operation_color;\n"
	"uniform uint operation_border;\n"
	"uniform uint operation_mask;\n"
	"uniform uint operation_flags;\n"
	"uniform uint operation_paint_mode;\n"
	"uniform uvec2 operation_pattern_origin;\n"
	"uniform highp usampler2D pattern_image;\n"
	"uint unpack_pixel(uvec4 value)\n"
	"{ return value.r | (value.g << 8) | (value.b << 16) |\n"
	"    (value.a << 24); }\n"
	"uvec4 pack_pixel(uint value)\n"
	"{ return uvec4(value & 255u, (value >> 8) & 255u,\n"
	"    (value >> 16) & 255u, (value >> 24) & 255u); }\n"
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
	"bool in_clip(ivec2 pixel)\n"
	"{ return (pixel.x >= operation_clip.x) && (pixel.y >= operation_clip.y) &&\n"
	"    (pixel.x <= operation_clip.z) && (pixel.y <= operation_clip.w); }\n"
	"uint mask_at(ivec2 pixel)\n"
	"{ return unpack_pixel(texelFetch(mask_image, pixel, 0)) & 1u; }\n"
	"uint pattern_color(ivec2 pixel)\n"
	"{\n"
	"    uint x = (uint(pixel.x) + operation_pattern_origin.x) & 7u;\n"
	"    uint y = (uint(pixel.y) + operation_pattern_origin.y) & 7u;\n"
	"    uvec4 value = texelFetch(pattern_image, ivec2(x, y), 0);\n"
	"    return (value.r | (value.g << 8) | (value.b << 16) |\n"
	"        (value.a << 24)) & operation_mask;\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 pixel = ivec2(gl_FragCoord.xy);\n"
	"    if (operation_mode == 0) {\n"
	"        uint seed_active = all(equal(pixel, operation_seed)) &&\n"
	"            in_clip(pixel) && (unpack_pixel(texelFetch(surface_image,\n"
	"            pixel, 0)) != operation_border) ? 1u : 0u;\n"
	"        output_pixel = pack_pixel(seed_active);\n"
	"        return;\n"
	"    }\n"
	"    if (operation_mode == 1) {\n"
	"        uint active = 0u;\n"
	"        if (in_clip(pixel) &&\n"
	"            (unpack_pixel(texelFetch(surface_image, pixel, 0)) != operation_border)) {\n"
	"            active = mask_at(pixel);\n"
	"            if ((active == 0u) && in_clip(pixel + ivec2(-1, 0))) active = mask_at(pixel + ivec2(-1, 0));\n"
	"            if ((active == 0u) && in_clip(pixel + ivec2(1, 0))) active = mask_at(pixel + ivec2(1, 0));\n"
	"            if ((active == 0u) && in_clip(pixel + ivec2(0, -1))) active = mask_at(pixel + ivec2(0, -1));\n"
	"            if ((active == 0u) && in_clip(pixel + ivec2(0, 1))) active = mask_at(pixel + ivec2(0, 1));\n"
	"        }\n"
	"        output_pixel = pack_pixel(active);\n"
	"        return;\n"
	"    }\n"
	"    if (operation_mode == 3) {\n"
	"        if ((mask_at(pixel) == 0u) ||\n"
	"            (unpack_pixel(texelFetch(surface_image, pixel, 0)) != 0u)) discard;\n"
	"        output_pixel = pack_pixel(0u);\n"
	"        return;\n"
	"    }\n"
	"    uint destination = unpack_pixel(texelFetch(surface_image, pixel, 0));\n"
	"    if (mask_at(pixel) != 0u) {\n"
	"        uint color;\n"
	"        if (operation_paint_mode != 0u) color = pattern_color(pixel);\n"
	"        else {\n"
	"            color = operation_color & operation_mask;\n"
	"            if ((operation_flags & 1u) != 0u) color = alpha_primitive(color, destination);\n"
	"        }\n"
	"        destination = color;\n"
	"    }\n"
	"    output_pixel = pack_pixel(destination);\n"
	"}\n";

static const char gles_blit_fragment_shader[] =
	"#version 300 es\n"
	"precision highp float;\n"
	"precision highp int;\n"
	"layout(location = 0) out highp uvec4 output_pixel;\n"
	"uniform highp usampler2D source_image;\n"
	"uniform highp usampler2D destination_image;\n"
	"uniform highp ivec2 operation_source_origin;\n"
	"uniform ivec2 operation_destination;\n"
	"uniform ivec2 operation_size;\n"
	"uniform uint operation_mode;\n"
	"uniform uint operation_alpha;\n"
	"uniform uint operation_depth;\n"
	"uniform uint operation_mask;\n"
	"uint unpack_pixel(uvec4 value)\n"
	"{ return value.r | (value.g << 8) | (value.b << 16) | (value.a << 24); }\n"
	"uvec4 pack_pixel(uint value)\n"
	"{ return uvec4(value & 255u, (value >> 8) & 255u,\n"
	"    (value >> 16) & 255u, (value >> 24) & 255u); }\n"
	"bool blend_pixel(uint source, uint destination, out uint result_color)\n"
	"{\n"
	"    uint alpha; uint srb; uint sga; uint drb; uint dga;\n"
	"    uint temporary1; uint temporary2; uint overflow;\n"
	"    if (operation_mode == 1u) result_color = source & operation_mask;\n"
	"    else if (operation_mode == 2u) result_color = (~source) & operation_mask;\n"
	"    else if (operation_mode == 3u) result_color = (source & destination) & operation_mask;\n"
	"    else if (operation_mode == 4u) result_color = (source | destination) & operation_mask;\n"
	"    else if (operation_mode == 5u) result_color = (source ^ destination) & operation_mask;\n"
	"    else if (operation_mode == 0u) {\n"
	"        result_color = source & operation_mask;\n"
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
	"            source += destination; overflow = source & 0x08010020u;\n"
	"            overflow -= overflow >> 5; source |= overflow; source &= 0x07C0F81Fu;\n"
	"            result_color = (source | (source >> 16)) & 0xFFFFu;\n"
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
	"    } else return false;\n"
	"    return true;\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 pixel = ivec2(gl_FragCoord.xy);\n"
	"    ivec2 offset = pixel - operation_destination;\n"
	"    if ((offset.x < 0) || (offset.y < 0) || (offset.x >= operation_size.x) ||\n"
	"        (offset.y >= operation_size.y)) discard;\n"
	"    uint source = unpack_pixel(texelFetch(source_image,\n"
	"        operation_source_origin + offset, 0));\n"
	"    uint destination = unpack_pixel(texelFetch(destination_image, offset, 0));\n"
	"    uint result_color; if (!blend_pixel(source, destination, result_color)) discard;\n"
	"    output_pixel = pack_pixel(result_color & operation_mask);\n"
	"}\n";

/*
	ES 3.0 has no compute shaders, but inverse mapping is still pure GPU work.
	A scissored fullscreen draw launches one fragment per destination pixel and
	the fragment shader performs all scaling, rotation, projection, sampling,
	and PUT blending. The CPU submits one matrix instead of rasterizing spans.
*/
static const char gles_transform_blit_fragment_shader[] =
	"#version 300 es\n"
	"precision highp float;\n"
	"precision highp int;\n"
	"layout(location = 0) out highp uvec4 output_pixel;\n"
	"uniform highp usampler2D source_image;\n"
	"uniform highp usampler2D destination_image;\n"
	"uniform highp ivec4 operation_source_rect;\n"
	"uniform highp ivec4 operation_bounds;\n"
	"uniform highp mat3 operation_inverse;\n"
	"uniform highp uint operation_mode;\n"
	"uniform highp uint operation_alpha;\n"
	"uniform highp uint operation_depth;\n"
	"uniform highp uint operation_mask;\n"
	"uniform highp uint operation_filter;\n"
	"uniform highp uint operation_wrap;\n"
	"uint unpack_pixel(uvec4 value)\n"
	"{ return value.r | (value.g << 8) | (value.b << 16) | (value.a << 24); }\n"
	"uvec4 pack_pixel(uint value)\n"
	"{ return uvec4(value & 255u, (value >> 8) & 255u,\n"
	"    (value >> 16) & 255u, (value >> 24) & 255u); }\n"
	"ivec2 source_coordinate(ivec2 p)\n"
	"{\n"
	"    ivec2 origin = operation_source_rect.xy;\n"
	"    ivec2 size = operation_source_rect.zw - origin + ivec2(1);\n"
	"    if (operation_wrap != 0u) { ivec2 relative = (p - origin) % size;\n"
	"        relative = (relative + size) % size; return origin + relative; }\n"
	"    return clamp(p, origin, operation_source_rect.zw);\n"
	"}\n"
	"uint source_pixel(ivec2 p)\n"
	"{ return unpack_pixel(texelFetch(source_image, source_coordinate(p), 0)); }\n"
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
	"            source += destination; overflow = source & 0x08010020u;\n"
	"            overflow -= overflow >> 5; source |= overflow; source &= 0x07C0F81Fu;\n"
	"            result_color = (source | (source >> 16)) & 0xFFFFu;\n"
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
	"    ivec2 pixel = ivec2(gl_FragCoord.xy);\n"
	"    if ((pixel.x < operation_bounds.x) || (pixel.y < operation_bounds.y) ||\n"
	"        (pixel.x > operation_bounds.z) || (pixel.y > operation_bounds.w)) discard;\n"
	"    vec3 mapped = operation_inverse * vec3(vec2(pixel) + vec2(0.5), 1.0);\n"
	"    if (mapped.z <= 0.000001) discard; vec2 source_coordinate_value = mapped.xy / mapped.z;\n"
	"    if ((operation_wrap == 0u) && ((source_coordinate_value.x < float(operation_source_rect.x)) ||\n"
	"        (source_coordinate_value.y < float(operation_source_rect.y)) ||\n"
	"        (source_coordinate_value.x >= float(operation_source_rect.z + 1)) ||\n"
	"        (source_coordinate_value.y >= float(operation_source_rect.w + 1)))) discard;\n"
	"    uint source = sample_source(source_coordinate_value);\n"
	"    uint destination = 0u;\n"
	"    if (operation_mode > 2u) destination = unpack_pixel(texelFetch(\n"
	"        destination_image, pixel - operation_bounds.xy, 0));\n"
	"    uint result_color;\n"
	"    if (!blend_pixel(source, destination, result_color)) discard;\n"
	"    output_pixel = pack_pixel(result_color & operation_mask);\n"
	"}\n";

/*
	Destination-independent transforms retain ordinary raster ordering when
	drawn as instances. Each instance carries one inverse matrix, so a complete
	sprite or Mode 7 run reaches the mobile GPU in one driver submission.
*/
static const char gles_transform_blit_batch_vertex_shader[] =
	"#version 300 es\n"
	"layout(location = 1) in highp ivec4 transform_source_rect;\n"
	"layout(location = 2) in highp ivec4 transform_bounds;\n"
	"layout(location = 3) in highp vec3 transform_inverse_row_0;\n"
	"layout(location = 4) in highp vec3 transform_inverse_row_1;\n"
	"layout(location = 5) in highp vec3 transform_inverse_row_2;\n"
	"layout(location = 6) in highp uvec4 transform_options;\n"
	"uniform highp vec2 operation_surface_size;\n"
	"flat out highp ivec4 raster_source_rect;\n"
	"flat out highp vec3 raster_inverse_row_0;\n"
	"flat out highp vec3 raster_inverse_row_1;\n"
	"flat out highp vec3 raster_inverse_row_2;\n"
	"flat out highp uvec4 raster_options;\n"
	"void main(void)\n"
	"{\n"
	"    const vec2 corner[6] = vec2[6](vec2(0.0, 0.0), vec2(1.0, 0.0),\n"
	"        vec2(0.0, 1.0), vec2(0.0, 1.0), vec2(1.0, 0.0), vec2(1.0, 1.0));\n"
	"    vec2 size = vec2(transform_bounds.zw - transform_bounds.xy + ivec2(1));\n"
	"    vec2 position = vec2(transform_bounds.xy) + corner[gl_VertexID] * size;\n"
	"    gl_Position = vec4((position / operation_surface_size) * 2.0 - 1.0,\n"
	"        0.0, 1.0);\n"
	"    raster_source_rect = transform_source_rect;\n"
	"    raster_inverse_row_0 = transform_inverse_row_0;\n"
	"    raster_inverse_row_1 = transform_inverse_row_1;\n"
	"    raster_inverse_row_2 = transform_inverse_row_2;\n"
	"    raster_options = transform_options;\n"
	"}\n";

static const char gles_transform_blit_batch_fragment_shader[] =
	"#version 300 es\n"
	"precision highp float;\n"
	"precision highp int;\n"
	"layout(location = 0) out highp uvec4 output_pixel;\n"
	"uniform highp usampler2D source_image;\n"
	"flat in highp ivec4 raster_source_rect;\n"
	"flat in highp vec3 raster_inverse_row_0;\n"
	"flat in highp vec3 raster_inverse_row_1;\n"
	"flat in highp vec3 raster_inverse_row_2;\n"
	"flat in highp uvec4 raster_options;\n"
	"uint unpack_pixel(uvec4 value)\n"
	"{ return value.r | (value.g << 8) | (value.b << 16) | (value.a << 24); }\n"
	"uvec4 pack_pixel(uint value)\n"
	"{ return uvec4(value & 255u, (value >> 8) & 255u,\n"
	"    (value >> 16) & 255u, (value >> 24) & 255u); }\n"
	"ivec2 source_coordinate(ivec2 p)\n"
	"{\n"
	"    ivec2 origin = raster_source_rect.xy;\n"
	"    ivec2 size = raster_source_rect.zw - origin + ivec2(1);\n"
	"    if ((raster_options.w >> 16) != 0u) {\n"
	"        ivec2 relative = (p - origin) % size;\n"
	"        relative = (relative + size) % size; return origin + relative;\n"
	"    }\n"
	"    return clamp(p, origin, raster_source_rect.zw);\n"
	"}\n"
	"uint source_pixel(ivec2 p)\n"
	"{ return unpack_pixel(texelFetch(source_image, source_coordinate(p), 0)); }\n"
	"uvec4 unpack_color(uint color)\n"
	"{\n"
	"    if (raster_options.y == 16u) return uvec4(\n"
	"        ((color >> 11) & 31u) * 255u / 31u,\n"
	"        ((color >> 5) & 63u) * 255u / 63u,\n"
	"        (color & 31u) * 255u / 31u, 255u);\n"
	"    return uvec4((color >> 16) & 255u, (color >> 8) & 255u,\n"
	"        color & 255u, (color >> 24) & 255u);\n"
	"}\n"
	"uint pack_color(uvec4 color)\n"
	"{\n"
	"    if (raster_options.y == 16u) return ((color.r * 31u / 255u) << 11) |\n"
	"        ((color.g * 63u / 255u) << 5) | (color.b * 31u / 255u);\n"
	"    return (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;\n"
	"}\n"
	"uint sample_source(vec2 coordinate)\n"
	"{\n"
	"    if (((raster_options.w & 65535u) == 0u) ||\n"
	"        (raster_options.y <= 8u))\n"
	"        return source_pixel(ivec2(floor(coordinate)));\n"
	"    vec2 texel = coordinate - vec2(0.5); ivec2 base = ivec2(floor(texel));\n"
	"    vec2 fraction = fract(texel);\n"
	"    vec4 top = mix(vec4(unpack_color(source_pixel(base))),\n"
	"        vec4(unpack_color(source_pixel(base + ivec2(1, 0)))), fraction.x);\n"
	"    vec4 bottom = mix(vec4(unpack_color(source_pixel(base + ivec2(0, 1)))),\n"
	"        vec4(unpack_color(source_pixel(base + ivec2(1, 1)))), fraction.x);\n"
	"    return pack_color(uvec4(round(mix(top, bottom, fraction.y))));\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    vec3 destination = vec3(gl_FragCoord.xy, 1.0);\n"
	"    vec3 mapped = vec3(dot(raster_inverse_row_0, destination),\n"
	"        dot(raster_inverse_row_1, destination),\n"
	"        dot(raster_inverse_row_2, destination));\n"
	"    if (mapped.z <= 0.000001) discard;\n"
	"    vec2 coordinate = mapped.xy / mapped.z;\n"
	"    if (((raster_options.w >> 16) == 0u) &&\n"
	"        ((coordinate.x < float(raster_source_rect.x)) ||\n"
	"         (coordinate.y < float(raster_source_rect.y)) ||\n"
	"         (coordinate.x >= float(raster_source_rect.z + 1)) ||\n"
	"         (coordinate.y >= float(raster_source_rect.w + 1)))) discard;\n"
	"    uint source = sample_source(coordinate) & raster_options.z;\n"
	"    if (raster_options.x == 0u) {\n"
	"        if (raster_options.y <= 8u) { if (source == 0u) discard; }\n"
	"        else if (raster_options.y == 16u) { if (source == 0xF81Fu) discard; }\n"
	"        else if ((source & 0x00FFFFFFu) == 0x00FF00FFu) discard;\n"
	"    } else if (raster_options.x == 2u) source = (~source) & raster_options.z;\n"
	"    output_pixel = pack_pixel(source);\n"
	"}\n";

static const char gles_present_fragment_shader[] =
	"#version 300 es\n"
	"precision highp float;\n"
	"precision highp int;\n"
	"layout(location = 0) out vec4 output_color;\n"
	"uniform highp usampler2D source_image;\n"
	"uniform sampler2D palette_image;\n"
	"uniform uint operation_depth;\n"
	"uniform ivec2 operation_window_size;\n"
	"uniform ivec4 keyboard_button_rect;\n"
	"uniform int keyboard_button_state;\n"
	"uint unpack_pixel(uvec4 value)\n"
	"{ return value.r | (value.g << 8) | (value.b << 16) | (value.a << 24); }\n"
	"bool keyboard_glyph(int glyph, int x, int y)\n"
	"{\n"
	"    if ((x < 0) || (x >= 5) || (y < 0) || (y >= 7)) return false;\n"
	"    if (glyph == 0) {\n"
	"        if (x == 0) return true;\n"
	"        if ((y == 0) || (y == 6)) return x == 4;\n"
	"        if ((y == 1) || (y == 5)) return x == 3;\n"
	"        if ((y == 2) || (y == 4)) return x == 2;\n"
	"        return (y == 3) && (x == 1);\n"
	"    }\n"
	"    if (x == 0) return true;\n"
	"    if ((y == 0) || (y == 3) || (y == 6)) return x < 4;\n"
	"    return ((y == 1) || (y == 2) || (y == 4) || (y == 5)) && (x == 4);\n"
	"}\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 output_size = ivec2(textureSize(source_image, 0));\n"
	"    ivec2 window_size = max(operation_window_size, ivec2(1));\n"
	"    vec2 normalized = gl_FragCoord.xy / vec2(max(window_size, ivec2(1)));\n"
	"    normalized.y = 1.0 - normalized.y;\n"
	"    ivec2 coordinate = clamp(ivec2(normalized * vec2(output_size)),\n"
	"        ivec2(0), output_size - ivec2(1));\n"
	"    uint pixel = unpack_pixel(texelFetch(source_image, coordinate, 0));\n"
	"    if (operation_depth <= 8u) {\n"
	"        uint mask = (1u << operation_depth) - 1u;\n"
	"        output_color = texelFetch(palette_image, ivec2(int(pixel & mask), 0), 0);\n"
	"    } else if (operation_depth == 16u) {\n"
	"        output_color = vec4(float((pixel >> 11) & 31u) / 31.0,\n"
	"            float((pixel >> 5) & 63u) / 63.0, float(pixel & 31u) / 31.0, 1.0);\n"
	"    } else {\n"
	"        output_color = vec4(float((pixel >> 16) & 255u),\n"
	"            float((pixel >> 8) & 255u), float(pixel & 255u), 255.0) / 255.0;\n"
	"    }\n"
	"    if (keyboard_button_state != 0) {\n"
	"        ivec2 native_pixel = ivec2(int(gl_FragCoord.x),\n"
	"            window_size.y - 1 - int(gl_FragCoord.y));\n"
	"        ivec2 local = native_pixel - keyboard_button_rect.xy;\n"
	"        ivec2 size = keyboard_button_rect.zw - keyboard_button_rect.xy;\n"
	"        if ((local.x >= 0) && (local.y >= 0) &&\n"
	"            (local.x < size.x) && (local.y < size.y)) {\n"
	"            vec4 fill = (keyboard_button_state == 3) ?\n"
	"                vec4(94.0, 128.0, 200.0, 255.0) / 255.0 :\n"
	"                ((keyboard_button_state == 2) ?\n"
	"                vec4(40.0, 120.0, 180.0, 255.0) / 255.0 :\n"
	"                vec4(54.0, 59.0, 68.0, 255.0) / 255.0);\n"
	"            if ((local.x < 2) || (local.y < 2) ||\n"
	"                (local.x >= size.x - 2) || (local.y >= size.y - 2))\n"
	"                output_color = vec4(16.0, 18.0, 24.0, 255.0) / 255.0;\n"
	"            else output_color = fill;\n"
	"            if (keyboard_glyph(0, local.x - 10, local.y - 16) ||\n"
	"                keyboard_glyph(1, local.x - 17, local.y - 16))\n"
	"                output_color = vec4(245.0, 246.0, 250.0, 255.0) / 255.0;\n"
	"        }\n"
	"    }\n"
	"}\n";

/*
	Some early GLES 3.0 drivers advertise integer framebuffer readback but
	return interleaved empty columns from glReadPixels(GL_RGBA_INTEGER).  A
	normalized RGBA8 staging target uses the universally supported color
	readback path while preserving every byte exactly.
*/
static const char gles_readback_fragment_shader[] =
	"#version 300 es\n"
	"precision highp float;\n"
	"precision highp int;\n"
	"layout(location = 0) out vec4 output_color;\n"
	"uniform highp usampler2D source_image;\n"
	"uniform highp ivec2 source_origin;\n"
	"void main(void)\n"
	"{\n"
	"    ivec2 coordinate = source_origin + ivec2(gl_FragCoord.xy);\n"
	"    output_color = vec4(texelFetch(source_image, coordinate, 0)) / 255.0;\n"
	"}\n";

static void gles_report_shader_log(FB_GFX3_GLES_STATE *state, GLuint object,
	int program, const char *description)
{
	char log[FB_GFX3_LOG_MESSAGE_SIZE];
	GLsizei length = 0;

	memset(log, 0, sizeof(log));
	if (program)
		glGetProgramInfoLog(object, (GLsizei)sizeof(log) - 1, &length, log);
	else
		glGetShaderInfoLog(object, (GLsizei)sizeof(log) - 1, &length, log);
	fb_gfx3_log_write(state->logger, FB_GFX3_LOG_ERROR, "%s: %s",
		description, (length > 0) ? log : "no GLES diagnostic");
}

static int gles_compile_shader(FB_GFX3_GLES_STATE *state, GLenum type,
	const char *source, const char *description, GLuint *shader)
{
	GLint compiled = GL_FALSE;

	*shader = glCreateShader(type);
	if (*shader == 0)
		return FB_GFX3_FAILED;
	glShaderSource(*shader, 1, &source, NULL);
	glCompileShader(*shader);
	glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_TRUE)
		return FB_GFX3_OK;
	gles_report_shader_log(state, *shader, FALSE, description);
	glDeleteShader(*shader);
	*shader = 0;
	return FB_GFX3_FAILED;
}

static int gles_create_program(FB_GFX3_GLES_STATE *state,
	const char *fragment_source, const char *description, GLuint *program)
{
	GLuint vertex_shader = 0;
	GLuint fragment_shader = 0;
	GLint linked = GL_FALSE;
	int result;

	result = gles_compile_shader(state, GL_VERTEX_SHADER, gles_vertex_shader,
		"GLES vertex shader", &vertex_shader);
	if (result != FB_GFX3_OK)
		return result;
	result = gles_compile_shader(state, GL_FRAGMENT_SHADER, fragment_source,
		description, &fragment_shader);
	if (result != FB_GFX3_OK) {
		glDeleteShader(vertex_shader);
		return result;
	}
	*program = glCreateProgram();
	if (*program != 0) {
		glAttachShader(*program, vertex_shader);
		glAttachShader(*program, fragment_shader);
		glLinkProgram(*program);
		glGetProgramiv(*program, GL_LINK_STATUS, &linked);
	}
	glDeleteShader(fragment_shader);
	glDeleteShader(vertex_shader);
	if (linked == GL_TRUE)
		return FB_GFX3_OK;
	if (*program != 0) {
		gles_report_shader_log(state, *program, TRUE, description);
		glDeleteProgram(*program);
		*program = 0;
	}
	return FB_GFX3_FAILED;
}

static int gles_create_program_with_vertex(FB_GFX3_GLES_STATE *state,
	const char *vertex_source, const char *fragment_source,
	const char *description, GLuint *program)
{
	GLuint vertex_shader = 0;
	GLuint fragment_shader = 0;
	GLint linked = GL_FALSE;
	int result;

	if ((vertex_source == NULL) || (fragment_source == NULL) ||
	    (program == NULL))
		return FB_GFX3_INVALID;
	result = gles_compile_shader(state, GL_VERTEX_SHADER, vertex_source,
		description, &vertex_shader);
	if (result != FB_GFX3_OK)
		return result;
	result = gles_compile_shader(state, GL_FRAGMENT_SHADER, fragment_source,
		description, &fragment_shader);
	if (result != FB_GFX3_OK) {
		glDeleteShader(vertex_shader);
		return result;
	}
	*program = glCreateProgram();
	if (*program != 0) {
		glAttachShader(*program, vertex_shader);
		glAttachShader(*program, fragment_shader);
		glLinkProgram(*program);
		glGetProgramiv(*program, GL_LINK_STATUS, &linked);
	}
	glDeleteShader(fragment_shader);
	glDeleteShader(vertex_shader);
	if (linked == GL_TRUE)
		return FB_GFX3_OK;
	if (*program != 0) {
		gles_report_shader_log(state, *program, TRUE, description);
		glDeleteProgram(*program);
		*program = 0;
	}
	return FB_GFX3_FAILED;
}

/* ------------------------------------------------------------------------- */
/* Surface and pixel helpers                                                 */
/* ------------------------------------------------------------------------- */

static uint32_t gles_color_mask(uint32_t depth)
{
	return (depth >= 32u) ? UINT32_MAX : ((1u << depth) - 1u);
}

static uint32_t gles_bytes_per_pixel(uint32_t depth)
{
	if (depth <= 8u)
		return 1;
	if (depth == 16u)
		return 2;
	return (depth == 32u) ? 4 : 0;
}

static uint32_t gles_decode_pixel(const unsigned char *source,
	uint32_t bytes_per_pixel)
{
	uint32_t value = source[0];

	if (bytes_per_pixel > 1u)
		value |= (uint32_t)source[1] << 8;
	if (bytes_per_pixel > 2u)
		value |= ((uint32_t)source[2] << 16) |
			((uint32_t)source[3] << 24);
	return value;
}

static void gles_encode_pixel(unsigned char *destination,
	uint32_t bytes_per_pixel, uint32_t value)
{
	destination[0] = (unsigned char)value;
	if (bytes_per_pixel > 1u)
		destination[1] = (unsigned char)(value >> 8);
	if (bytes_per_pixel > 2u) {
		destination[2] = (unsigned char)(value >> 16);
		destination[3] = (unsigned char)(value >> 24);
	}
}

static void gles_pack_texture_pixel(unsigned char *destination,
	uint32_t value)
{
	destination[0] = (unsigned char)value;
	destination[1] = (unsigned char)(value >> 8);
	destination[2] = (unsigned char)(value >> 16);
	destination[3] = (unsigned char)(value >> 24);
}

static uint32_t gles_unpack_texture_pixel(const unsigned char *source)
{
	return source[0] | ((uint32_t)source[1] << 8) |
		((uint32_t)source[2] << 16) | ((uint32_t)source[3] << 24);
}

static int gles_check_error(FB_GFX3_GLES_STATE *state,
	const char *operation)
{
	GLenum error = glGetError();

	if (error == GL_NO_ERROR)
		return FB_GFX3_OK;
	fb_gfx3_log_write(state->logger, FB_GFX3_LOG_ERROR,
		"OpenGL ES error 0x%04X during %s", (unsigned int)error,
		operation);
	while (glGetError() != GL_NO_ERROR) {
		/* Leave a clean error queue for the next checked operation. */
	}
	return FB_GFX3_FAILED;
}

static int gles_clip_rect(const FB_GFX3_GLES_SURFACE *surface,
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
	return (clipped->x1 <= clipped->x2) && (clipped->y1 <= clipped->y2);
}

static void gles_surface_destroy(void *resource)
{
	FB_GFX3_GLES_SURFACE *surface = (FB_GFX3_GLES_SURFACE *)resource;

	if (surface == NULL)
		return;
	if (surface->texture != 0)
		glDeleteTextures(1, &surface->texture);
	free(surface);
}

static int gles_surface_retain(FB_GFX3_GLES_STATE *state,
	FB_GFX3_HANDLE handle, uint64_t sequence, FB_GFX3_GLES_SURFACE **surface)
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

static int gles_attach_surface(FB_GFX3_GLES_STATE *state,
	FB_GFX3_GLES_SURFACE *surface, GLenum target)
{
	glBindFramebuffer(target, state->framebuffer);
	glFramebufferTexture2D(target, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		surface->texture, 0);
	if (glCheckFramebufferStatus(target) != GL_FRAMEBUFFER_COMPLETE)
		return FB_GFX3_UNSUPPORTED;
	return FB_GFX3_OK;
}

static int gles_copy_texture_region(FB_GFX3_GLES_STATE *state,
	FB_GFX3_GLES_SURFACE *source, int source_x, int source_y,
	uint32_t width, uint32_t height, GLuint *copy);

static int gles_draw_primitive(FB_GFX3_GLES_STATE *state,
	FB_GFX3_GLES_SURFACE *surface, const FB_GFX3_RECT *clip,
	const FB_GFX3_RECT *draw_rect, int operation_type, const int data[4],
	float radius_x, float radius_y, uint32_t color, uint32_t style, int filled,
	uint32_t flags)
{
	FB_GFX3_RECT area;
	GLuint snapshot = 0;
	int result;

	area = *draw_rect;
	if (area.x1 < clip->x1)
		area.x1 = clip->x1;
	if (area.y1 < clip->y1)
		area.y1 = clip->y1;
	if (area.x2 > clip->x2)
		area.x2 = clip->x2;
	if (area.y2 > clip->y2)
		area.y2 = clip->y2;
	if ((area.x1 > area.x2) || (area.y1 > area.y2))
		return FB_GFX3_OK;
	if ((flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0) {
		result = gles_copy_texture_region(state, surface, area.x1, area.y1,
			(uint32_t)(area.x2 - area.x1 + 1),
			(uint32_t)(area.y2 - area.y1 + 1), &snapshot);
		if (result != FB_GFX3_OK)
			return result;
	}
	result = gles_attach_surface(state, surface, GL_FRAMEBUFFER);
	if (result != FB_GFX3_OK) {
		if (snapshot != 0)
			glDeleteTextures(1, &snapshot);
		return result;
	}
	glViewport(0, 0, (GLsizei)surface->width, (GLsizei)surface->height);
	glEnable(GL_SCISSOR_TEST);
	glScissor(area.x1, area.y1, area.x2 - area.x1 + 1,
		area.y2 - area.y1 + 1);
	glDisable(GL_BLEND);
	glUseProgram(state->primitive_program);
	glUniform1i(glGetUniformLocation(state->primitive_program,
		"operation_points"), 0);
	glUniform1i(glGetUniformLocation(state->primitive_program,
		"operation_points_alpha"), 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, snapshot);
	glUniform1i(glGetUniformLocation(state->primitive_program,
		"destination_snapshot"), 1);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(state->primitive_program,
		"operation_type"), operation_type);
	glUniform4i(glGetUniformLocation(state->primitive_program,
		"operation_clip"), clip->x1, clip->y1, clip->x2, clip->y2);
	glUniform4iv(glGetUniformLocation(state->primitive_program,
		"operation_data"), 1, data);
	glUniform2i(glGetUniformLocation(state->primitive_program,
		"operation_snapshot_origin"), area.x1, area.y1);
	glUniform2f(glGetUniformLocation(state->primitive_program,
		"operation_radii"), radius_x, radius_y);
	glUniform1ui(glGetUniformLocation(state->primitive_program,
		"operation_color"), color);
	glUniform1ui(glGetUniformLocation(state->primitive_program,
		"operation_style"), style & 0xFFFFu);
	glUniform1ui(glGetUniformLocation(state->primitive_program,
		"operation_mask"), gles_color_mask(surface->depth));
	glUniform1ui(glGetUniformLocation(state->primitive_program,
		"operation_filled"), filled != FALSE);
	glUniform1ui(glGetUniformLocation(state->primitive_program,
		"operation_flags"), flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND);
	glBindVertexArray(state->vertex_array);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDisable(GL_SCISSOR_TEST);
	result = gles_check_error(state, "fragment primitive draw");
	if (snapshot != 0)
		glDeleteTextures(1, &snapshot);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Command implementations                                                   */
/* ------------------------------------------------------------------------- */

static int gles_surface_create(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_SURFACE_CREATE_COMMAND *payload;
	FB_GFX3_GLES_SURFACE *surface;
	FB_GFX3_RECT full;
	FB_GFX3_HANDLE handle;
	int data[4] = { 0, 0, 0, 0 };
	int result;

	if ((command->completion == NULL) ||
	    (fb_gfx3_command_payload_size(command) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_SURFACE_CREATE_COMMAND *)command->payload;
	if ((payload->width == 0) || (payload->height == 0) ||
	    (payload->width > state->maximum_texture_size) ||
	    (payload->height > state->maximum_texture_size) ||
	    (gles_bytes_per_pixel(payload->depth) == 0))
		return FB_GFX3_INVALID;
	surface = (FB_GFX3_GLES_SURFACE *)calloc(1, sizeof(*surface));
	if (surface == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	surface->state = state;
	surface->width = payload->width;
	surface->height = payload->height;
	surface->depth = payload->depth;
	glGenTextures(1, &surface->texture);
	glBindTexture(GL_TEXTURE_2D, surface->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8UI, (GLsizei)surface->width,
		(GLsizei)surface->height);
	result = gles_check_error(state, "surface texture allocation");
	if (result != FB_GFX3_OK) {
		gles_surface_destroy(surface);
		return result;
	}
	full.x1 = 0;
	full.y1 = 0;
	full.x2 = (int32_t)surface->width - 1;
	full.y2 = (int32_t)surface->height - 1;
	result = gles_draw_primitive(state, surface, &full, &full, 0, data,
		0.0f, 0.0f, payload->clear_color, 0xFFFFu, TRUE, 0);
	if (result != FB_GFX3_OK) {
		gles_surface_destroy(surface);
		return result;
	}
	handle = fb_gfx3_resource_register(state->resources,
		FB_GFX3_RESOURCE_SURFACE, surface, gles_surface_destroy);
	if (handle == 0) {
		gles_surface_destroy(surface);
		return FB_GFX3_OUT_OF_MEMORY;
	}
	result = fb_gfx3_completion_set_value(command->completion, 0, handle);
	if (result != FB_GFX3_OK) {
		fb_gfx3_resource_release(state->resources, handle);
		fb_gfx3_resources_collect(state->resources, UINT64_MAX);
	}
	return result;
}

static int gles_surface_release(FB_GFX3_GLES_STATE *state,
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

static int gles_surface_upload(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_SURFACE_UPLOAD_COMMAND *payload;
	FB_GFX3_GLES_SURFACE *surface;
	unsigned char *expanded = NULL;
	size_t header_size = offsetof(FB_GFX3_SURFACE_UPLOAD_COMMAND, data);
	size_t expected_size;
	size_t expanded_size;
	size_t row_size;
	uint32_t bytes_per_pixel;
	uint32_t x;
	uint32_t y;
	int result;

	if (fb_gfx3_command_payload_size(command) < header_size)
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_SURFACE_UPLOAD_COMMAND *)command->payload;
	if ((payload->width == 0) || (payload->height == 0) ||
	    (payload->destination_x < 0) || (payload->destination_y < 0) ||
	    (payload->data_size != fb_gfx3_command_payload_size(command) -
		header_size))
		return FB_GFX3_INVALID;
	result = gles_surface_retain(state, command->target, command->sequence,
		&surface);
	if (result != FB_GFX3_OK)
		return result;
	bytes_per_pixel = gles_bytes_per_pixel(surface->depth);
	if (((uint64_t)(uint32_t)payload->destination_x + payload->width >
	     surface->width) ||
	    ((uint64_t)(uint32_t)payload->destination_y + payload->height >
	     surface->height) ||
	    (fb_gfx3_size_multiply(payload->width, bytes_per_pixel,
	     &row_size) != FB_GFX3_OK) || (payload->source_pitch < row_size) ||
	    (fb_gfx3_size_multiply(payload->source_pitch, payload->height,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != payload->data_size) ||
	    (fb_gfx3_size_multiply((size_t)payload->width * 4u,
	     payload->height, &expanded_size) != FB_GFX3_OK)) {
		result = FB_GFX3_INVALID;
		goto done;
	}

	/*
		A 32-bit gfxlib surface and the GLES integer texture use the same
		little-endian byte order. Full-frame software presenters already
		supply tightly packed rows, so uploading that memory directly avoids
		allocating and rebuilding an identical RGBA buffer every frame.

		Pitched subimages and lower-depth indexed surfaces retain the general
		conversion path below.
	*/
	if ((surface->depth == 32u) &&
	    (payload->source_pitch == payload->width * 4u)) {
		glBindTexture(GL_TEXTURE_2D, surface->texture);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glTexSubImage2D(GL_TEXTURE_2D, 0, payload->destination_x,
			payload->destination_y, (GLsizei)payload->width,
			(GLsizei)payload->height, GL_RGBA_INTEGER,
			GL_UNSIGNED_BYTE, payload->data);
		result = gles_check_error(state, "32-bit surface upload");
		goto done;
	}

	expanded = (unsigned char *)malloc(expanded_size);
	if (expanded == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	for (y = 0; y < payload->height; y++) {
		const unsigned char *source_row = payload->data +
			((size_t)y * payload->source_pitch);
		unsigned char *destination_row = expanded +
			((size_t)y * payload->width * 4u);
		for (x = 0; x < payload->width; x++)
			gles_pack_texture_pixel(destination_row + ((size_t)x * 4u),
				gles_decode_pixel(source_row +
				((size_t)x * bytes_per_pixel), bytes_per_pixel));
	}
	glBindTexture(GL_TEXTURE_2D, surface->texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, payload->destination_x,
		payload->destination_y, (GLsizei)payload->width,
		(GLsizei)payload->height, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, expanded);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	result = gles_check_error(state, "surface upload");

done:
	free(expanded);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int gles_read_region(FB_GFX3_GLES_STATE *state,
	FB_GFX3_GLES_SURFACE *surface, int x, int y, uint32_t width,
	uint32_t height, unsigned char *pixels)
{
	int result;

	if ((state->readback_texture == 0) ||
	    (state->readback_width != width) ||
	    (state->readback_height != height)) {
		if (state->readback_texture != 0)
			glDeleteTextures(1, &state->readback_texture);
		state->readback_texture = 0;
		state->readback_width = 0;
		state->readback_height = 0;
		glGenTextures(1, &state->readback_texture);
		glBindTexture(GL_TEXTURE_2D, state->readback_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, (GLsizei)width,
			(GLsizei)height);
		result = gles_check_error(state, "readback staging allocation");
		if (result != FB_GFX3_OK)
			return result;
		state->readback_width = width;
		state->readback_height = height;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, state->framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		state->readback_texture, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return FB_GFX3_UNSUPPORTED;
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	glUseProgram(state->readback_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, surface->texture);
	glUniform1i(state->readback_source_location, 0);
	glUniform2i(state->readback_origin_location, x, y);
	glBindVertexArray(state->vertex_array);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, (GLsizei)width, (GLsizei)height, GL_RGBA,
		GL_UNSIGNED_BYTE, pixels);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	return gles_check_error(state, "surface readback");
}

static int gles_surface_download(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_SURFACE_DOWNLOAD_COMMAND *payload;
	FB_GFX3_GLES_SURFACE *surface;
	unsigned char *packed = NULL;
	unsigned char *destination;
	size_t expected_size;
	size_t packed_size;
	size_t row_size;
	uint32_t bytes_per_pixel;
	uint32_t x;
	uint32_t y;
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
	result = gles_surface_retain(state, command->target, command->sequence,
		&surface);
	if (result != FB_GFX3_OK)
		return result;
	bytes_per_pixel = gles_bytes_per_pixel(surface->depth);
	if (((uint64_t)(uint32_t)payload->source_x + payload->width >
	     surface->width) ||
	    ((uint64_t)(uint32_t)payload->source_y + payload->height >
	     surface->height) ||
	    (fb_gfx3_size_multiply(payload->width, bytes_per_pixel,
	     &row_size) != FB_GFX3_OK) || (payload->destination_pitch < row_size) ||
	    (fb_gfx3_size_multiply(payload->destination_pitch, payload->height,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != payload->destination_size) ||
	    (fb_gfx3_size_multiply((size_t)payload->width * 4u,
	     payload->height, &packed_size) != FB_GFX3_OK)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	packed = (unsigned char *)malloc(packed_size);
	if (packed == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	result = gles_read_region(state, surface, payload->source_x,
		payload->source_y, payload->width, payload->height, packed);
	if (result != FB_GFX3_OK)
		goto done;
	destination = (unsigned char *)(uintptr_t)payload->destination_address;
	for (y = 0; y < payload->height; y++) {
		unsigned char *destination_row = destination +
			((size_t)y * payload->destination_pitch);
		const unsigned char *source_row = packed +
			((size_t)y * payload->width * 4u);
		for (x = 0; x < payload->width; x++)
			gles_encode_pixel(destination_row +
				((size_t)x * bytes_per_pixel), bytes_per_pixel,
				gles_unpack_texture_pixel(source_row + ((size_t)x * 4u)));
	}

done:
	free(packed);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int gles_clear(FB_GFX3_GLES_STATE *state, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_CLEAR_COMMAND *payload;
	FB_GFX3_GLES_SURFACE *surface;
	FB_GFX3_RECT clip;
	int data[4] = { 0, 0, 0, 0 };
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_CLEAR_COMMAND *)command->payload;
	result = gles_surface_retain(state, command->target, command->sequence,
		&surface);
	if (result != FB_GFX3_OK)
		return result;
	if (gles_clip_rect(surface, &payload->clip, &clip))
		result = gles_draw_primitive(state, surface, &clip, &clip, 0, data,
			0.0f, 0.0f, payload->color, 0xFFFFu, TRUE,
			payload->flags);
	else
		result = FB_GFX3_OK;
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int gles_points(FB_GFX3_GLES_STATE *state, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_POINTS_COMMAND *payload;
	FB_GFX3_GLES_SURFACE *surface;
	FB_GFX3_RECT clip;
	size_t points_size;
	size_t expected_size;
	FB_GFX3_RECT area;
	GLuint snapshot = 0;
	uint32_t index;
	int need_snapshot = FALSE;
	int result;

	if (fb_gfx3_command_payload_size(command) <
	    offsetof(FB_GFX3_POINTS_COMMAND, point))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_POINTS_COMMAND *)command->payload;
	if ((fb_gfx3_size_multiply(payload->count, sizeof(payload->point[0]),
	     &points_size) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point), points_size,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)))
		return FB_GFX3_INVALID;
	result = gles_surface_retain(state, command->target, command->sequence,
		&surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!gles_clip_rect(surface, &payload->clip, &clip)) {
		result = FB_GFX3_OK;
		goto done;
	}
	area.x1 = INT32_MAX;
	area.y1 = INT32_MAX;
	area.x2 = INT32_MIN;
	area.y2 = INT32_MIN;
	for (index = 0; index < payload->count; index++) {
		const FB_GFX3_POINT *point = &payload->point[index];

		if ((point->x < clip.x1) || (point->x > clip.x2) ||
		    (point->y < clip.y1) || (point->y > clip.y2))
			continue;
		if (point->x < area.x1)
			area.x1 = point->x;
		if (point->y < area.y1)
			area.y1 = point->y;
		if (point->x > area.x2)
			area.x2 = point->x;
		if (point->y > area.y2)
			area.y2 = point->y;
		if ((point->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0)
			need_snapshot = TRUE;
	}
	if ((area.x1 > area.x2) || (area.y1 > area.y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	if (need_snapshot) {
		result = gles_copy_texture_region(state, surface, area.x1, area.y1,
			(uint32_t)(area.x2 - area.x1 + 1),
			(uint32_t)(area.y2 - area.y1 + 1), &snapshot);
		if (result != FB_GFX3_OK)
			goto done;
	}
	result = gles_attach_surface(state, surface, GL_FRAMEBUFFER);
	if (result != FB_GFX3_OK)
		goto done;
	glViewport(0, 0, (GLsizei)surface->width, (GLsizei)surface->height);
	glEnable(GL_SCISSOR_TEST);
	glScissor(area.x1, area.y1, area.x2 - area.x1 + 1,
		area.y2 - area.y1 + 1);
	glDisable(GL_BLEND);
	glUseProgram(state->primitive_program);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, snapshot);
	glUniform1i(glGetUniformLocation(state->primitive_program,
		"destination_snapshot"), 1);
	glUniform1i(glGetUniformLocation(state->primitive_program,
		"operation_points"), 1);
	/*
		The shared primitive fragment shader selects its POINT payload only for
		operation type 4.  This must be set for every point batch because the
		uniform otherwise retains the preceding LINE, BOX, or CIRCLE command's
		value.  On GLES that stale value made PSET test the prior primitive's
		coverage and silently leave the destination pixel unchanged.
	*/
	glUniform1i(glGetUniformLocation(state->primitive_program,
		"operation_type"), 4);
	glUniform1i(glGetUniformLocation(state->primitive_program,
		"operation_points_alpha"), need_snapshot);
	glUniform2f(glGetUniformLocation(state->primitive_program,
		"operation_surface_size"), (float)surface->width,
		(float)surface->height);
	glUniform1ui(glGetUniformLocation(state->primitive_program,
		"operation_mask"), gles_color_mask(surface->depth));
	glUniform2i(glGetUniformLocation(state->primitive_program,
		"operation_snapshot_origin"), area.x1, area.y1);
	glBindVertexArray(state->vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)points_size, payload->point,
		GL_STREAM_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 2, GL_INT, sizeof(FB_GFX3_POINT), NULL);
	glVertexAttribDivisor(1, 1);
	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(FB_GFX3_POINT),
		(const void *)offsetof(FB_GFX3_POINT, color));
	glVertexAttribDivisor(2, 1);
	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(FB_GFX3_POINT),
		(const void *)offsetof(FB_GFX3_POINT, flags));
	glVertexAttribDivisor(3, 1);
	glDrawArraysInstanced(GL_POINTS, 0, 1, (GLsizei)payload->count);
	glVertexAttribDivisor(1, 0);
	glVertexAttribDivisor(2, 0);
	glVertexAttribDivisor(3, 0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisable(GL_SCISSOR_TEST);
	result = gles_check_error(state, "instanced point draw");

done:
	if (snapshot != 0)
		glDeleteTextures(1, &snapshot);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

/*
	Opaque point commands can be concatenated in submission order.  GLES point
	rasterization provides the same last-writer order as the individual draws,
	while one vertex upload and draw removes the per-fragment cost that ordinary
	PRINT otherwise pays for every runtime output segment.
*/
static size_t gles_points_batch_count(FB_GFX3_COMMAND *const *commands,
	size_t available)
{
	const FB_GFX3_POINTS_COMMAND *first;
	size_t index;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_POINTS) ||
	    (fb_gfx3_command_payload_size(commands[0]) <
	     offsetof(FB_GFX3_POINTS_COMMAND, point)))
		return 1u;
	first = (const FB_GFX3_POINTS_COMMAND *)commands[0]->payload;
	if (available > FB_GFX3_GLES_POINTS_BATCH_LIMIT)
		available = FB_GFX3_GLES_POINTS_BATCH_LIMIT;
	for (index = 0; index < available; ++index) {
		const FB_GFX3_POINTS_COMMAND *payload;
		size_t points_size;
		size_t expected_size;
		uint32_t point_index;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_POINTS) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) <
		     offsetof(FB_GFX3_POINTS_COMMAND, point)))
			break;
		payload = (const FB_GFX3_POINTS_COMMAND *)commands[index]->payload;
		if ((payload->count == 0u) ||
		    (memcmp(&payload->clip, &first->clip, sizeof(payload->clip)) != 0) ||
		    (fb_gfx3_size_multiply(payload->count, sizeof(payload->point[0]),
		     &points_size) != FB_GFX3_OK) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point),
		     points_size, &expected_size) != FB_GFX3_OK) ||
		    (expected_size != fb_gfx3_command_payload_size(commands[index])))
			break;
		for (point_index = 0; point_index < payload->count; ++point_index) {
			if (payload->point[point_index].flags != 0u) {
				/*
					Alpha points need their own destination snapshot and cannot
					join the opaque instanced batch.  The first command must still
					consume one queue entry; returning zero here would leave the
					render loop on the same command forever.
				*/
				return (index == 0u) ? 1u : index;
			}
		}
	}
	return (index < 2u) ? 1u : index;
}

static int gles_points_batch(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *const *commands, size_t count)
{
	FB_GFX3_COMMAND *batch_command = NULL;
	FB_GFX3_POINTS_COMMAND *batch;
	size_t total_points = 0u;
	size_t points_size;
	size_t payload_size;
	size_t index;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_GLES_POINTS_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	for (index = 0; index < count; ++index) {
		const FB_GFX3_POINTS_COMMAND *payload;
		size_t source_points_size;
		size_t expected_size;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_POINTS) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) <
		     offsetof(FB_GFX3_POINTS_COMMAND, point)))
			return FB_GFX3_INVALID;
		payload = (const FB_GFX3_POINTS_COMMAND *)commands[index]->payload;
		if ((fb_gfx3_size_multiply(payload->count, sizeof(payload->point[0]),
		     &source_points_size) != FB_GFX3_OK) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point),
		     source_points_size, &expected_size) != FB_GFX3_OK) ||
		    (expected_size != fb_gfx3_command_payload_size(commands[index])) ||
		    (fb_gfx3_size_add(total_points, payload->count,
		     &total_points) != FB_GFX3_OK))
			return FB_GFX3_INVALID;
	}
	if ((total_points > UINT32_MAX) ||
	    (fb_gfx3_size_multiply(total_points, sizeof(batch->point[0]),
	     &points_size) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point),
	     points_size, &payload_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	batch_command = fb_gfx3_command_create(FB_GFX3_COMMAND_POINTS,
		payload_size);
	if (batch_command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	batch_command->target = commands[0]->target;
	batch_command->sequence = commands[count - 1u]->sequence;
	batch = (FB_GFX3_POINTS_COMMAND *)batch_command->payload;
	batch->clip = ((const FB_GFX3_POINTS_COMMAND *)commands[0]->payload)->clip;
	batch->count = (uint32_t)total_points;
	points_size = 0u;
	for (index = 0; index < count; ++index) {
		const FB_GFX3_POINTS_COMMAND *payload =
			(const FB_GFX3_POINTS_COMMAND *)commands[index]->payload;
		size_t source_points_size = (size_t)payload->count *
			sizeof(payload->point[0]);

		memcpy((unsigned char *)batch->point + points_size, payload->point,
			source_points_size);
		points_size += source_points_size;
	}
	result = gles_points(state, batch_command);
	fb_gfx3_command_destroy(batch_command);
	return result;
}

static int gles_line_batch_steps(const FB_GFX3_LINE_COMMAND *payload,
	uint32_t *steps)
{
	int64_t horizontal;
	int64_t vertical;
	uint64_t maximum;

	if ((payload == NULL) || (steps == NULL))
		return FALSE;
	horizontal = (int64_t)payload->x2 - (int64_t)payload->x1;
	vertical = (int64_t)payload->y2 - (int64_t)payload->y1;
	if (horizontal < 0)
		horizontal = -horizontal;
	if (vertical < 0)
		vertical = -vertical;
	maximum = (uint64_t)((horizontal > vertical) ? horizontal : vertical);
	if (maximum >= FB_GFX3_GLES_LINE_BATCH_MAX_STEPS)
		return FALSE;
	*steps = (uint32_t)maximum + 1u;
	return TRUE;
}

static size_t gles_line_batch_count(FB_GFX3_COMMAND *const *commands,
	size_t available)
{
	const FB_GFX3_LINE_COMMAND *first;
	size_t index;
	uint32_t steps;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_LINE) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1u;
	first = (const FB_GFX3_LINE_COMMAND *)commands[0]->payload;
	if ((first->flags != 0u) || !gles_line_batch_steps(first, &steps))
		return 1u;
	if (available > FB_GFX3_GLES_LINE_BATCH_LIMIT)
		available = FB_GFX3_GLES_LINE_BATCH_LIMIT;
	for (index = 1u; index < available; index++) {
		const FB_GFX3_LINE_COMMAND *candidate;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_LINE) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_LINE_COMMAND *)commands[index]->payload;
		if ((candidate->flags != 0u) ||
		    !gles_line_batch_steps(candidate, &steps) ||
		    (memcmp(&candidate->clip, &first->clip,
		     sizeof(candidate->clip)) != 0))
			break;
	}
	return index;
}

static int gles_line_batch(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *const *commands, size_t count)
{
	FB_GFX3_GLES_LINE_BATCH_ITEM items[FB_GFX3_GLES_LINE_BATCH_LIMIT];
	const FB_GFX3_LINE_COMMAND *first;
	FB_GFX3_GLES_SURFACE *surface;
	FB_GFX3_RECT clip;
	size_t index;
	uint32_t maximum_steps = 0u;
	uint32_t steps;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_GLES_LINE_BATCH_LIMIT) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)) ||
	    (state->line_batch_program == 0))
		return FB_GFX3_INVALID;
	first = (const FB_GFX3_LINE_COMMAND *)commands[0]->payload;
	result = gles_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!gles_clip_rect(surface, &first->clip, &clip)) {
		result = FB_GFX3_OK;
		goto done;
	}
	for (index = 0u; index < count; index++) {
		const FB_GFX3_LINE_COMMAND *payload =
			(const FB_GFX3_LINE_COMMAND *)commands[index]->payload;

		items[index].x1 = payload->x1;
		items[index].y1 = payload->y1;
		items[index].x2 = payload->x2;
		items[index].y2 = payload->y2;
		items[index].color = payload->color & gles_color_mask(surface->depth);
		items[index].style = payload->style & 0xFFFFu;
		if (!gles_line_batch_steps(payload, &steps)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (steps > maximum_steps)
			maximum_steps = steps;
	}
	result = gles_attach_surface(state, surface, GL_FRAMEBUFFER);
	if (result != FB_GFX3_OK)
		goto done;
	glViewport(0, 0, (GLsizei)surface->width, (GLsizei)surface->height);
	glEnable(GL_SCISSOR_TEST);
	glScissor(clip.x1, clip.y1, clip.x2 - clip.x1 + 1,
		clip.y2 - clip.y1 + 1);
	glDisable(GL_BLEND);
	glUseProgram(state->line_batch_program);
	glUniform2f(state->line_batch_size_location, (GLfloat)surface->width,
		(GLfloat)surface->height);
	glBindVertexArray(state->vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(count * sizeof(items[0])),
		items, GL_STREAM_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 4, GL_INT, sizeof(items[0]), NULL);
	glVertexAttribDivisor(1, 1);
	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(items[0]),
		(const void *)offsetof(FB_GFX3_GLES_LINE_BATCH_ITEM, color));
	glVertexAttribDivisor(2, 1);
	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(items[0]),
		(const void *)offsetof(FB_GFX3_GLES_LINE_BATCH_ITEM, style));
	glVertexAttribDivisor(3, 1);
	glDrawArraysInstanced(GL_POINTS, 0, (GLsizei)maximum_steps,
		(GLsizei)count);
	glVertexAttribDivisor(1, 0);
	glVertexAttribDivisor(2, 0);
	glVertexAttribDivisor(3, 0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisable(GL_SCISSOR_TEST);
	result = gles_check_error(state, "instanced exact line batch");

done:
	fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

static int gles_line(FB_GFX3_GLES_STATE *state, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_LINE_COMMAND *payload;
	FB_GFX3_GLES_SURFACE *surface;
	FB_GFX3_RECT clip;
	FB_GFX3_RECT area;
	int data[4];
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_LINE_COMMAND *)command->payload;
	result = gles_surface_retain(state, command->target, command->sequence,
		&surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!gles_clip_rect(surface, &payload->clip, &clip)) {
		result = FB_GFX3_OK;
		goto done;
	}
	area.x1 = (payload->x1 < payload->x2) ? payload->x1 : payload->x2;
	area.y1 = (payload->y1 < payload->y2) ? payload->y1 : payload->y2;
	area.x2 = (payload->x1 > payload->x2) ? payload->x1 : payload->x2;
	area.y2 = (payload->y1 > payload->y2) ? payload->y1 : payload->y2;
	data[0] = payload->x1;
	data[1] = payload->y1;
	data[2] = payload->x2;
	data[3] = payload->y2;
	result = gles_draw_primitive(state, surface, &clip, &area, 1, data,
		0.0f, 0.0f, payload->color, payload->style, FALSE,
		payload->flags);

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static size_t gles_rectangle_batch_count(FB_GFX3_COMMAND *const *commands,
	size_t available)
{
	const FB_GFX3_RECTANGLE_COMMAND *first;
	size_t index;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_RECTANGLE) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1u;
	first = (const FB_GFX3_RECTANGLE_COMMAND *)commands[0]->payload;
	if ((first->filled == 0u) || (first->flags != 0u) ||
	    (first->x1 > first->x2) || (first->y1 > first->y2))
		return 1u;
	if (available > FB_GFX3_GLES_RECTANGLE_BATCH_LIMIT)
		available = FB_GFX3_GLES_RECTANGLE_BATCH_LIMIT;
	for (index = 1u; index < available; index++) {
		const FB_GFX3_RECTANGLE_COMMAND *candidate;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_RECTANGLE) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_RECTANGLE_COMMAND *)commands[index]->payload;
		if ((candidate->filled == 0u) || (candidate->flags != 0u) ||
		    (candidate->x1 > candidate->x2) || (candidate->y1 > candidate->y2) ||
		    (memcmp(&candidate->clip, &first->clip,
		     sizeof(candidate->clip)) != 0))
			break;
	}
	return index;
}

static int gles_rectangle_batch(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *const *commands, size_t count)
{
	FB_GFX3_GLES_RECTANGLE_BATCH_ITEM items[FB_GFX3_GLES_RECTANGLE_BATCH_LIMIT];
	const FB_GFX3_RECTANGLE_COMMAND *first;
	FB_GFX3_GLES_SURFACE *surface;
	FB_GFX3_RECT clip;
	size_t index;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_GLES_RECTANGLE_BATCH_LIMIT) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)) ||
	    (state->rectangle_batch_program == 0))
		return FB_GFX3_INVALID;
	first = (const FB_GFX3_RECTANGLE_COMMAND *)commands[0]->payload;
	result = gles_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!gles_clip_rect(surface, &first->clip, &clip)) {
		result = FB_GFX3_OK;
		goto done;
	}
	for (index = 0u; index < count; index++) {
		const FB_GFX3_RECTANGLE_COMMAND *payload =
			(const FB_GFX3_RECTANGLE_COMMAND *)commands[index]->payload;

		items[index].x1 = payload->x1;
		items[index].y1 = payload->y1;
		items[index].x2 = payload->x2;
		items[index].y2 = payload->y2;
		items[index].color = payload->color & gles_color_mask(surface->depth);
	}
	result = gles_attach_surface(state, surface, GL_FRAMEBUFFER);
	if (result != FB_GFX3_OK)
		goto done;
	glViewport(0, 0, (GLsizei)surface->width, (GLsizei)surface->height);
	glEnable(GL_SCISSOR_TEST);
	glScissor(clip.x1, clip.y1, clip.x2 - clip.x1 + 1,
		clip.y2 - clip.y1 + 1);
	glDisable(GL_BLEND);
	glUseProgram(state->rectangle_batch_program);
	glUniform2f(state->rectangle_batch_size_location,
		(GLfloat)surface->width, (GLfloat)surface->height);
	glBindVertexArray(state->vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(count * sizeof(items[0])),
		items, GL_STREAM_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 4, GL_INT, sizeof(items[0]), NULL);
	glVertexAttribDivisor(1, 1);
	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(items[0]),
		(const void *)offsetof(FB_GFX3_GLES_RECTANGLE_BATCH_ITEM, color));
	glVertexAttribDivisor(2, 1);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)count);
	glVertexAttribDivisor(1, 0);
	glVertexAttribDivisor(2, 0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisable(GL_SCISSOR_TEST);
	result = gles_check_error(state, "instanced opaque rectangle batch");

done:
	fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

static int gles_rectangle(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_RECTANGLE_COMMAND *payload;
	FB_GFX3_GLES_SURFACE *surface;
	FB_GFX3_RECT clip;
	FB_GFX3_RECT area;
	int data[4];
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_RECTANGLE_COMMAND *)command->payload;
	if ((payload->x1 > payload->x2) || (payload->y1 > payload->y2))
		return FB_GFX3_INVALID;
	result = gles_surface_retain(state, command->target, command->sequence,
		&surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!gles_clip_rect(surface, &payload->clip, &clip)) {
		result = FB_GFX3_OK;
		goto done;
	}
	area.x1 = payload->x1;
	area.y1 = payload->y1;
	area.x2 = payload->x2;
	area.y2 = payload->y2;
	data[0] = payload->x1;
	data[1] = payload->y1;
	data[2] = payload->x2;
	data[3] = payload->y2;
	result = gles_draw_primitive(state, surface, &clip, &area, 2, data,
		0.0f, 0.0f, payload->color, payload->style,
		payload->filled != 0, payload->flags);

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

/*
	OpenGL ES 3.0 has no compute shader or image-store path. Full ellipses
	therefore retain gfxlib2's midpoint control sequence on the render thread,
	but rasterize every resulting span or endpoint through the GPU fragment
	pipeline. This is deliberately different from the old implicit-distance
	approximation: the compatibility contract is the exact midpoint pixel set,
	not a mathematically smooth-looking boundary.
*/
static void gles_append_ellipse_span(const FB_GFX3_RECT *clip,
	FB_GFX3_GLES_ELLIPSE_SPAN_ITEM *items, uint32_t *count, int y, int x1,
	int x2, uint32_t color, uint32_t limit)
{
	FB_GFX3_GLES_ELLIPSE_SPAN_ITEM *item;

	if ((y < clip->y1) || (y > clip->y2) || (x2 < clip->x1) ||
	    (x1 > clip->x2))
		return;
	/*
		The caller limits each radius to 256, so the midpoint sequence cannot
		exceed the fixed 1025 span array. Keep the guard nevertheless because
		this is a renderer boundary fed by public drawing APIs.
	*/
	if (*count >= limit)
		return;
	item = &items[*count];
	item->x1 = (x1 < clip->x1) ? clip->x1 : x1;
	item->y1 = y;
	item->x2 = (x2 > clip->x2) ? clip->x2 : x2;
	item->y2 = y;
	item->color = color;
	item->reserved1 = 0;
	item->reserved2 = 0;
	item->reserved3 = 0;
	(*count)++;
}

static int gles_draw_ellipse_span_batch(FB_GFX3_GLES_STATE *state,
	FB_GFX3_GLES_SURFACE *surface,
	const FB_GFX3_GLES_ELLIPSE_SPAN_ITEM *items, uint32_t count)
{
	int result;

	if ((items == NULL) || (count == 0u) ||
	    (count > FB_GFX3_GLES_ELLIPSE_BATCH_SPAN_LIMIT) ||
	    (state->ellipse_span_batch_program == 0))
		return FB_GFX3_INVALID;
	result = gles_attach_surface(state, surface, GL_FRAMEBUFFER);
	if (result != FB_GFX3_OK)
		return result;
	glViewport(0, 0, (GLsizei)surface->width, (GLsizei)surface->height);
	glDisable(GL_BLEND);
	glUseProgram(state->ellipse_span_batch_program);
	glUniform2f(state->ellipse_span_batch_size_location,
		(GLfloat)surface->width, (GLfloat)surface->height);
	glBindVertexArray(state->vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(count * sizeof(items[0])),
		items, GL_STREAM_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 4, GL_INT, sizeof(items[0]), NULL);
	glVertexAttribDivisor(1, 1);
	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(items[0]),
		(const void *)offsetof(FB_GFX3_GLES_ELLIPSE_SPAN_ITEM, color));
	glVertexAttribDivisor(2, 1);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)count);
	glVertexAttribDivisor(1, 0);
	glVertexAttribDivisor(2, 0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	return gles_check_error(state, "exact GLES ellipse span batch");
}

static int gles_draw_ellipse_scanline(FB_GFX3_GLES_STATE *state,
	FB_GFX3_GLES_SURFACE *surface, const FB_GFX3_RECT *clip, int y, int x1,
	int x2, uint32_t color, int filled, uint32_t flags)
{
	FB_GFX3_RECT area;
	int data[4];
	int result;

	if ((y < clip->y1) || (y > clip->y2))
		return FB_GFX3_OK;
	if (filled) {
		if ((x2 < clip->x1) || (x1 > clip->x2))
			return FB_GFX3_OK;
		area.x1 = x1;
		area.y1 = y;
		area.x2 = x2;
		area.y2 = y;
		data[0] = x1;
		data[1] = y;
		data[2] = x2;
		data[3] = y;
		return gles_draw_primitive(state, surface, clip, &area, 2, data,
			0.0f, 0.0f, color, 0xFFFFu, TRUE, flags);
	}

	/*
		The two endpoint calls remain separate even when x1 equals x2. That
		preserves gfxlib2's ordered double write at a degenerate scanline,
		which is observable when alpha primitives are enabled.
	*/
	if ((x1 >= clip->x1) && (x1 <= clip->x2)) {
		area.x1 = x1;
		area.y1 = y;
		area.x2 = x1;
		area.y2 = y;
		result = gles_draw_primitive(state, surface, clip, &area, 0, data,
			0.0f, 0.0f, color, 0xFFFFu, TRUE, flags);
		if (result != FB_GFX3_OK)
			return result;
	}
	if ((x2 >= clip->x1) && (x2 <= clip->x2)) {
		area.x1 = x2;
		area.y1 = y;
		area.x2 = x2;
		area.y2 = y;
		return gles_draw_primitive(state, surface, clip, &area, 0, data,
			0.0f, 0.0f, color, 0xFFFFu, TRUE, flags);
	}
	return FB_GFX3_OK;
}

static void gles_append_opaque_ellipse_spans(
	const FB_GFX3_ELLIPSE_COMMAND *payload, const FB_GFX3_RECT *clip,
	uint32_t depth, FB_GFX3_GLES_ELLIPSE_SPAN_ITEM *items,
	uint32_t *count, uint32_t limit)
{
	int d;
	int x1;
	int x2;
	int y1;
	int y2;
	int64_t aq;
	int64_t bq;
	int64_t dx;
	int64_t dy;
	int64_t r;
	int64_t rx;
	int64_t ry;
	uint32_t color;

	/* The caller has already restricted this helper to the midpoint fast path. */
	x1 = (int)((float)payload->center_x - payload->radius_x);
	x2 = (int)((float)payload->center_x + payload->radius_x);
	y1 = payload->center_y;
	y2 = payload->center_y;
	color = payload->color & gles_color_mask(depth);
	gles_append_ellipse_span(clip, items, count, y1, x1, x2, color, limit);
	if (payload->radius_y == 0.0f)
		return;
	aq = (int64_t)(payload->radius_x * payload->radius_x);
	bq = (int64_t)(payload->radius_y * payload->radius_y);
	dx = aq * 2;
	dy = bq * 2;
	r = (int64_t)(payload->radius_x * (float)bq);
	rx = r * 2;
	ry = 0;
	d = (int)payload->radius_x;
	while (d > 0) {
		if (r > 0) {
			y1++;
			y2--;
			ry += dx;
			r -= ry;
		}
		if (r <= 0) {
			d--;
			x1++;
			x2--;
			rx -= dy;
			r += rx;
		}
		gles_append_ellipse_span(clip, items, count, y1, x1, x2, color, limit);
		gles_append_ellipse_span(clip, items, count, y2, x1, x2, color, limit);
	}
}

static int gles_ellipse_batchable(const FB_GFX3_ELLIPSE_COMMAND *payload)
{
	return (payload->filled != 0u) && (payload->flags == 0u) &&
		(payload->radius_x >= 0.0f) && (payload->radius_x <= 256.0f) &&
		(payload->radius_y >= 0.0f) && (payload->radius_y <= 256.0f);
}

static size_t gles_ellipse_batch_count(FB_GFX3_COMMAND *const *commands,
	size_t available)
{
	const FB_GFX3_ELLIPSE_COMMAND *first;
	size_t index;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_ELLIPSE) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1u;
	first = (const FB_GFX3_ELLIPSE_COMMAND *)commands[0]->payload;
	if (!gles_ellipse_batchable(first))
		return 1u;
	if (available > FB_GFX3_GLES_ELLIPSE_BATCH_LIMIT)
		available = FB_GFX3_GLES_ELLIPSE_BATCH_LIMIT;
	for (index = 1u; index < available; ++index) {
		const FB_GFX3_ELLIPSE_COMMAND *candidate;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_ELLIPSE) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_ELLIPSE_COMMAND *)commands[index]->payload;
		if (!gles_ellipse_batchable(candidate) ||
		    (memcmp(&candidate->clip, &first->clip,
		     sizeof(candidate->clip)) != 0))
			break;
	}
	return index;
}

static int gles_ellipse_batch(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *const *commands, size_t count)
{
	FB_GFX3_GLES_ELLIPSE_SPAN_ITEM *items;
	const FB_GFX3_ELLIPSE_COMMAND *first;
	FB_GFX3_GLES_SURFACE *surface;
	FB_GFX3_RECT clip;
	uint32_t span_count = 0u;
	size_t index;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_GLES_ELLIPSE_BATCH_LIMIT) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return FB_GFX3_INVALID;
	first = (const FB_GFX3_ELLIPSE_COMMAND *)commands[0]->payload;
	result = gles_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!gles_clip_rect(surface, &first->clip, &clip)) {
		result = FB_GFX3_OK;
		goto done;
	}
	items = (FB_GFX3_GLES_ELLIPSE_SPAN_ITEM *)malloc(
		FB_GFX3_GLES_ELLIPSE_BATCH_SPAN_LIMIT * sizeof(items[0]));
	if (items == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	for (index = 0u; index < count; ++index) {
		const FB_GFX3_ELLIPSE_COMMAND *payload =
			(const FB_GFX3_ELLIPSE_COMMAND *)commands[index]->payload;

		gles_append_opaque_ellipse_spans(payload, &clip, surface->depth,
			items, &span_count, FB_GFX3_GLES_ELLIPSE_BATCH_SPAN_LIMIT);
	}
	result = (span_count == 0u) ? FB_GFX3_OK :
		gles_draw_ellipse_span_batch(state, surface, items, span_count);
	free(items);

done:
	fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

static int gles_ellipse(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_ELLIPSE_COMMAND *payload;
	FB_GFX3_GLES_SURFACE *surface;
	FB_GFX3_RECT clip;
	int d;
	int x1;
	int x2;
	int y1;
	int y2;
	int64_t aq;
	int64_t bq;
	int64_t dx;
	int64_t dy;
	int64_t r;
	int64_t rx;
	int64_t ry;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_ELLIPSE_COMMAND *)command->payload;
	if (!(payload->radius_x >= 0.0f) || !(payload->radius_x <= 32767.0f) ||
	    !(payload->radius_y >= 0.0f) || !(payload->radius_y <= 32767.0f))
		return FB_GFX3_INVALID;
	result = gles_surface_retain(state, command->target, command->sequence,
		&surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!gles_clip_rect(surface, &payload->clip, &clip)) {
		result = FB_GFX3_OK;
		goto done;
	}

	x1 = (int)((float)payload->center_x - payload->radius_x);
	x2 = (int)((float)payload->center_x + payload->radius_x);
	y1 = payload->center_y;
	y2 = payload->center_y;
	/*
		Opaque filled ellipses are the normal CIRCLE ... , BF case. Preserve
		the existing integer midpoint decisions, then let one instanced GPU draw
		rasterize all of their spans. Alpha and outline ellipses retain their
		ordered per-span path because their repeated writes are observable.
	*/
	if ((payload->filled != 0u) &&
	    ((payload->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) == 0u) &&
	    (payload->radius_x <= 256.0f) &&
	    (payload->radius_y <= 256.0f)) {
		FB_GFX3_GLES_ELLIPSE_SPAN_ITEM
			items[FB_GFX3_GLES_ELLIPSE_SPAN_BATCH_LIMIT];
		uint32_t count = 0;

		gles_append_ellipse_span(&clip, items, &count, y1, x1, x2,
			payload->color & gles_color_mask(surface->depth),
			FB_GFX3_GLES_ELLIPSE_SPAN_BATCH_LIMIT);
		if (payload->radius_y != 0.0f) {
			aq = (int64_t)(payload->radius_x * payload->radius_x);
			bq = (int64_t)(payload->radius_y * payload->radius_y);
			dx = aq * 2;
			dy = bq * 2;
			r = (int64_t)(payload->radius_x * (float)bq);
			rx = r * 2;
			ry = 0;
			d = (int)payload->radius_x;
			while (d > 0) {
				if (r > 0) {
					y1++;
					y2--;
					ry += dx;
					r -= ry;
				}
				if (r <= 0) {
					d--;
					x1++;
					x2--;
					rx -= dy;
					r += rx;
				}
				gles_append_ellipse_span(&clip, items, &count, y1, x1, x2,
					payload->color & gles_color_mask(surface->depth),
					FB_GFX3_GLES_ELLIPSE_SPAN_BATCH_LIMIT);
				gles_append_ellipse_span(&clip, items, &count, y2, x1, x2,
					payload->color & gles_color_mask(surface->depth),
					FB_GFX3_GLES_ELLIPSE_SPAN_BATCH_LIMIT);
			}
		}
		if (count == 0u) {
			result = FB_GFX3_OK;
			goto done;
		}
		result = gles_draw_ellipse_span_batch(state, surface, items, count);
		goto done;
	}
	if (payload->radius_y == 0.0f) {
		result = gles_draw_ellipse_scanline(state, surface, &clip, y1, x1,
			x2, payload->color, TRUE, payload->flags);
		goto done;
	}

	result = gles_draw_ellipse_scanline(state, surface, &clip, y1, x1, x2,
		payload->color, payload->filled != 0, payload->flags);
	if (result != FB_GFX3_OK)
		goto done;
	aq = (int64_t)(payload->radius_x * payload->radius_x);
	bq = (int64_t)(payload->radius_y * payload->radius_y);
	dx = aq * 2;
	dy = bq * 2;
	r = (int64_t)(payload->radius_x * (float)bq);
	rx = r * 2;
	ry = 0;
	d = (int)payload->radius_x;
	while (d > 0) {
		if (r > 0) {
			y1++;
			y2--;
			ry += dx;
			r -= ry;
		}
		if (r <= 0) {
			d--;
			x1++;
			x2--;
			rx -= dy;
			r += rx;
		}
		result = gles_draw_ellipse_scanline(state, surface, &clip, y1, x1,
			x2, payload->color, payload->filled != 0, payload->flags);
		if (result != FB_GFX3_OK)
			goto done;
		result = gles_draw_ellipse_scanline(state, surface, &clip, y2, x1,
			x2, payload->color, payload->filled != 0, payload->flags);
		if (result != FB_GFX3_OK)
			goto done;
	}

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int gles_copy_texture_region(FB_GFX3_GLES_STATE *state,
	FB_GFX3_GLES_SURFACE *source, int source_x, int source_y,
	uint32_t width, uint32_t height, GLuint *copy)
{
	int result;

	*copy = 0;
	glGenTextures(1, copy);
	glBindTexture(GL_TEXTURE_2D, *copy);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8UI, (GLsizei)width,
		(GLsizei)height);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, state->read_framebuffer);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, source->texture, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, state->draw_framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, *copy, 0);
	if ((glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) !=
	     GL_FRAMEBUFFER_COMPLETE) ||
	    (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) !=
	     GL_FRAMEBUFFER_COMPLETE)) {
		result = FB_GFX3_UNSUPPORTED;
		goto fail;
	}
	glBlitFramebuffer(source_x, source_y, source_x + (int)width,
		source_y + (int)height, 0, 0, (int)width, (int)height,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	result = gles_check_error(state, "GPU surface snapshot");
	if (result == FB_GFX3_OK)
		return result;

fail:
	glDeleteTextures(1, copy);
	*copy = 0;
	return result;
}

static int gles_paint_texture_create(FB_GFX3_GLES_STATE *state,
	uint32_t width, uint32_t height, GLuint *texture)
{
	if ((state == NULL) || (texture == NULL) || (width == 0) ||
	    (height == 0))
		return FB_GFX3_INVALID;
	*texture = 0;
	glGenTextures(1, texture);
	if (*texture == 0)
		return FB_GFX3_OUT_OF_MEMORY;
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8UI, (GLsizei)width,
		(GLsizei)height);
	return gles_check_error(state, "PAINT texture allocation");
}

static int gles_paint_draw(FB_GFX3_GLES_STATE *state, GLuint destination,
	GLuint source, GLuint mask, GLuint pattern, uint32_t width, uint32_t height,
	int mode,
	const FB_GFX3_PAINT_COMMAND *payload, const FB_GFX3_RECT *clip,
	uint32_t depth)
{
	if ((state == NULL) || (destination == 0) || (pattern == 0) ||
	    (payload == NULL) ||
	    (clip == NULL))
		return FB_GFX3_INVALID;
	glBindFramebuffer(GL_FRAMEBUFFER, state->framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, destination, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return FB_GFX3_UNSUPPORTED;
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	glUseProgram(state->paint_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, source);
	glUniform1i(glGetUniformLocation(state->paint_program, "surface_image"),
		0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, mask);
	glUniform1i(glGetUniformLocation(state->paint_program, "mask_image"), 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, pattern);
	glUniform1i(glGetUniformLocation(state->paint_program, "pattern_image"),
		2);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(state->paint_program, "operation_mode"),
		mode);
	glUniform2i(glGetUniformLocation(state->paint_program, "operation_seed"),
		payload->x, payload->y);
	glUniform4i(glGetUniformLocation(state->paint_program, "operation_clip"),
		clip->x1, clip->y1, clip->x2, clip->y2);
	glUniform1ui(glGetUniformLocation(state->paint_program, "operation_color"),
		payload->color);
	glUniform1ui(glGetUniformLocation(state->paint_program, "operation_border"),
		payload->border_color);
	glUniform1ui(glGetUniformLocation(state->paint_program, "operation_mask"),
		gles_color_mask(depth));
	glUniform1ui(glGetUniformLocation(state->paint_program, "operation_flags"),
		payload->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND);
	glUniform1ui(glGetUniformLocation(state->paint_program,
		"operation_paint_mode"), payload->paint_mode);
	/*
		GLES raster coordinates address the complete target texture. Unlike the
		CPU compatibility staging buffer, they are not rebased to VIEW, so the
		command's staging origin must not be added to the tile a second time.
	*/
	glUniform2ui(glGetUniformLocation(state->paint_program,
		"operation_pattern_origin"), 0, 0);
	glBindVertexArray(state->vertex_array);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	return FB_GFX3_OK;
}

/*
	The GLES PAINT frontier uses ping-pong mask textures.  A batch needs a
	stable copy of its starting mask so one occlusion query can tell whether
	any new pixels were reached by the entire batch.  This framebuffer copy is
	GPU-local and avoids a CPU readback after every expansion.
*/
static int gles_paint_texture_copy(FB_GFX3_GLES_STATE *state,
	GLuint destination, GLuint source, uint32_t width, uint32_t height)
{
	if ((state == NULL) || (destination == 0) || (source == 0) ||
	    (width == 0) || (height == 0))
		return FB_GFX3_INVALID;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, state->read_framebuffer);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, source, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, state->draw_framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, destination, 0);
	if ((glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) !=
	     GL_FRAMEBUFFER_COMPLETE) ||
	    (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) !=
	     GL_FRAMEBUFFER_COMPLETE))
		return FB_GFX3_UNSUPPORTED;
	glBlitFramebuffer(0, 0, (GLint)width, (GLint)height,
		0, 0, (GLint)width, (GLint)height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	return gles_check_error(state, "PAINT frontier mask copy");
}

/*
	The PAINT command owns raw 8 by 8 pattern bytes in explicit little-endian
	words. Upload them to one tiny integer texture per command. This is command
	data, not a surface readback or upload: the flood mask and target pixels stay
	in graphics memory for the whole operation. Texture fetch also avoids the
	driver-dependent dynamic uniform-array indexing that is undesirable on older
	ES 3.0 implementations.
*/
static unsigned char gles_paint_pattern_byte(
	const FB_GFX3_PAINT_COMMAND *payload, uint32_t offset)
{
	if ((payload == NULL) || (offset >= payload->pattern_size))
		return 0;
	return (unsigned char)(payload->pattern_word[offset / 4u] >>
		((offset & 3u) * 8u));
}

static int gles_paint_pattern_texture_create(FB_GFX3_GLES_STATE *state,
	const FB_GFX3_PAINT_COMMAND *payload, uint32_t depth, GLuint *texture)
{
	unsigned char pixels[8 * 8 * 4];
	uint32_t bytes_per_pixel;
	uint32_t tile_index;
	uint32_t byte_index;
	uint32_t offset;
	int result;

	if ((state == NULL) || (payload == NULL) || (texture == NULL))
		return FB_GFX3_INVALID;
	bytes_per_pixel = (depth <= 8u) ? 1u :
		((depth <= 16u) ? 2u : 4u);
	for (tile_index = 0; tile_index < 64u; tile_index++) {
		offset = tile_index * bytes_per_pixel;
		for (byte_index = 0; byte_index < 4u; byte_index++) {
			pixels[(tile_index * 4u) + byte_index] =
				(byte_index < bytes_per_pixel) ?
				gles_paint_pattern_byte(payload, offset + byte_index) : 0;
		}
	}
	result = gles_paint_texture_create(state, 8, 8, texture);
	if (result != FB_GFX3_OK)
		return result;
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 8, 8, GL_RGBA_INTEGER,
		GL_UNSIGNED_BYTE, pixels);
	return gles_check_error(state, "PAINT pattern texture upload");
}

static int gles_paint(FB_GFX3_GLES_STATE *state, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_PAINT_COMMAND *payload;
	FB_GFX3_GLES_SURFACE *surface;
	FB_GFX3_RECT clip;
	GLuint mask_a = 0;
	GLuint mask_b = 0;
	GLuint result_texture = 0;
	GLuint pattern_texture = 0;
	GLuint changed_query = 0;
	GLuint source_mask;
	GLuint destination_mask;
	GLuint swap_texture;
	size_t pixel_count;
	size_t iteration;
	size_t batch_iteration;
	GLuint changed_pixels;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_PAINT_COMMAND *)command->payload;

	result = gles_surface_retain(state, command->target, command->sequence,
		&surface);
	if (result != FB_GFX3_OK)
		return result;
	if (!gles_clip_rect(surface, &payload->clip, &clip) ||
	    (payload->x < clip.x1) || (payload->y < clip.y1) ||
	    (payload->x > clip.x2) || (payload->y > clip.y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	if (fb_gfx3_size_multiply(surface->width, surface->height,
	     &pixel_count) != FB_GFX3_OK) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	result = gles_paint_texture_create(state, surface->width, surface->height,
		&mask_a);
	if (result != FB_GFX3_OK)
		goto done;
	result = gles_paint_texture_create(state, surface->width, surface->height,
		&mask_b);
	if (result != FB_GFX3_OK)
		goto done;
	result = gles_paint_texture_create(state, surface->width, surface->height,
		&result_texture);
	if (result != FB_GFX3_OK)
		goto done;
	result = gles_paint_pattern_texture_create(state, payload, surface->depth,
		&pattern_texture);
	if (result != FB_GFX3_OK)
		goto done;
	glGenQueries(1, &changed_query);
	if (changed_query == 0) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto done;
	}
	result = gles_paint_draw(state, mask_a, surface->texture, mask_b,
		pattern_texture, surface->width, surface->height, 0, payload, &clip,
		surface->depth);
	if (result != FB_GFX3_OK)
		goto done;
	source_mask = mask_a;
	destination_mask = mask_b;
	for (iteration = 0; iteration < pixel_count; ) {
		/*
			Save the batch's initial frontier before expanding it.  The final
			comparison shader discards every pre-existing pixel, so its occlusion
			query precisely reports whether this batch found anything new.  Querying
			the expansion draw itself would always report samples because that draw
			writes every texel in its output texture.
		*/
		result = gles_paint_texture_copy(state, result_texture, source_mask,
			surface->width, surface->height);
		if (result != FB_GFX3_OK)
			goto done;
		for (batch_iteration = 0;
		     (batch_iteration < FB_GFX3_GLES_PAINT_QUERY_BATCH) &&
		     (iteration < pixel_count);
		     batch_iteration++, iteration++) {
			result = gles_paint_draw(state, destination_mask,
				surface->texture, source_mask, pattern_texture, surface->width,
				surface->height, 1, payload, &clip, surface->depth);
			if (result != FB_GFX3_OK)
				break;
			swap_texture = source_mask;
			source_mask = destination_mask;
			destination_mask = swap_texture;
		}
		if (result != FB_GFX3_OK)
			goto done;
		glBeginQuery(GL_ANY_SAMPLES_PASSED, changed_query);
		result = gles_paint_draw(state, destination_mask, result_texture,
			source_mask, pattern_texture, surface->width, surface->height, 3,
			payload, &clip, surface->depth);
		glEndQuery(GL_ANY_SAMPLES_PASSED);
		if (result != FB_GFX3_OK)
			goto done;
		glGetQueryObjectuiv(changed_query, GL_QUERY_RESULT,
			&changed_pixels);
		if (changed_pixels == 0)
			break;
	}
	result = gles_paint_draw(state, result_texture, surface->texture,
		source_mask, pattern_texture, surface->width, surface->height, 2,
		payload, &clip,
		surface->depth);
	if (result != FB_GFX3_OK)
		goto done;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, state->read_framebuffer);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, result_texture, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, state->draw_framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, surface->texture, 0);
	if ((glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) !=
	     GL_FRAMEBUFFER_COMPLETE) ||
	    (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) !=
	     GL_FRAMEBUFFER_COMPLETE)) {
		result = FB_GFX3_UNSUPPORTED;
		goto done;
	}
	glBlitFramebuffer(0, 0, (GLint)surface->width, (GLint)surface->height,
		0, 0, (GLint)surface->width, (GLint)surface->height,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	result = gles_check_error(state, "PAINT ping-pong fragment passes");

done:
	if (changed_query != 0)
		glDeleteQueries(1, &changed_query);
	if (pattern_texture != 0)
		glDeleteTextures(1, &pattern_texture);
	if (result_texture != 0)
		glDeleteTextures(1, &result_texture);
	if (mask_b != 0)
		glDeleteTextures(1, &mask_b);
	if (mask_a != 0)
		glDeleteTextures(1, &mask_a);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

/*
	An adjacent run of compatible opaque solid PAINT commands has one observable
	result and an unchanged non-border topology. Retaining the final recolour is
	particularly important on GLES, where each flood expansion is a series of
	GPU mask passes. Border-coloured, patterned, and alpha fills remain ordered
	one at a time because they do not satisfy that equivalence.
*/
static size_t gles_paint_batch_count(FB_GFX3_COMMAND *const *commands,
	size_t available)
{
	const FB_GFX3_PAINT_COMMAND *first;
	size_t count = 1u;

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

static int gles_blit(FB_GFX3_GLES_STATE *state, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_BLIT_COMMAND *payload;
	FB_GFX3_GLES_SURFACE *destination;
	FB_GFX3_GLES_SURFACE *source;
	FB_GFX3_RECT clip;
	FB_GFX3_RECT area;
	GLuint source_copy = 0;
	GLuint destination_copy = 0;
	uint32_t width;
	uint32_t height;
	uint32_t copy_width;
	uint32_t copy_height;
	int copy_x;
	int copy_y;
	int source_x;
	int source_y;
	GLuint source_texture;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_BLIT_COMMAND *)command->payload;
	if (payload->mode == FB_GFX3_BLIT_CUSTOM)
		return FB_GFX3_UNSUPPORTED;
	if (payload->mode > FB_GFX3_BLIT_BLEND)
		return FB_GFX3_INVALID;
	result = gles_surface_retain(state, command->target, command->sequence,
		&destination);
	if (result != FB_GFX3_OK)
		return result;
	result = gles_surface_retain(state, payload->source, command->sequence,
		&source);
	if (result != FB_GFX3_OK) {
		fb_gfx3_resource_release(state->resources, command->target);
		return result;
	}
	if ((source->depth != destination->depth) ||
	    (payload->source_rect.x1 < 0) || (payload->source_rect.y1 < 0) ||
	    (payload->source_rect.x1 > payload->source_rect.x2) ||
	    (payload->source_rect.y1 > payload->source_rect.y2) ||
	    (payload->source_rect.x2 >= (int32_t)source->width) ||
	    (payload->source_rect.y2 >= (int32_t)source->height) ||
	    !gles_clip_rect(destination, &payload->clip, &clip)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	width = (uint32_t)(payload->source_rect.x2 - payload->source_rect.x1 + 1);
	height = (uint32_t)(payload->source_rect.y2 - payload->source_rect.y1 + 1);
	area.x1 = payload->destination_x;
	area.y1 = payload->destination_y;
	area.x2 = payload->destination_x + (int32_t)width - 1;
	area.y2 = payload->destination_y + (int32_t)height - 1;
	if ((area.x2 < clip.x1) || (area.y2 < clip.y1) ||
	    (area.x1 > clip.x2) || (area.y1 > clip.y2)) {
		result = FB_GFX3_OK;
		goto done;
	}
	/*
		A fully visible same-depth PSET between distinct surfaces is an exact GPU
		framebuffer copy. This is the normal SCREENCOPY page-flip case. Avoid
		the two safety snapshots and fragment shader when no clipping, colour
		mask, or overlap rule needs them; all other transfers retain the
		general path below.
	*/
	if ((payload->mode == FB_GFX3_BLIT_PSET) && (source != destination) &&
	    (source->depth == destination->depth) &&
	    (area.x1 >= clip.x1) && (area.y1 >= clip.y1) &&
	    (area.x2 <= clip.x2) && (area.y2 <= clip.y2)) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, state->read_framebuffer);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, source->texture, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, state->draw_framebuffer);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, destination->texture, 0);
		if ((glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) !=
		     GL_FRAMEBUFFER_COMPLETE) ||
		    (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) !=
		     GL_FRAMEBUFFER_COMPLETE)) {
			result = FB_GFX3_UNSUPPORTED;
			goto done;
		}
		glBlitFramebuffer(payload->source_rect.x1, payload->source_rect.y1,
			payload->source_rect.x1 + (GLint)width,
			payload->source_rect.y1 + (GLint)height,
			payload->destination_x, payload->destination_y,
			payload->destination_x + (GLint)width,
			payload->destination_y + (GLint)height,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
		result = gles_check_error(state, "surface PSET framebuffer copy");
		goto done;
	}
	copy_x = (area.x1 < clip.x1) ? clip.x1 : area.x1;
	copy_y = (area.y1 < clip.y1) ? clip.y1 : area.y1;
	copy_width = (uint32_t)(((area.x2 > clip.x2) ? clip.x2 : area.x2) -
		copy_x + 1);
	copy_height = (uint32_t)(((area.y2 > clip.y2) ? clip.y2 : area.y2) -
		copy_y + 1);
	source_x = payload->source_rect.x1 + copy_x - payload->destination_x;
	source_y = payload->source_rect.y1 + copy_y - payload->destination_y;
	/*
		Sampling a different GPU surface while writing the destination surface
		is legal in GLES.  Keep the source coordinates in the shader and avoid
		making a temporary texture for the common sprite-to-screen case.  A
		self PUT is feedback, so it still needs an immutable source snapshot.
	*/
	if (source == destination) {
		result = gles_copy_texture_region(state, source,
			source_x, source_y, copy_width, copy_height,
			&source_copy);
		if (result != FB_GFX3_OK)
			goto done;
		source_texture = source_copy;
		source_x = 0;
		source_y = 0;
	} else {
		source_texture = source->texture;
	}
	result = gles_copy_texture_region(state, destination,
		copy_x, copy_y, copy_width, copy_height,
		&destination_copy);
	if (result != FB_GFX3_OK)
		goto done;
	result = gles_attach_surface(state, destination, GL_FRAMEBUFFER);
	if (result != FB_GFX3_OK)
		goto done;
	glViewport(0, 0, (GLsizei)destination->width,
		(GLsizei)destination->height);
	glEnable(GL_SCISSOR_TEST);
	glScissor(copy_x, copy_y, (GLsizei)copy_width, (GLsizei)copy_height);
	/* Integer PUT modes write their calculated native pixel exactly. */
	glDisable(GL_BLEND);
	glUseProgram(state->blit_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, source_texture);
	glUniform1i(state->blit_source_image_location, 0);
	glUniform2i(state->blit_source_origin_location, source_x, source_y);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, destination_copy);
	glUniform1i(state->blit_destination_image_location, 1);
	glUniform2i(state->blit_destination_location, copy_x, copy_y);
	glUniform2i(state->blit_size_location,
		(int)copy_width, (int)copy_height);
	glUniform1ui(state->blit_mode_location, payload->mode);
	glUniform1ui(state->blit_alpha_location, payload->alpha);
	glUniform1ui(state->blit_depth_location, destination->depth);
	glUniform1ui(state->blit_mask_location, gles_color_mask(destination->depth));
	glBindVertexArray(state->vertex_array);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDisable(GL_SCISSOR_TEST);
	result = gles_check_error(state, "GPU surface blit");

done:
	if (destination_copy != 0)
		glDeleteTextures(1, &destination_copy);
	if (source_copy != 0)
		glDeleteTextures(1, &source_copy);
	fb_gfx3_resource_release(state->resources, payload->source);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int gles_transform_blit(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_TRANSFORM_BLIT_COMMAND *payload;
	FB_GFX3_GLES_SURFACE *destination = NULL;
	FB_GFX3_GLES_SURFACE *source = NULL;
	FB_GFX3_RECT clip;
	FB_GFX3_RECT bounds;
	GLuint source_copy = 0;
	GLuint destination_copy = 0;
	GLuint source_texture;
	GLuint destination_texture;
	GLfloat inverse[9];
	uint32_t width;
	uint32_t height;
	int result;

	if ((state == NULL) || (command == NULL) ||
	    (fb_gfx3_command_payload_size(command) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_TRANSFORM_BLIT_COMMAND *)command->payload;
	if ((payload->mode == FB_GFX3_BLIT_CUSTOM) ||
	    (payload->mode > FB_GFX3_BLIT_BLEND) ||
	    (payload->filter > FB_GFX3_TRANSFORM_FILTER_LINEAR) ||
	    (payload->wrap > FB_GFX3_TRANSFORM_WRAP_REPEAT))
		return FB_GFX3_INVALID;
	result = gles_surface_retain(state, command->target, command->sequence,
		&destination);
	if (result != FB_GFX3_OK)
		return result;
	result = gles_surface_retain(state, payload->source, command->sequence,
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
	if (!gles_clip_rect(destination, &payload->clip, &clip)) {
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

	/* A self-transform would otherwise sample the active draw attachment. */
	if (source == destination) {
		result = gles_copy_texture_region(state, source, 0, 0,
			source->width, source->height, &source_copy);
		if (result != FB_GFX3_OK)
			goto done;
		source_texture = source_copy;
	} else {
		source_texture = source->texture;
	}
	/* TRANS, PSET, and PRESET never read the old destination value. */
	destination_texture = source_texture;
	if (payload->mode > FB_GFX3_BLIT_PRESET) {
		result = gles_copy_texture_region(state, destination, bounds.x1,
			bounds.y1, width, height, &destination_copy);
		if (result != FB_GFX3_OK)
			goto done;
		destination_texture = destination_copy;
	}
	result = gles_attach_surface(state, destination, GL_FRAMEBUFFER);
	if (result != FB_GFX3_OK)
		goto done;

	/* GLES requires column-major input and does not accept transpose = TRUE. */
	inverse[0] = payload->inverse[0];
	inverse[1] = payload->inverse[3];
	inverse[2] = payload->inverse[6];
	inverse[3] = payload->inverse[1];
	inverse[4] = payload->inverse[4];
	inverse[5] = payload->inverse[7];
	inverse[6] = payload->inverse[2];
	inverse[7] = payload->inverse[5];
	inverse[8] = payload->inverse[8];
	glViewport(0, 0, (GLsizei)destination->width,
		(GLsizei)destination->height);
	glEnable(GL_SCISSOR_TEST);
	glScissor(bounds.x1, bounds.y1, (GLsizei)width, (GLsizei)height);
	glDisable(GL_BLEND);
	glUseProgram(state->transform_blit_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, source_texture);
	glUniform1i(state->transform_blit_source_image_location, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, destination_texture);
	glUniform1i(state->transform_blit_destination_image_location, 1);
	glUniform4i(state->transform_blit_source_rect_location,
		payload->source_rect.x1, payload->source_rect.y1,
		payload->source_rect.x2, payload->source_rect.y2);
	glUniform4i(state->transform_blit_bounds_location,
		bounds.x1, bounds.y1, bounds.x2, bounds.y2);
	glUniformMatrix3fv(state->transform_blit_inverse_location, 1, GL_FALSE,
		inverse);
	glUniform1ui(state->transform_blit_mode_location, payload->mode);
	glUniform1ui(state->transform_blit_alpha_location, payload->alpha);
	glUniform1ui(state->transform_blit_depth_location, destination->depth);
	glUniform1ui(state->transform_blit_mask_location,
		gles_color_mask(destination->depth));
	glUniform1ui(state->transform_blit_filter_location, payload->filter);
	glUniform1ui(state->transform_blit_wrap_location, payload->wrap);
	glBindVertexArray(state->vertex_array);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDisable(GL_SCISSOR_TEST);
	result = gles_check_error(state, "GPU surface transform blit");

done:
	if (destination_copy != 0)
		glDeleteTextures(1, &destination_copy);
	if (source_copy != 0)
		glDeleteTextures(1, &source_copy);
	if (source != NULL)
		fb_gfx3_resource_release(state->resources, payload->source);
	if (destination != NULL)
		fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static uint32_t gles_blit_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available)
{
	const FB_GFX3_BLIT_COMMAND *first;
	uint32_t index;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_BLIT) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1u;
	first = (const FB_GFX3_BLIT_COMMAND *)commands[0]->payload;
	if (((first->mode != FB_GFX3_BLIT_TRANS) &&
	     (first->mode != FB_GFX3_BLIT_PSET) &&
	     (first->mode != FB_GFX3_BLIT_PRESET)) ||
	    (first->source == commands[0]->target))
		return 1u;
	if (available > FB_GFX3_GLES_BLIT_BATCH_LIMIT)
		available = FB_GFX3_GLES_BLIT_BATCH_LIMIT;
	for (index = 1u; index < available; index++) {
		const FB_GFX3_BLIT_COMMAND *candidate;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_BLIT) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_BLIT_COMMAND *)commands[index]->payload;
		if ((commands[index]->target != commands[0]->target) ||
		    (candidate->source != first->source) ||
		    (candidate->mode != first->mode) ||
		    (memcmp(&candidate->clip, &first->clip, sizeof(first->clip)) != 0))
			break;
	}
	return index;
}

static uint32_t gles_transform_blit_batch_count(
	FB_GFX3_COMMAND *const *commands, uint32_t available)
{
	const FB_GFX3_TRANSFORM_BLIT_COMMAND *first;
	uint32_t index;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_TRANSFORM_BLIT) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1u;
	first = (const FB_GFX3_TRANSFORM_BLIT_COMMAND *)commands[0]->payload;
	if ((first->source == commands[0]->target) ||
	    (first->mode > FB_GFX3_BLIT_PRESET))
		return 1u;
	if (available > FB_GFX3_GLES_BLIT_BATCH_LIMIT)
		available = FB_GFX3_GLES_BLIT_BATCH_LIMIT;
	for (index = 1u; index < available; ++index) {
		const FB_GFX3_TRANSFORM_BLIT_COMMAND *candidate;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_TRANSFORM_BLIT) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_TRANSFORM_BLIT_COMMAND *)
			commands[index]->payload;
		if ((commands[index]->target != commands[0]->target) ||
		    (candidate->source != first->source) ||
		    (candidate->mode > FB_GFX3_BLIT_PRESET))
			break;
	}
	return index;
}

static uint32_t gles_palette_batch_count(FB_GFX3_COMMAND *const *commands,
	uint32_t available)
{
	uint32_t count = 1u;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_PALETTE) ||
	    (fb_gfx3_command_payload_size(commands[0]) !=
	     sizeof(FB_GFX3_PALETTE_COMMAND)))
		return 1u;
	while ((count < available) && (commands[count] != NULL) &&
	       (commands[count]->type == FB_GFX3_COMMAND_PALETTE) &&
	       (fb_gfx3_command_payload_size(commands[count]) ==
	        sizeof(FB_GFX3_PALETTE_COMMAND)))
		count++;
	return count;
}

/*
	PSET, PRESET, and TRANS from a distinct source do not read the destination.
	GLES rasterization guarantees primitive submission order, so instanced quads
	have the same overlapping-sprite last-writer semantics as individual PUT
	calls without taking a source or destination snapshot for every sprite.
*/
static int gles_blit_batch_render(FB_GFX3_GLES_STATE *state,
	FB_GFX3_GLES_SURFACE *destination, FB_GFX3_GLES_SURFACE *source,
	const FB_GFX3_RECT *clip, const FB_GFX3_GLES_BLIT_BATCH_ITEM *items,
	uint32_t count, uint32_t mode)
{
	GLuint program;
	GLint source_location;
	GLint size_location;
	uint32_t trans_index = 0u;
	int specialized_trans = FALSE;
	int result;

	if ((state == NULL) || (destination == NULL) || (source == NULL) ||
	    (clip == NULL) || (items == NULL) || (count == 0u) ||
	    (count > FB_GFX3_GLES_BLIT_BATCH_LIMIT) ||
	    (mode > FB_GFX3_BLIT_PRESET))
		return FB_GFX3_INVALID;
	result = gles_attach_surface(state, destination, GL_FRAMEBUFFER);
	if (result != FB_GFX3_OK)
		return result;
	glViewport(0, 0, (GLsizei)destination->width, (GLsizei)destination->height);
	glEnable(GL_SCISSOR_TEST);
	glScissor(clip->x1, clip->y1, (GLsizei)(clip->x2 - clip->x1 + 1),
		(GLsizei)(clip->y2 - clip->y1 + 1));
	/* A preceding presentation or extension callback must not leak blend state. */
	glDisable(GL_BLEND);
	program = state->blit_batch_program;
	source_location = state->blit_batch_source_location;
	size_location = state->blit_batch_size_location;
	if (mode == FB_GFX3_BLIT_TRANS) {
		if (destination->depth == 16u)
			trans_index = 1u;
		else if (destination->depth >= 32u)
			trans_index = 2u;
		program = state->blit_batch_trans_program[trans_index];
		source_location = state->blit_batch_trans_source_location[trans_index];
		size_location = state->blit_batch_trans_size_location[trans_index];
		specialized_trans = TRUE;
	}
	glUseProgram(program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, source->texture);
	glUniform1i(source_location, 0);
	glUniform2f(size_location, (GLfloat)destination->width,
		(GLfloat)destination->height);
	if (!specialized_trans) {
		glUniform1ui(state->blit_batch_mode_location, mode);
		glUniform1ui(state->blit_batch_depth_location, destination->depth);
		glUniform1ui(state->blit_batch_mask_location,
			gles_color_mask(destination->depth));
	}
	glBindVertexArray(state->vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(count * sizeof(items[0])),
		items, GL_STREAM_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 4, GL_INT, sizeof(items[0]), NULL);
	glVertexAttribDivisor(1, 1);
	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 2, GL_INT, sizeof(items[0]),
		(const void *)offsetof(FB_GFX3_GLES_BLIT_BATCH_ITEM, destination_x));
	glVertexAttribDivisor(2, 1);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)count);
	glVertexAttribDivisor(1, 0);
	glVertexAttribDivisor(2, 0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisable(GL_SCISSOR_TEST);
	return gles_check_error(state, "ordered GLES blit batch");
}

static int gles_blit_batch(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count)
{
	FB_GFX3_GLES_BLIT_BATCH_ITEM items[FB_GFX3_GLES_BLIT_BATCH_LIMIT];
	const FB_GFX3_BLIT_COMMAND *first;
	FB_GFX3_GLES_SURFACE *destination = NULL;
	FB_GFX3_GLES_SURFACE *source = NULL;
	FB_GFX3_RECT clip;
	uint32_t index;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_GLES_BLIT_BATCH_LIMIT) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return FB_GFX3_INVALID;
	first = (const FB_GFX3_BLIT_COMMAND *)commands[0]->payload;
	result = gles_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = gles_surface_retain(state, first->source,
		commands[count - 1u]->sequence, &source);
	if (result != FB_GFX3_OK)
		goto done;
	if ((source == destination) || (source->depth != destination->depth) ||
	    !gles_clip_rect(destination, &first->clip, &clip)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	for (index = 0u; index < count; index++) {
		const FB_GFX3_BLIT_COMMAND *payload;
		int64_t right;
		int64_t bottom;

		if ((commands[index] == NULL) ||
		    ((index > 0u) &&
		     (commands[index]->sequence <= commands[index - 1u]->sequence)) ||
		    (fb_gfx3_command_payload_size(commands[index]) != sizeof(*payload))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		payload = (const FB_GFX3_BLIT_COMMAND *)commands[index]->payload;
		if ((commands[index]->target != commands[0]->target) ||
		    (payload->source != first->source) ||
		    (payload->mode != first->mode) ||
		    (memcmp(&payload->clip, &first->clip, sizeof(first->clip)) != 0) ||
		    (payload->source_rect.x1 < 0) || (payload->source_rect.y1 < 0) ||
		    (payload->source_rect.x1 > payload->source_rect.x2) ||
		    (payload->source_rect.y1 > payload->source_rect.y2) ||
		    (payload->source_rect.x2 >= (int32_t)source->width) ||
		    (payload->source_rect.y2 >= (int32_t)source->height)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		items[index].source_x = payload->source_rect.x1;
		items[index].source_y = payload->source_rect.y1;
		items[index].width = payload->source_rect.x2 - payload->source_rect.x1 + 1;
		items[index].height = payload->source_rect.y2 - payload->source_rect.y1 + 1;
		right = (int64_t)payload->destination_x + items[index].width;
		bottom = (int64_t)payload->destination_y + items[index].height;
		if ((right < INT32_MIN) || (right > INT32_MAX) ||
		    (bottom < INT32_MIN) || (bottom > INT32_MAX)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		items[index].destination_x = payload->destination_x;
		items[index].destination_y = payload->destination_y;
	}
	result = gles_blit_batch_render(state, destination, source, &clip, items,
		count, first->mode);

done:
	if (source != NULL)
		fb_gfx3_resource_release(state->resources, first->source);
	fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

/*
	A producer BLITS packet removes one heap allocation and one Win32/Android
	mutex round trip from every public PUT. Clips are split into consecutive runs
	because ES 3.0 exposes one scissor rectangle for an instanced draw.
*/
static int gles_blits(FB_GFX3_GLES_STATE *state, FB_GFX3_COMMAND *command)
{
	FB_GFX3_GLES_BLIT_BATCH_ITEM items[FB_GFX3_GLES_BLIT_BATCH_LIMIT];
	const FB_GFX3_BLITS_COMMAND *payload;
	FB_GFX3_GLES_SURFACE *destination = NULL;
	FB_GFX3_GLES_SURFACE *source = NULL;
	FB_GFX3_RECT run_clip;
	size_t blit_bytes;
	size_t expected_size;
	uint32_t index = 0u;
	int result;

	if ((state == NULL) || (command == NULL) ||
	    (fb_gfx3_command_payload_size(command) <
	     offsetof(FB_GFX3_BLITS_COMMAND, blit)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_BLITS_COMMAND *)command->payload;
	if ((payload->count == 0u) ||
	    (payload->count > FB_GFX3_GLES_BLIT_BATCH_LIMIT) ||
	    (payload->source == command->target) ||
	    (payload->mode > FB_GFX3_BLIT_PRESET) ||
	    (fb_gfx3_size_multiply(payload->count, sizeof(payload->blit[0]),
	     &blit_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_BLITS_COMMAND, blit), blit_bytes,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)))
		return FB_GFX3_INVALID;
	result = gles_surface_retain(state, command->target, command->sequence,
		&destination);
	if (result != FB_GFX3_OK)
		return result;
	result = gles_surface_retain(state, payload->source, command->sequence,
		&source);
	if (result != FB_GFX3_OK)
		goto done;
	if ((source == destination) || (source->depth != destination->depth)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	while (index < payload->count) {
		uint32_t item_count = 0u;

		while (index < payload->count) {
			const FB_GFX3_BLIT_COMMAND *blit = &payload->blit[index];
			FB_GFX3_RECT clip;
			int64_t right;
			int64_t bottom;

			if ((blit->source != payload->source) ||
			    (blit->mode != payload->mode) ||
			    (blit->alpha != payload->alpha) ||
			    (blit->source_rect.x1 < 0) ||
			    (blit->source_rect.y1 < 0) ||
			    (blit->source_rect.x1 > blit->source_rect.x2) ||
			    (blit->source_rect.y1 > blit->source_rect.y2) ||
			    (blit->source_rect.x2 >= (int32_t)source->width) ||
			    (blit->source_rect.y2 >= (int32_t)source->height)) {
				result = FB_GFX3_INVALID;
				goto done;
			}
			if (!gles_clip_rect(destination, &blit->clip, &clip)) {
				index++;
				continue;
			}
			if ((item_count != 0u) &&
			    (memcmp(&run_clip, &clip, sizeof(clip)) != 0))
				break;
			right = (int64_t)blit->destination_x +
				blit->source_rect.x2 - blit->source_rect.x1 + 1;
			bottom = (int64_t)blit->destination_y +
				blit->source_rect.y2 - blit->source_rect.y1 + 1;
			if ((right < INT32_MIN) || (right > INT32_MAX) ||
			    (bottom < INT32_MIN) || (bottom > INT32_MAX)) {
				result = FB_GFX3_INVALID;
				goto done;
			}
			if (item_count == 0u)
				run_clip = clip;
			items[item_count].source_x = blit->source_rect.x1;
			items[item_count].source_y = blit->source_rect.y1;
			items[item_count].width = blit->source_rect.x2 -
				blit->source_rect.x1 + 1;
			items[item_count].height = blit->source_rect.y2 -
				blit->source_rect.y1 + 1;
			items[item_count].destination_x = blit->destination_x;
			items[item_count].destination_y = blit->destination_y;
			item_count++;
			index++;
		}
		if (item_count != 0u) {
			result = gles_blit_batch_render(state, destination, source,
				&run_clip, items, item_count, payload->mode);
			if (result != FB_GFX3_OK)
				goto done;
		}
	}
	result = FB_GFX3_OK;

done:
	if (source != NULL)
		fb_gfx3_resource_release(state->resources, payload->source);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int gles_transform_blit_batch(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *const *commands, uint32_t count)
{
	FB_GFX3_GLES_TRANSFORM_BATCH_ITEM items[
		FB_GFX3_GLES_BLIT_BATCH_LIMIT];
	const FB_GFX3_TRANSFORM_BLIT_COMMAND *first;
	FB_GFX3_GLES_SURFACE *destination = NULL;
	FB_GFX3_GLES_SURFACE *source = NULL;
	uint32_t item_count = 0u;
	uint32_t index;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_GLES_BLIT_BATCH_LIMIT) ||
	    (commands[0] == NULL) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return FB_GFX3_INVALID;
	first = (const FB_GFX3_TRANSFORM_BLIT_COMMAND *)commands[0]->payload;
	result = gles_surface_retain(state, commands[0]->target,
		commands[count - 1u]->sequence, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = gles_surface_retain(state, first->source,
		commands[count - 1u]->sequence, &source);
	if (result != FB_GFX3_OK)
		goto done;
	if ((source == destination) || (source->depth != destination->depth)) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	for (index = 0u; index < count; ++index) {
		const FB_GFX3_TRANSFORM_BLIT_COMMAND *payload;
		FB_GFX3_RECT clip;
		FB_GFX3_RECT bounds;
		FB_GFX3_GLES_TRANSFORM_BATCH_ITEM *item;

		if ((commands[index] == NULL) ||
		    ((index > 0u) && (commands[index]->sequence <=
		     commands[index - 1u]->sequence)) ||
		    (commands[index]->type != FB_GFX3_COMMAND_TRANSFORM_BLIT) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*payload))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		payload = (const FB_GFX3_TRANSFORM_BLIT_COMMAND *)
			commands[index]->payload;
		if ((commands[index]->target != commands[0]->target) ||
		    (payload->source != first->source) ||
		    (payload->mode > FB_GFX3_BLIT_PRESET) ||
		    (payload->source_rect.x1 < 0) ||
		    (payload->source_rect.y1 < 0) ||
		    (payload->source_rect.x1 > payload->source_rect.x2) ||
		    (payload->source_rect.y1 > payload->source_rect.y2) ||
		    (payload->source_rect.x2 >= (int32_t)source->width) ||
		    (payload->source_rect.y2 >= (int32_t)source->height) ||
		    (payload->filter > FB_GFX3_TRANSFORM_FILTER_LINEAR) ||
		    (payload->wrap > FB_GFX3_TRANSFORM_WRAP_REPEAT)) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (!gles_clip_rect(destination, &payload->clip, &clip))
			continue;
		bounds = payload->destination_bounds;
		if (bounds.x1 < clip.x1)
			bounds.x1 = clip.x1;
		if (bounds.y1 < clip.y1)
			bounds.y1 = clip.y1;
		if (bounds.x2 > clip.x2)
			bounds.x2 = clip.x2;
		if (bounds.y2 > clip.y2)
			bounds.y2 = clip.y2;
		if ((bounds.x1 > bounds.x2) || (bounds.y1 > bounds.y2))
			continue;
		item = &items[item_count++];
		item->source_rect[0] = payload->source_rect.x1;
		item->source_rect[1] = payload->source_rect.y1;
		item->source_rect[2] = payload->source_rect.x2;
		item->source_rect[3] = payload->source_rect.y2;
		item->bounds[0] = bounds.x1;
		item->bounds[1] = bounds.y1;
		item->bounds[2] = bounds.x2;
		item->bounds[3] = bounds.y2;
		memset(item->inverse_row_0, 0, sizeof(item->inverse_row_0));
		memset(item->inverse_row_1, 0, sizeof(item->inverse_row_1));
		memset(item->inverse_row_2, 0, sizeof(item->inverse_row_2));
		memcpy(item->inverse_row_0, payload->inverse,
			3u * sizeof(payload->inverse[0]));
		memcpy(item->inverse_row_1, payload->inverse + 3,
			3u * sizeof(payload->inverse[0]));
		memcpy(item->inverse_row_2, payload->inverse + 6,
			3u * sizeof(payload->inverse[0]));
		item->options[0] = payload->mode;
		item->options[1] = destination->depth;
		item->options[2] = gles_color_mask(destination->depth);
		item->options[3] = payload->filter | (payload->wrap << 16);
	}
	if (item_count == 0u) {
		result = FB_GFX3_OK;
		goto done;
	}
	result = gles_attach_surface(state, destination, GL_FRAMEBUFFER);
	if (result != FB_GFX3_OK)
		goto done;
	glViewport(0, 0, (GLsizei)destination->width,
		(GLsizei)destination->height);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	glUseProgram(state->transform_blit_batch_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, source->texture);
	glUniform1i(state->transform_blit_batch_source_location, 0);
	glUniform2f(state->transform_blit_batch_size_location,
		(GLfloat)destination->width, (GLfloat)destination->height);
	glBindVertexArray(state->vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		(GLsizeiptr)(item_count * sizeof(items[0])), items, GL_STREAM_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 4, GL_INT, sizeof(items[0]),
		(const void *)offsetof(FB_GFX3_GLES_TRANSFORM_BATCH_ITEM,
		 source_rect));
	glVertexAttribDivisor(1, 1);
	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 4, GL_INT, sizeof(items[0]),
		(const void *)offsetof(FB_GFX3_GLES_TRANSFORM_BATCH_ITEM, bounds));
	glVertexAttribDivisor(2, 1);
	for (index = 0u; index < 3u; ++index) {
		size_t offset = offsetof(FB_GFX3_GLES_TRANSFORM_BATCH_ITEM,
			inverse_row_0) + index * sizeof(items[0].inverse_row_0);

		glEnableVertexAttribArray(3u + index);
		glVertexAttribPointer(3u + index, 3, GL_FLOAT, GL_FALSE,
			sizeof(items[0]), (const void *)offset);
		glVertexAttribDivisor(3u + index, 1);
	}
	glEnableVertexAttribArray(6);
	glVertexAttribIPointer(6, 4, GL_UNSIGNED_INT, sizeof(items[0]),
		(const void *)offsetof(FB_GFX3_GLES_TRANSFORM_BATCH_ITEM, options));
	glVertexAttribDivisor(6, 1);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)item_count);
	for (index = 1u; index <= 6u; ++index) {
		glVertexAttribDivisor(index, 0);
		glDisableVertexAttribArray(index);
	}
	result = gles_check_error(state, "ordered GLES transform batch");

done:
	if (source != NULL)
		fb_gfx3_resource_release(state->resources, first->source);
	fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

static int gles_read_pixel(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_READ_PIXEL_COMMAND *payload;
	FB_GFX3_GLES_SURFACE *surface;
	unsigned char packed[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
	uint32_t color = UINT32_MAX;
	int result;

	if ((command->completion == NULL) ||
	    (fb_gfx3_command_payload_size(command) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_READ_PIXEL_COMMAND *)command->payload;
	result = gles_surface_retain(state, command->target, command->sequence,
		&surface);
	if (result != FB_GFX3_OK)
		return result;
	if ((payload->x >= 0) && (payload->y >= 0) &&
	    (payload->x < (int32_t)surface->width) &&
	    (payload->y < (int32_t)surface->height)) {
		result = gles_read_region(state, surface, payload->x, payload->y,
			1, 1, packed);
		if (result != FB_GFX3_OK)
			goto done;
		color = gles_unpack_texture_pixel(packed) &
			gles_color_mask(surface->depth);
	}
	result = fb_gfx3_completion_set_value(command->completion, 0, color);

done:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int gles_palette(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_PALETTE_COMMAND *payload;
	unsigned char packed[256 * 4];
	uint32_t index;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_PALETTE_COMMAND *)command->payload;
	memcpy(state->palette, payload->color, sizeof(state->palette));
	for (index = 0; index < 256; index++) {
		packed[index * 4u] = (unsigned char)payload->color[index];
		packed[index * 4u + 1u] = (unsigned char)(payload->color[index] >> 8);
		packed[index * 4u + 2u] = (unsigned char)(payload->color[index] >> 16);
		packed[index * 4u + 3u] = 0xFF;
	}
	glBindTexture(GL_TEXTURE_2D, state->palette_texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGBA,
		GL_UNSIGNED_BYTE, packed);
	if (state->visible_surface != 0)
		state->presentation_dirty = TRUE;
	return gles_check_error(state, "palette texture update");
}

static int gles_page_set(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_PAGE_SET_COMMAND *payload;
	FB_GFX3_GLES_SURFACE *surface;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_PAGE_SET_COMMAND *)command->payload;
	result = gles_surface_retain(state, command->target, command->sequence,
		&surface);
	if (result != FB_GFX3_OK)
		return result;
	if ((payload->width != surface->width) ||
	    (payload->height != surface->height) ||
	    (payload->depth != surface->depth))
		result = FB_GFX3_INVALID;
	else {
		state->visible_surface = command->target;
		state->presentation_dirty = TRUE;
	}
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static void gles_read_keyboard_overlay(FB_GFX3_GLES_STATE *state,
	FB_GFX3_ANDROID_KEYBOARD_OVERLAY *overlay, int *overlay_state)
{
	memset(overlay, 0, sizeof(*overlay));
	*overlay_state = 0;
	if ((fb_gfx3_platform_keyboard_overlay(state->platform, overlay) !=
	     FB_GFX3_OK) || !overlay->visible)
		return;
	*overlay_state = overlay->pressed ? 3 :
		(overlay->keyboard_visible ? 2 : 1);
}

static int gles_present_handle(FB_GFX3_GLES_STATE *state,
	FB_GFX3_HANDLE handle, uint64_t sequence)
{
	FB_GFX3_GLES_SURFACE *surface;
	FB_GFX3_ANDROID_KEYBOARD_OVERLAY keyboard_overlay;
	uint32_t width;
	uint32_t height;
	int keyboard_state = 0;
	int result;

	result = gles_surface_retain(state, handle, sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	result = state->platform_vtable->client_size(state->platform, &width,
		&height);
	if (result != FB_GFX3_OK)
		goto done;
	gles_read_keyboard_overlay(state, &keyboard_overlay, &keyboard_state);
	state->keyboard_overlay = keyboard_overlay;
	state->keyboard_overlay_state = keyboard_state;
	state->keyboard_overlay_known = TRUE;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	glDisable(GL_SCISSOR_TEST);
	glUseProgram(state->present_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, surface->texture);
	glUniform1i(glGetUniformLocation(state->present_program, "source_image"),
		0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, state->palette_texture);
	glUniform1i(glGetUniformLocation(state->present_program, "palette_image"),
		1);
	glUniform1ui(glGetUniformLocation(state->present_program,
		"operation_depth"), surface->depth);
	glUniform2i(glGetUniformLocation(state->present_program,
		"operation_window_size"), (int)width, (int)height);
	glUniform4i(glGetUniformLocation(state->present_program,
		"keyboard_button_rect"), keyboard_overlay.x0, keyboard_overlay.y0,
		keyboard_overlay.x1, keyboard_overlay.y1);
	glUniform1i(glGetUniformLocation(state->present_program,
		"keyboard_button_state"), keyboard_state);
	glBindVertexArray(state->vertex_array);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	result = gles_check_error(state, "presentation draw");
	if (result == FB_GFX3_OK)
		result = state->platform_vtable->swap_buffers(state->platform);
	if (result == FB_GFX3_OK)
		result = state->platform_vtable->show_window(state->platform);
	if (result == FB_GFX3_OK)
		state->presentation_dirty = FALSE;

done:
	fb_gfx3_resource_release(state->resources, handle);
	state->platform_vtable->pump_events(state->platform);
	return result;
}

static int gles_present(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *command)
{
	if (fb_gfx3_command_payload_size(command) != 0)
		return FB_GFX3_INVALID;
	return gles_present_handle(state, command->target, command->sequence);
}

static int gles_window_title(FB_GFX3_GLES_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_WINDOW_TITLE_COMMAND *payload;
	size_t expected_size;

	if (fb_gfx3_command_payload_size(command) < sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_WINDOW_TITLE_COMMAND *)command->payload;
	if ((fb_gfx3_size_add(sizeof(*payload), payload->length,
	     &expected_size) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(expected_size, 1u, &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)) ||
	    (payload->title[payload->length] != '\0'))
		return FB_GFX3_INVALID;
	return state->platform_vtable->set_window_title(state->platform,
		payload->title);
}

/* ------------------------------------------------------------------------- */
/* Fence tracking                                                            */
/* ------------------------------------------------------------------------- */

static void gles_remove_first_fence(FB_GFX3_GLES_STATE *state)
{
	FB_GFX3_GLES_FENCE *fence = state->first_fence;

	if (fence == NULL)
		return;
	state->first_fence = fence->next;
	if (state->first_fence == NULL)
		state->last_fence = NULL;
	state->completed_sequence = fence->sequence;
	glDeleteSync(fence->sync);
	free(fence);
}

static int gles_finish_fence_fallback(FB_GFX3_GLES_STATE *state,
	uint64_t sequence, const char *reason, GLenum wait_result)
{
	if (!state->synchronous_fence_fallback) {
		fb_gfx3_log_write(state->logger, FB_GFX3_LOG_WARNING,
			"OpenGL ES fence fallback enabled after %s (0x%04X)",
			reason, (unsigned int)wait_result);
		state->synchronous_fence_fallback = TRUE;
	}
	/*
	    Some early GLES 3 Adreno drivers expose sync objects but fail their
	    first client wait.  Drain the sync-related error and retain the same
	    completion guarantee through glFinish.  Subsequent submissions avoid
	    the defective path entirely.
	*/
	while (glGetError() != GL_NO_ERROR) {
	}
	glFinish();
	while (state->first_fence != NULL)
		gles_remove_first_fence(state);
	state->completed_sequence = sequence;
	return gles_check_error(state, "synchronous fence fallback");
}

static int gles_record_fence(FB_GFX3_GLES_STATE *state, uint64_t sequence)
{
	FB_GFX3_GLES_FENCE *fence;

	if (state->synchronous_fence_fallback)
		return gles_finish_fence_fallback(state, sequence,
			"previous fence failure", GL_WAIT_FAILED);
	fence = (FB_GFX3_GLES_FENCE *)calloc(1, sizeof(*fence));
	if (fence == NULL) {
		glFinish();
		while (state->first_fence != NULL)
			gles_remove_first_fence(state);
		state->completed_sequence = sequence;
		return FB_GFX3_OK;
	}
	fence->sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	if (fence->sync == NULL) {
		free(fence);
		return gles_finish_fence_fallback(state, sequence,
			"glFenceSync", GL_WAIT_FAILED);
	}
	fence->sequence = sequence;
	if (state->last_fence != NULL)
		state->last_fence->next = fence;
	else
		state->first_fence = fence;
	state->last_fence = fence;
	/*
		A fence orders commands inside this context, but creating one does not
		require the GLES implementation to submit its buffered command stream.
		Desktop drivers commonly flush full internal buffers on their own.  Older
		mobile drivers may retain an entire off-screen sprite stream until the
		first client wait supplies GL_SYNC_FLUSH_COMMANDS_BIT.  That makes a
		dedicated renderer thread look asynchronous while the GPU does no useful
		work until POINT or another readback stalls the BASIC thread.

		glFlush is the nonblocking operation intended for this boundary.  It hands
		all work through this fence to the driver without waiting for completion,
		so the GPU can run while the application prepares subsequent commands.
	*/
	glFlush();
	return gles_check_error(state, "flush asynchronous submission");
}

static uint64_t gles_poll_completed(FB_GFX3_GLES_STATE *state)
{
	GLenum wait_result;

	while (state->first_fence != NULL) {
		wait_result = glClientWaitSync(state->first_fence->sync, 0, 0);
		if ((wait_result != GL_ALREADY_SIGNALED) &&
		    (wait_result != GL_CONDITION_SATISFIED))
			break;
		gles_remove_first_fence(state);
	}
	return state->completed_sequence;
}

static int gles_wait_for_sequence(FB_GFX3_GLES_STATE *state,
	uint64_t sequence)
{
	GLenum wait_result;

	if (sequence > state->submitted_sequence)
		return FB_GFX3_INVALID;
	while (state->completed_sequence < sequence) {
		if (state->first_fence == NULL)
			return gles_finish_fence_fallback(state, sequence,
				"missing fence", GL_WAIT_FAILED);
		wait_result = glClientWaitSync(state->first_fence->sync,
			GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
		if ((wait_result != GL_ALREADY_SIGNALED) &&
		    (wait_result != GL_CONDITION_SATISFIED))
			return gles_finish_fence_fallback(state, sequence,
				"glClientWaitSync", wait_result);
		gles_remove_first_fence(state);
	}
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Backend interface                                                         */
/* ------------------------------------------------------------------------- */

static int gles_probe(FB_GFX3_BACKEND_CAPS *caps)
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
		FB_GFX3_FEATURE_TIMELINE_FENCES |
		FB_GFX3_FEATURE_PRESENT_IMMEDIATE | FB_GFX3_FEATURE_PACKED_BLITS;
	caps->max_surface_width = 4096;
	caps->max_surface_height = 4096;
	caps->max_batch_commands = 4096;
	caps->max_packed_blits = FB_GFX3_GLES_BLIT_BATCH_LIMIT;
	return FB_GFX3_OK;
}

static uint32_t gles_get_nonnegative_integer(GLenum name)
{
	GLint value = 0;

	glGetIntegerv(name, &value);
	return (value > 0) ? (uint32_t)value : 0u;
}

static void gles_capture_gl_info(FB_GFX3_BACKEND *backend)
{
	FB_GFX3_BACKEND_GL_INFO *info = &backend->gl_info;
	const GLubyte *extensions;
	size_t length;

	memset(info, 0, sizeof(*info));
	info->available = TRUE;
	info->color_red_bits = gles_get_nonnegative_integer(GL_RED_BITS);
	info->color_green_bits = gles_get_nonnegative_integer(GL_GREEN_BITS);
	info->color_blue_bits = gles_get_nonnegative_integer(GL_BLUE_BITS);
	info->color_alpha_bits = gles_get_nonnegative_integer(GL_ALPHA_BITS);
	info->color_bits = info->color_red_bits + info->color_green_bits +
		info->color_blue_bits + info->color_alpha_bits;
	info->depth_bits = gles_get_nonnegative_integer(GL_DEPTH_BITS);
	info->stencil_bits = gles_get_nonnegative_integer(GL_STENCIL_BITS);
	info->samples = gles_get_nonnegative_integer(GL_SAMPLES);
	extensions = glGetString(GL_EXTENSIONS);
	if (extensions == NULL)
		return;
	length = strlen((const char *)extensions);
	if (length >= sizeof(info->extensions))
		length = sizeof(info->extensions) - 1u;
	memcpy(info->extensions, extensions, length);
	info->extensions[length] = '\0';
}

static int gles_init(FB_GFX3_BACKEND *backend,
	const FB_GFX3_BACKEND_CONFIG *config)
{
	static const char *const trans_fragment_shader[3] = {
		gles_blit_batch_trans8_fragment_shader,
		gles_blit_batch_trans16_fragment_shader,
		gles_blit_batch_trans32_fragment_shader
	};
	static const char *const trans_shader_name[3] = {
		"GLES 8-bit transparent blit batch shader",
		"GLES RGB565 transparent blit batch shader",
		"GLES 32-bit transparent blit batch shader"
	};
	static const GLfloat fullscreen_triangle[6] = {
		-1.0f, -1.0f,
		 3.0f, -1.0f,
		-1.0f,  3.0f
	};
	FB_GFX3_GLES_STATE *state;
	FB_GFX3_PLATFORM_OPENGL_CONFIG platform_config;
	const GLubyte *version;
	GLint maximum_texture_size = 0;
	uint32_t shader_index;
	int result;

	if ((backend == NULL) || (config == NULL) ||
	    (config->resources == NULL))
		return FB_GFX3_INVALID;
	state = (FB_GFX3_GLES_STATE *)calloc(1, sizeof(*state));
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
	platform_config.major_version = 3;
	platform_config.minor_version = 0;
	platform_config.flags = config->flags;
	platform_config.title = (config->title != NULL) ? config->title :
		"FreeBASIC gfxlib3 OpenGL ES";
	result = state->platform_vtable->create_opengl(&state->platform,
		&platform_config);
	if (result != FB_GFX3_OK)
		return result;
	version = glGetString(GL_VERSION);
	if ((version == NULL) || (strstr((const char *)version,
	    "OpenGL ES 3.") == NULL))
		return FB_GFX3_UNSUPPORTED;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximum_texture_size);
	if (maximum_texture_size <= 0)
		return FB_GFX3_UNSUPPORTED;
	state->maximum_texture_size = (uint32_t)maximum_texture_size;
	backend->caps.max_surface_width = state->maximum_texture_size;
	backend->caps.max_surface_height = state->maximum_texture_size;
	gles_capture_gl_info(backend);
	result = gles_create_program(state, gles_primitive_fragment_shader,
		"GLES primitive fragment shader", &state->primitive_program);
	if (result != FB_GFX3_OK)
		return result;
	result = gles_create_program(state, gles_paint_fragment_shader,
		"GLES PAINT fragment shader", &state->paint_program);
	if (result != FB_GFX3_OK)
		return result;
	result = gles_create_program(state, gles_blit_fragment_shader,
		"GLES blit fragment shader", &state->blit_program);
	if (result != FB_GFX3_OK)
		return result;
	/*
		Destination-reading PUT modes must retain their snapshot draw, but they
		can arrive hundreds of times per frame. Uniform names are fixed by this
		program, so resolve them once at link time instead of forcing an older
		mobile driver to search the program object for every sprite.
	*/
	state->blit_source_image_location = glGetUniformLocation(
		state->blit_program, "source_image");
	state->blit_source_origin_location = glGetUniformLocation(
		state->blit_program, "operation_source_origin");
	state->blit_destination_image_location = glGetUniformLocation(
		state->blit_program, "destination_image");
	state->blit_destination_location = glGetUniformLocation(
		state->blit_program, "operation_destination");
	state->blit_size_location = glGetUniformLocation(state->blit_program,
		"operation_size");
	state->blit_mode_location = glGetUniformLocation(state->blit_program,
		"operation_mode");
	state->blit_alpha_location = glGetUniformLocation(state->blit_program,
		"operation_alpha");
	state->blit_depth_location = glGetUniformLocation(state->blit_program,
		"operation_depth");
	state->blit_mask_location = glGetUniformLocation(state->blit_program,
		"operation_mask");
	if ((state->blit_source_image_location < 0) ||
	    (state->blit_source_origin_location < 0) ||
	    (state->blit_destination_image_location < 0) ||
	    (state->blit_destination_location < 0) ||
	    (state->blit_size_location < 0) ||
	    (state->blit_mode_location < 0) ||
	    (state->blit_alpha_location < 0) ||
	    (state->blit_depth_location < 0) ||
	    (state->blit_mask_location < 0))
		return FB_GFX3_FAILED;
	result = gles_create_program(state, gles_transform_blit_fragment_shader,
		"GLES transform blit fragment shader",
		&state->transform_blit_program);
	if (result != FB_GFX3_OK)
		return result;
	state->transform_blit_source_image_location = glGetUniformLocation(
		state->transform_blit_program, "source_image");
	state->transform_blit_destination_image_location = glGetUniformLocation(
		state->transform_blit_program, "destination_image");
	state->transform_blit_source_rect_location = glGetUniformLocation(
		state->transform_blit_program, "operation_source_rect");
	state->transform_blit_bounds_location = glGetUniformLocation(
		state->transform_blit_program, "operation_bounds");
	state->transform_blit_inverse_location = glGetUniformLocation(
		state->transform_blit_program, "operation_inverse");
	state->transform_blit_mode_location = glGetUniformLocation(
		state->transform_blit_program, "operation_mode");
	state->transform_blit_alpha_location = glGetUniformLocation(
		state->transform_blit_program, "operation_alpha");
	state->transform_blit_depth_location = glGetUniformLocation(
		state->transform_blit_program, "operation_depth");
	state->transform_blit_mask_location = glGetUniformLocation(
		state->transform_blit_program, "operation_mask");
	state->transform_blit_filter_location = glGetUniformLocation(
		state->transform_blit_program, "operation_filter");
	state->transform_blit_wrap_location = glGetUniformLocation(
		state->transform_blit_program, "operation_wrap");
	if ((state->transform_blit_source_image_location < 0) ||
	    (state->transform_blit_destination_image_location < 0) ||
	    (state->transform_blit_source_rect_location < 0) ||
	    (state->transform_blit_bounds_location < 0) ||
	    (state->transform_blit_inverse_location < 0) ||
	    (state->transform_blit_mode_location < 0) ||
	    (state->transform_blit_alpha_location < 0) ||
	    (state->transform_blit_depth_location < 0) ||
	    (state->transform_blit_mask_location < 0) ||
	    (state->transform_blit_filter_location < 0) ||
	    (state->transform_blit_wrap_location < 0))
		return FB_GFX3_FAILED;
	result = gles_create_program_with_vertex(state,
		gles_transform_blit_batch_vertex_shader,
		gles_transform_blit_batch_fragment_shader,
		"GLES ordered transform batch shader",
		&state->transform_blit_batch_program);
	if (result != FB_GFX3_OK)
		return result;
	state->transform_blit_batch_source_location = glGetUniformLocation(
		state->transform_blit_batch_program, "source_image");
	state->transform_blit_batch_size_location = glGetUniformLocation(
		state->transform_blit_batch_program, "operation_surface_size");
	if ((state->transform_blit_batch_source_location < 0) ||
	    (state->transform_blit_batch_size_location < 0))
		return FB_GFX3_FAILED;
	result = gles_create_program_with_vertex(state,
		gles_blit_batch_vertex_shader, gles_blit_batch_fragment_shader,
		"GLES ordered blit batch shader", &state->blit_batch_program);
	if (result != FB_GFX3_OK)
		return result;
	state->blit_batch_source_location = glGetUniformLocation(
		state->blit_batch_program, "source_image");
	state->blit_batch_size_location = glGetUniformLocation(
		state->blit_batch_program, "operation_surface_size");
	state->blit_batch_mode_location = glGetUniformLocation(
		state->blit_batch_program, "operation_mode");
	state->blit_batch_depth_location = glGetUniformLocation(
		state->blit_batch_program, "operation_depth");
	state->blit_batch_mask_location = glGetUniformLocation(
		state->blit_batch_program, "operation_mask");
	if ((state->blit_batch_source_location < 0) ||
	    (state->blit_batch_size_location < 0) ||
	    (state->blit_batch_mode_location < 0) ||
	    (state->blit_batch_depth_location < 0) ||
	    (state->blit_batch_mask_location < 0))
		return FB_GFX3_FAILED;
	for (shader_index = 0u; shader_index < 3u; shader_index++) {
		result = gles_create_program_with_vertex(state,
			gles_blit_batch_vertex_shader,
			trans_fragment_shader[shader_index],
			trans_shader_name[shader_index],
			&state->blit_batch_trans_program[shader_index]);
		if (result != FB_GFX3_OK)
			return result;
		state->blit_batch_trans_source_location[shader_index] =
			glGetUniformLocation(state->blit_batch_trans_program[shader_index],
				"source_image");
		state->blit_batch_trans_size_location[shader_index] =
			glGetUniformLocation(state->blit_batch_trans_program[shader_index],
				"operation_surface_size");
		if ((state->blit_batch_trans_source_location[shader_index] < 0) ||
		    (state->blit_batch_trans_size_location[shader_index] < 0))
			return FB_GFX3_FAILED;
	}
	result = gles_create_program_with_vertex(state,
		gles_ellipse_span_batch_vertex_shader,
		gles_ellipse_span_batch_fragment_shader,
		"GLES exact ellipse span batch shader",
		&state->ellipse_span_batch_program);
	if (result != FB_GFX3_OK)
		return result;
	state->ellipse_span_batch_size_location = glGetUniformLocation(
		state->ellipse_span_batch_program, "operation_surface_size");
	if (state->ellipse_span_batch_size_location < 0)
		return FB_GFX3_FAILED;
	result = gles_create_program_with_vertex(state,
		gles_rectangle_batch_vertex_shader, gles_rectangle_batch_fragment_shader,
		"GLES opaque rectangle batch shader", &state->rectangle_batch_program);
	if (result != FB_GFX3_OK)
		return result;
	state->rectangle_batch_size_location = glGetUniformLocation(
		state->rectangle_batch_program, "operation_surface_size");
	if (state->rectangle_batch_size_location < 0)
		return FB_GFX3_FAILED;
	result = gles_create_program_with_vertex(state, gles_line_batch_vertex_shader,
		gles_line_batch_fragment_shader, "GLES exact line batch shader",
		&state->line_batch_program);
	if (result != FB_GFX3_OK)
		return result;
	state->line_batch_size_location = glGetUniformLocation(
		state->line_batch_program, "operation_surface_size");
	if (state->line_batch_size_location < 0)
		return FB_GFX3_FAILED;
	result = gles_create_program(state, gles_present_fragment_shader,
		"GLES presentation fragment shader", &state->present_program);
	if (result != FB_GFX3_OK)
		return result;
	result = gles_create_program(state, gles_readback_fragment_shader,
		"GLES normalized readback fragment shader", &state->readback_program);
	if (result != FB_GFX3_OK)
		return result;
	state->readback_source_location = glGetUniformLocation(
		state->readback_program, "source_image");
	state->readback_origin_location = glGetUniformLocation(
		state->readback_program, "source_origin");
	if ((state->readback_source_location < 0) ||
	    (state->readback_origin_location < 0))
		return FB_GFX3_FAILED;
	glGenFramebuffers(1, &state->framebuffer);
	glGenFramebuffers(1, &state->read_framebuffer);
	glGenFramebuffers(1, &state->draw_framebuffer);
	glGenVertexArrays(1, &state->vertex_array);
	glBindVertexArray(state->vertex_array);
	/*
		Adreno 306 validation quirk

		The shaders derive a full-screen triangle from gl_VertexID, which is
		legal in OpenGL ES 3.0. This older Qualcomm driver nevertheless rejects
		a draw when the bound vertex array has no enabled attribute. Keep one
		tiny resident position buffer in attribute zero. Current shaders do not
		consume it, but its valid enabled state satisfies the driver without a
		per-command upload or a CPU-side geometry path.
	*/
	glGenBuffers(1, &state->vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(fullscreen_triangle),
		fullscreen_triangle, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenTextures(1, &state->palette_texture);
	glBindTexture(GL_TEXTURE_2D, state->palette_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 256, 1);
	result = gles_check_error(state, "backend object allocation");
	if (result != FB_GFX3_OK)
		return result;
	fb_gfx3_log_write(state->logger, FB_GFX3_LOG_INFO,
		"OpenGL ES gfxlib3 backend initialized: %s", version);
	return FB_GFX3_OK;
}

static void gles_shutdown(FB_GFX3_BACKEND *backend)
{
	FB_GFX3_GLES_STATE *state;
	uint32_t shader_index;

	if ((backend == NULL) || (backend->state == NULL))
		return;
	state = (FB_GFX3_GLES_STATE *)backend->state;
	while (state->first_fence != NULL)
		gles_remove_first_fence(state);
	if (state->readback_texture != 0)
		glDeleteTextures(1, &state->readback_texture);
	if (state->palette_texture != 0)
		glDeleteTextures(1, &state->palette_texture);
	if (state->vertex_array != 0)
		glDeleteVertexArrays(1, &state->vertex_array);
	if (state->vertex_buffer != 0)
		glDeleteBuffers(1, &state->vertex_buffer);
	if (state->draw_framebuffer != 0)
		glDeleteFramebuffers(1, &state->draw_framebuffer);
	if (state->read_framebuffer != 0)
		glDeleteFramebuffers(1, &state->read_framebuffer);
	if (state->framebuffer != 0)
		glDeleteFramebuffers(1, &state->framebuffer);
	if (state->present_program != 0)
		glDeleteProgram(state->present_program);
	if (state->readback_program != 0)
		glDeleteProgram(state->readback_program);
	if (state->blit_program != 0)
		glDeleteProgram(state->blit_program);
	if (state->transform_blit_program != 0)
		glDeleteProgram(state->transform_blit_program);
	if (state->transform_blit_batch_program != 0)
		glDeleteProgram(state->transform_blit_batch_program);
	if (state->blit_batch_program != 0)
		glDeleteProgram(state->blit_batch_program);
	for (shader_index = 0u; shader_index < 3u; shader_index++) {
		if (state->blit_batch_trans_program[shader_index] != 0u)
			glDeleteProgram(state->blit_batch_trans_program[shader_index]);
	}
	if (state->ellipse_span_batch_program != 0)
		glDeleteProgram(state->ellipse_span_batch_program);
	if (state->rectangle_batch_program != 0)
		glDeleteProgram(state->rectangle_batch_program);
	if (state->line_batch_program != 0)
		glDeleteProgram(state->line_batch_program);
	if (state->primitive_program != 0)
		glDeleteProgram(state->primitive_program);
	if (state->paint_program != 0)
		glDeleteProgram(state->paint_program);
	if ((state->platform_vtable != NULL) &&
	    (state->platform_vtable->destroy != NULL))
		state->platform_vtable->destroy(state->platform);
	free(state);
	backend->state = NULL;
}

static int gles_command_writes_visible(const FB_GFX3_GLES_STATE *state,
	const FB_GFX3_COMMAND *command)
{
	if ((state->visible_surface == 0) ||
	    (command->target != state->visible_surface))
		return FALSE;
	switch (command->type) {
	case FB_GFX3_COMMAND_SURFACE_UPLOAD:
	case FB_GFX3_COMMAND_CLEAR:
	case FB_GFX3_COMMAND_POINTS:
	case FB_GFX3_COMMAND_LINE:
	case FB_GFX3_COMMAND_RECTANGLE:
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

static int gles_command_is_deferred_full_page(
	const FB_GFX3_COMMAND *command)
{
	const FB_GFX3_BLIT_COMMAND *blit;

	if (command == NULL)
		return FALSE;
	if (command->type == FB_GFX3_COMMAND_PAGE_SET)
		return fb_gfx3_command_payload_size(command) ==
			sizeof(FB_GFX3_PAGE_SET_COMMAND);
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
	return (blit->source != command->target) &&
		(blit->mode == FB_GFX3_BLIT_PSET) &&
		(blit->source_rect.x1 == 0) && (blit->source_rect.y1 == 0) &&
		(blit->destination_x == 0) && (blit->destination_y == 0) &&
		(blit->clip.x1 <= 0) && (blit->clip.y1 <= 0);
}

static int gles_execute(FB_GFX3_BACKEND *backend,
	FB_GFX3_COMMAND *const *commands, size_t count,
	uint64_t *submitted_sequence)
{
	FB_GFX3_GLES_STATE *state;
	FB_GFX3_COMMAND *command;
	FB_GFX3_ANDROID_KEYBOARD_OVERLAY keyboard_overlay;
	uint64_t last_sequence;
	size_t index;
	size_t execute_count;
	int defer_page_presentation = TRUE;
	int keyboard_state;
	int result;

	if ((backend == NULL) || (backend->state == NULL) ||
	    (commands == NULL) || (count == 0))
		return FB_GFX3_INVALID;
	state = (FB_GFX3_GLES_STATE *)backend->state;
	fb_gfx3_log_write(state->logger, FB_GFX3_LOG_TRACE,
		"GLES drain begin: %u commands, types %u through %u",
		(unsigned int)count,
		(commands[0] != NULL) ? (unsigned int)commands[0]->type : 0u,
		(commands[count - 1u] != NULL) ?
			(unsigned int)commands[count - 1u]->type : 0u);
	state->platform_vtable->pump_events(state->platform);
	for (index = 0u; index < count; index++) {
		if (!gles_command_is_deferred_full_page(commands[index])) {
			defer_page_presentation = FALSE;
			break;
		}
	}
	last_sequence = state->submitted_sequence;
	for (index = 0; index < count; index += execute_count) {
		command = commands[index];
		if ((command == NULL) || (command->sequence <= last_sequence))
			return FB_GFX3_INVALID;
		execute_count = 1u;
		switch (command->type) {
		case FB_GFX3_COMMAND_SURFACE_CREATE:
			result = gles_surface_create(state, command);
			break;
		case FB_GFX3_COMMAND_SURFACE_DESTROY:
			if (command->target == state->visible_surface) {
				state->visible_surface = 0;
				state->presentation_dirty = FALSE;
			}
			result = gles_surface_release(state, command);
			break;
		case FB_GFX3_COMMAND_SURFACE_UPLOAD:
			result = gles_surface_upload(state, command);
			break;
		case FB_GFX3_COMMAND_SURFACE_DOWNLOAD:
			result = gles_surface_download(state, command);
			break;
		case FB_GFX3_COMMAND_CLEAR:
			result = gles_clear(state, command);
			break;
		case FB_GFX3_COMMAND_POINTS:
			execute_count = gles_points_batch_count(commands + index,
				count - index);
			if (execute_count > 1u)
				result = gles_points_batch(state, commands + index,
					execute_count);
			else
				result = gles_points(state, command);
			break;
		case FB_GFX3_COMMAND_LINE:
			execute_count = gles_line_batch_count(commands + index,
				count - index);
			if (execute_count > 1u)
				result = gles_line_batch(state, commands + index, execute_count);
			else
				result = gles_line(state, command);
			break;
		case FB_GFX3_COMMAND_RECTANGLE:
			execute_count = gles_rectangle_batch_count(commands + index,
				count - index);
			if (execute_count > 1u)
				result = gles_rectangle_batch(state, commands + index,
					execute_count);
			else
				result = gles_rectangle(state, command);
			break;
		case FB_GFX3_COMMAND_ELLIPSE:
			execute_count = gles_ellipse_batch_count(commands + index,
				count - index);
			if (execute_count > 1u)
				result = gles_ellipse_batch(state, commands + index,
					execute_count);
			else
				result = gles_ellipse(state, command);
			break;
		case FB_GFX3_COMMAND_PAINT:
			execute_count = gles_paint_batch_count(commands + index,
				count - index);
			result = gles_paint(state,
				commands[index + execute_count - 1u]);
			break;
		case FB_GFX3_COMMAND_BLIT:
			execute_count = gles_blit_batch_count(commands + index,
				(uint32_t)((count - index > UINT32_MAX) ? UINT32_MAX :
				count - index));
			if (execute_count > 1u)
				result = gles_blit_batch(state, commands + index,
					(uint32_t)execute_count);
			else
				result = gles_blit(state, command);
			break;
		case FB_GFX3_COMMAND_BLITS:
			result = gles_blits(state, command);
			break;
		case FB_GFX3_COMMAND_TRANSFORM_BLIT:
			execute_count = gles_transform_blit_batch_count(commands + index,
				(uint32_t)((count - index > UINT32_MAX) ? UINT32_MAX :
				count - index));
			if (execute_count > 1u)
				result = gles_transform_blit_batch(state, commands + index,
					(uint32_t)execute_count);
			else
				result = gles_transform_blit(state, command);
			break;
		case FB_GFX3_COMMAND_READ_PIXEL:
			result = gles_read_pixel(state, command);
			break;
		case FB_GFX3_COMMAND_PALETTE:
			execute_count = gles_palette_batch_count(commands + index,
				(uint32_t)((count - index > UINT32_MAX) ? UINT32_MAX :
				count - index));
			result = gles_palette(state,
				commands[index + execute_count - 1u]);
			break;
		case FB_GFX3_COMMAND_PAGE_SET:
			result = gles_page_set(state, command);
			break;
		case FB_GFX3_COMMAND_PRESENT:
			/*
				SCREENSET and SCREENCOPY presentation requests are asynchronous.
				While later commands remain in this renderer drain, an
				intermediate front buffer is not observable by the Basic thread.
				Defer the swap until the drain's final visible page, but retain a
				one-command synchronous PRESENT as its ordered completion point.
			*/
			if ((index + 1u < count) &&
			    (command->target == state->visible_surface)) {
				state->presentation_dirty = TRUE;
				result = FB_GFX3_OK;
			} else
				result = gles_present(state, command);
			break;
		case FB_GFX3_COMMAND_WINDOW_TITLE:
			result = gles_window_title(state, command);
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
		if (result != FB_GFX3_OK)
			return result;
		if (gles_command_writes_visible(state, command))
			state->presentation_dirty = TRUE;
		last_sequence = commands[index + execute_count - 1u]->sequence;
	}
	if ((count == 1) &&
	    (commands[0]->type == FB_GFX3_COMMAND_INPUT_POLL)) {
		/* Input snapshots do not require Android overlay presentation. */
		state->submitted_sequence = last_sequence;
		if (submitted_sequence != NULL)
			*submitted_sequence = last_sequence;
		return FB_GFX3_OK;
	}
	if ((count == 1) &&
	    (commands[0]->type == FB_GFX3_COMMAND_PLATFORM_POLL)) {
		int overlay_changed;

		gles_read_keyboard_overlay(state, &keyboard_overlay, &keyboard_state);
		overlay_changed = !state->keyboard_overlay_known ||
		    (keyboard_state != state->keyboard_overlay_state) ||
		    (memcmp(&keyboard_overlay, &state->keyboard_overlay,
		     sizeof(keyboard_overlay)) != 0);
		if (state->presentation_dirty || overlay_changed) {
			if (state->visible_surface != 0) {
				result = gles_present_handle(state, state->visible_surface,
					last_sequence);
				if (result != FB_GFX3_OK)
					return result;
			} else {
				state->keyboard_overlay = keyboard_overlay;
				state->keyboard_overlay_state = keyboard_state;
				state->keyboard_overlay_known = TRUE;
			}
			result = gles_record_fence(state, last_sequence);
			if (result != FB_GFX3_OK)
				return result;
		}
		state->submitted_sequence = last_sequence;
		if (submitted_sequence != NULL)
			*submitted_sequence = last_sequence;
		return FB_GFX3_OK;
	}
	if (state->presentation_dirty && (state->visible_surface != 0) &&
	    !defer_page_presentation) {
		result = gles_present_handle(state, state->visible_surface,
			last_sequence);
		if (result != FB_GFX3_OK)
			return result;
	}
	result = gles_record_fence(state, last_sequence);
	if (result != FB_GFX3_OK)
		return result;
	state->submitted_sequence = last_sequence;
	if (submitted_sequence != NULL)
		*submitted_sequence = last_sequence;
	fb_gfx3_log_write(state->logger, FB_GFX3_LOG_TRACE,
		"GLES drain submitted: sequence %llu",
		(unsigned long long)last_sequence);
	return FB_GFX3_OK;
}

static uint64_t gles_completed_sequence(FB_GFX3_BACKEND *backend)
{
	if ((backend == NULL) || (backend->state == NULL))
		return 0;
	return gles_poll_completed((FB_GFX3_GLES_STATE *)backend->state);
}

static int gles_wait_sequence(FB_GFX3_BACKEND *backend, uint64_t sequence)
{
	if ((backend == NULL) || (backend->state == NULL))
		return FB_GFX3_INVALID;
	return gles_wait_for_sequence((FB_GFX3_GLES_STATE *)backend->state,
		sequence);
}

static int gles_wait_idle(FB_GFX3_BACKEND *backend)
{
	FB_GFX3_GLES_STATE *state;

	if ((backend == NULL) || (backend->state == NULL))
		return FB_GFX3_INVALID;
	state = (FB_GFX3_GLES_STATE *)backend->state;
	glFinish();
	while (state->first_fence != NULL)
		gles_remove_first_fence(state);
	state->completed_sequence = state->submitted_sequence;
	return gles_check_error(state, "wait for idle");
}

static void *gles_get_opengl_proc(FB_GFX3_BACKEND *backend,
	const char *name)
{
	FB_GFX3_GLES_STATE *state;
	void *procedure = NULL;

	if ((backend == NULL) || (backend->state == NULL) || (name == NULL) ||
	    (name[0] == '\0'))
		return NULL;
	state = (FB_GFX3_GLES_STATE *)backend->state;
	if ((state->platform_vtable == NULL) ||
	    (state->platform_vtable->load_opengl_function == NULL) ||
	    (state->platform_vtable->load_opengl_function(state->platform, name,
	     &procedure, sizeof(procedure)) != FB_GFX3_OK))
		return NULL;
	return procedure;
}

#else

static int gles_probe(FB_GFX3_BACKEND_CAPS *caps)
{
	if (caps != NULL)
		memset(caps, 0, sizeof(*caps));
	return FB_GFX3_UNSUPPORTED;
}

static int gles_init(FB_GFX3_BACKEND *backend,
	const FB_GFX3_BACKEND_CONFIG *config)
{
	(void)backend;
	(void)config;
	return FB_GFX3_UNSUPPORTED;
}

static void gles_shutdown(FB_GFX3_BACKEND *backend)
{
	if (backend != NULL)
		backend->state = NULL;
}

static int gles_execute(FB_GFX3_BACKEND *backend,
	FB_GFX3_COMMAND *const *commands, size_t count,
	uint64_t *submitted_sequence)
{
	(void)backend;
	(void)commands;
	(void)count;
	(void)submitted_sequence;
	return FB_GFX3_UNSUPPORTED;
}

static uint64_t gles_completed_sequence(FB_GFX3_BACKEND *backend)
{
	(void)backend;
	return 0;
}

static int gles_wait_sequence(FB_GFX3_BACKEND *backend, uint64_t sequence)
{
	(void)backend;
	(void)sequence;
	return FB_GFX3_UNSUPPORTED;
}

static int gles_wait_idle(FB_GFX3_BACKEND *backend)
{
	(void)backend;
	return FB_GFX3_UNSUPPORTED;
}

static void *gles_get_opengl_proc(FB_GFX3_BACKEND *backend,
	const char *name)
{
	(void)backend;
	(void)name;
	return NULL;
}

#endif

const FB_GFX3_BACKEND_VTABLE __fb_gfx3_backend_gles = {
	FB_GFX3_BACKEND_ABI_VERSION,
	"OpenGL ES 3.0",
	gles_probe,
	gles_init,
	gles_shutdown,
	gles_execute,
	gles_completed_sequence,
	gles_wait_sequence,
	gles_wait_idle,
	gles_get_opengl_proc
};

/* end of android/gfx3_backend_gles.c */
