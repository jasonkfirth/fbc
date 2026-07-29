/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_vulkan.h

    Purpose:

        Declare the header-independent Vulkan runtime used by the gfxlib3
        renderer backend.

    Responsibilities:

        - expose loader, instance, physical-device, logical-device, and queue
          lifecycle as one checked operation
        - report the selected queue family and loader API version
        - expose checked device-local surface transfers and GPU primitives
	- expose Win32, X11, and Android surface, swapchain, and presentation
	  lifecycle
        - keep Vulkan implementation types private from the common renderer

    This file intentionally does NOT contain:

        - Vulkan SDK header dependencies
        - Vulkan SDK declarations
        - a FreeBASIC-visible low-level Vulkan API
*/

#ifndef __FB_GFX3_VULKAN_H__
#define __FB_GFX3_VULKAN_H__

#include "gfx3_protocol.h"

typedef struct FB_GFX3_VULKAN_RUNTIME {
	void *implementation;
	uint32_t loader_api_version;
	uint32_t physical_device_count;
	/* Loader enumeration index and PCI identifiers of the selected adapter. */
	uint32_t selected_physical_device_index;
	uint32_t selected_vendor_id;
	uint32_t selected_device_id;
	uint32_t selected_device_type;
	/*
		The loader-provided device name is retained for diagnostics. It is
		always terminated even if a non-conforming driver fills all 256 bytes.
	*/
	char selected_device_name[256];
	/* VkPhysicalDeviceLimits::maxStorageBufferRange for the selected adapter. */
	uint64_t maximum_storage_buffer_range;
	uint32_t queue_family_index;
	uint32_t queue_flags;
	uint32_t present_width;
	uint32_t present_height;
	uint32_t swapchain_image_count;
	/* Live command slots retained by the runtime, not Vulkan queue depth. */
	uint32_t in_flight_submission_count;
	/* High-water mark used by the standalone slot-ring regression test. */
	uint32_t maximum_in_flight_submission_count;
	/* Historical submission counter retained for existing bootstrap diagnostics. */
	uint64_t completed_submission_count;
	/*
		Actual vkQueueSubmit call count. Several ordered runtime operations may
		share one call when command-buffer batching is enabled.
	*/
	uint64_t queue_submit_count;
	int windowed;
	int initialized;
} FB_GFX3_VULKAN_RUNTIME;

typedef struct FB_GFX3_VULKAN_SURFACE {
	void *implementation;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
} FB_GFX3_VULKAN_SURFACE;

/*
	A batch contains ordered operations for one source and destination surface.
	The Vulkan backend only forms these from adjacent renderer commands. Its
	tiled path resolves an ordered shared-source packet in one compute dispatch.
	The descriptor fallback records a dispatch and write-to-read barrier for
	each entry. Both routes preserve the result of separate PUT commands.
*/
typedef struct FB_GFX3_VULKAN_BLIT {
	FB_GFX3_RECT clip;
	FB_GFX3_RECT source_rect;
	int32_t destination_x;
	int32_t destination_y;
	uint32_t mode;
	uint32_t alpha;
} FB_GFX3_VULKAN_BLIT;

/*
	A full-surface PSET between distinct GPU surfaces is a transfer operation,
	not a per-pixel blend. PAGE COPY uses this compact description so an ordered
	run of page flips can become one Vulkan command-buffer submission. The GPU
	still performs every copy; the BASIC and renderer threads do no pixel work.
*/
typedef struct FB_GFX3_VULKAN_SURFACE_COPY {
	FB_GFX3_VULKAN_SURFACE *destination;
	FB_GFX3_VULKAN_SURFACE *source;
} FB_GFX3_VULKAN_SURFACE_COPY;

/*
	Adjacent LINE commands share a target but must retain FIFO overlap order.
	The runtime records each entry as an individual compute dispatch within one
	command-buffer submission, with a compute write-to-read barrier between
	entries.
*/
typedef struct FB_GFX3_VULKAN_LINE {
	FB_GFX3_RECT clip;
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
	uint32_t color;
	uint32_t style;
	uint32_t flags;
} FB_GFX3_VULKAN_LINE;

/*
	An ellipse batch retains one exact midpoint dispatch per command because
	shapes may overlap. The runtime records those dispatches and their
	write-to-read dependencies in one Vulkan submission.
*/
typedef struct FB_GFX3_VULKAN_ELLIPSE {
	FB_GFX3_RECT clip;
	int32_t center_x;
	int32_t center_y;
	float radius_x;
	float radius_y;
	uint32_t color;
	int filled;
	uint32_t flags;
} FB_GFX3_VULKAN_ELLIPSE;

/*
	POINTS commands can overlap, so their dispatches remain ordered even when
	the Vulkan backend records them in one submission.  Point memory belongs to
	the caller until the batch call returns.
*/
typedef struct FB_GFX3_VULKAN_POINTS {
	FB_GFX3_RECT clip;
	const FB_GFX3_POINT *points;
	uint32_t point_count;
} FB_GFX3_VULKAN_POINTS;

enum FB_GFX3_VULKAN_PRIMITIVE_TYPE {
	FB_GFX3_VULKAN_PRIMITIVE_LINE = 1,
	FB_GFX3_VULKAN_PRIMITIVE_ELLIPSE = 2,
	FB_GFX3_VULKAN_PRIMITIVE_POINT = 3,
	FB_GFX3_VULKAN_PRIMITIVE_RECTANGLE = 4
};

/*
	One shader-visible record can describe an opaque point, styled line,
	midpoint ellipse, or styled rectangle. The backend flattens only adjacent
	commands for one target, and order is the one-based FIFO winner key.
	Rectangle parameters store the 16-bit style in the low half and the filled
	flag in bit 31.
*/
typedef struct FB_GFX3_VULKAN_PRIMITIVE {
	int32_t geometry[4];
	FB_GFX3_RECT clip;
	uint32_t parameters[4]; /* color, type, style/filled, order */
	uint32_t format[4];     /* surface width, height, color mask, reserved */
	uint32_t batch[4];      /* generation, resolve x, resolve y, workgroups */
} FB_GFX3_VULKAN_PRIMITIVE;

/*
	Opaque filled rectangles normally use ordered transfer fills. Same-colour
	runs may instead use the rectangle compute shader because overlap order then
	cannot change a pixel result.
*/
typedef struct FB_GFX3_VULKAN_CLEAR_RECTANGLE {
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
	uint32_t color;
} FB_GFX3_VULKAN_CLEAR_RECTANGLE;

/*
	An ordered rectangle packet can mix filled and outline boxes. Exact clipping
	and style coverage remain shader work; the host uses bounds only to assign
	each command to the small set of surface tiles it may touch.
*/
typedef struct FB_GFX3_VULKAN_RECTANGLE {
	FB_GFX3_RECT clip;
	int32_t x1;
	int32_t y1;
	int32_t x2;
	int32_t y2;
	uint32_t color;
	uint32_t style;
	uint32_t filled;
	uint32_t flags;
} FB_GFX3_VULKAN_RECTANGLE;

/*
	Adapter and queue values mirror the Vulkan 1.0 values without pulling
	Vulkan SDK headers into the public gfxlib3 build.  The score helper is
	kept separate from loader state so regression tests can cover the policy
	without requiring a Vulkan-capable machine.
*/
#define FB_GFX3_VULKAN_DEVICE_TYPE_OTHER           0u
#define FB_GFX3_VULKAN_DEVICE_TYPE_INTEGRATED_GPU  1u
#define FB_GFX3_VULKAN_DEVICE_TYPE_DISCRETE_GPU    2u
#define FB_GFX3_VULKAN_DEVICE_TYPE_VIRTUAL_GPU     3u
#define FB_GFX3_VULKAN_DEVICE_TYPE_CPU             4u

#define FB_GFX3_VULKAN_QUEUE_GRAPHICS              0x00000001u
#define FB_GFX3_VULKAN_QUEUE_COMPUTE               0x00000002u

uint64_t fb_gfx3_vulkan_device_score(uint32_t device_type,
	int shader_float64, uint32_t queue_flags);

int fb_gfx3_vulkan_runtime_open(FB_GFX3_VULKAN_RUNTIME *runtime);
int fb_gfx3_vulkan_runtime_open_windowed(FB_GFX3_VULKAN_RUNTIME *runtime,
	uintptr_t native_instance, uintptr_t native_window, uint32_t width,
	uint32_t height);
int fb_gfx3_vulkan_runtime_resize(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint32_t width, uint32_t height);
int fb_gfx3_vulkan_runtime_tag_submission(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint64_t sequence);
uint64_t fb_gfx3_vulkan_runtime_completed_sequence(
	FB_GFX3_VULKAN_RUNTIME *runtime);
int fb_gfx3_vulkan_runtime_wait_sequence(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint64_t sequence);
int fb_gfx3_vulkan_runtime_poll(FB_GFX3_VULKAN_RUNTIME *runtime);
int fb_gfx3_vulkan_runtime_wait_idle(FB_GFX3_VULKAN_RUNTIME *runtime);
int fb_gfx3_vulkan_runtime_submit_empty(FB_GFX3_VULKAN_RUNTIME *runtime);
int fb_gfx3_vulkan_runtime_fill_u32(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint32_t *values, size_t value_count, uint32_t value);
int fb_gfx3_vulkan_runtime_compute_add_u32(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint32_t *values, size_t value_count, uint32_t addend);
int fb_gfx3_vulkan_surface_create(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, uint32_t width, uint32_t height,
	uint32_t depth, uint32_t clear_color);
int fb_gfx3_vulkan_surface_clear(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, int32_t x1, int32_t y1,
	int32_t x2, int32_t y2, uint32_t color);
int fb_gfx3_vulkan_surface_clear_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_CLEAR_RECTANGLE *rectangles,
	size_t rectangle_count);
int fb_gfx3_vulkan_surface_opaque_rectangle_batch(
	FB_GFX3_VULKAN_RUNTIME *runtime, FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_CLEAR_RECTANGLE *rectangles,
	size_t rectangle_count);
int fb_gfx3_vulkan_surface_rectangle_batch(
	FB_GFX3_VULKAN_RUNTIME *runtime, FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_RECTANGLE *rectangles, size_t rectangle_count);
int fb_gfx3_vulkan_surface_points(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	const FB_GFX3_POINT *points, uint32_t point_count);
int fb_gfx3_vulkan_surface_points_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_POINTS *operations, size_t operation_count);
int fb_gfx3_vulkan_surface_primitive_batch(
	FB_GFX3_VULKAN_RUNTIME *runtime, FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_PRIMITIVE *primitives, size_t primitive_count);
int fb_gfx3_vulkan_surface_glyphs(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	const FB_GFX3_GLYPH *glyphs, uint32_t glyph_count);
int fb_gfx3_vulkan_surface_line(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	int32_t x1, int32_t y1, int32_t x2, int32_t y2,
	uint32_t color, uint32_t style, uint32_t flags);
int fb_gfx3_vulkan_surface_line_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_VULKAN_LINE *lines,
	size_t line_count);
int fb_gfx3_vulkan_surface_rectangle(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	int32_t x1, int32_t y1, int32_t x2, int32_t y2,
	uint32_t color, uint32_t style, int filled, uint32_t flags);
int fb_gfx3_vulkan_surface_ellipse(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	int32_t center_x, int32_t center_y, float radius_x, float radius_y,
	uint32_t color, int filled, uint32_t flags);
int fb_gfx3_vulkan_surface_ellipse_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_ELLIPSE *ellipses, size_t ellipse_count);
int fb_gfx3_vulkan_surface_paint(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	int32_t x, int32_t y, uint32_t color, uint32_t border_color,
	uint32_t flags, const FB_GFX3_PAINT_COMMAND *paint_command);
int fb_gfx3_vulkan_surface_blit(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *destination,
	FB_GFX3_VULKAN_SURFACE *source, const FB_GFX3_RECT *clip,
	const FB_GFX3_RECT *source_rect, int32_t destination_x,
	int32_t destination_y, uint32_t mode, uint32_t alpha);
int fb_gfx3_vulkan_surface_copy_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	const FB_GFX3_VULKAN_SURFACE_COPY *copies, size_t copy_count);
int fb_gfx3_vulkan_surface_transform_blit(
	FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *destination,
	FB_GFX3_VULKAN_SURFACE *source,
	const FB_GFX3_TRANSFORM_BLIT_COMMAND *transform);
int fb_gfx3_vulkan_surface_transform_blit_batch(
	FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *destination,
	FB_GFX3_VULKAN_SURFACE *source,
	const FB_GFX3_TRANSFORM_BLIT_COMMAND *const *transforms,
	size_t transform_count);
int fb_gfx3_vulkan_surface_blit_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *destination,
	FB_GFX3_VULKAN_SURFACE *source, const FB_GFX3_VULKAN_BLIT *blits,
	size_t blit_count);
int fb_gfx3_vulkan_surface_upload(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, int32_t destination_x,
	int32_t destination_y, uint32_t width, uint32_t height,
	const void *source, size_t source_pitch);
int fb_gfx3_vulkan_surface_download(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, int32_t source_x, int32_t source_y,
	uint32_t width, uint32_t height, void *destination,
	size_t destination_pitch);
int fb_gfx3_vulkan_surface_present(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const uint32_t *palette,
	size_t palette_count, const int32_t keyboard_button_rect[4],
	uint32_t keyboard_button_state);
void fb_gfx3_vulkan_surface_destroy(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface);
void fb_gfx3_vulkan_runtime_close(FB_GFX3_VULKAN_RUNTIME *runtime);

#endif

/* end of gfx3_vulkan.h */
