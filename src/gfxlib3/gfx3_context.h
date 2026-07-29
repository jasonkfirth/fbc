/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_context.h

    Purpose:

        Provide the typed internal surface API used by the compatibility front
        end without exposing command allocation details.

    Responsibilities:

        - own one renderer and logger
        - represent validated surfaces with their owning context
        - provide queued draw, transfer, readback, and flush operations

    This file intentionally does NOT contain:

        - the public FreeBASIC graphics ABI
        - VIEW, WINDOW, relative-coordinate, or QB behavior
        - backend-specific texture or image objects
*/

#ifndef __FB_GFX3_CONTEXT_H__
#define __FB_GFX3_CONTEXT_H__

#include "gfx3_debug.h"
#include "gfx3_protocol.h"
#include "gfx3_renderer.h"

typedef struct FB_GFX3_CONTEXT_CONFIG {
	const FB_GFX3_BACKEND_VTABLE *backend;
	void *platform;
	FB_GFX3_LOG_CALLBACK log_callback;
	void *log_user_data;
	const char *title;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t page_count;
	uint32_t console_font_height;
	uint32_t flags;
	size_t queue_capacity;
	size_t resource_capacity;
	uint32_t idle_poll_milliseconds;
	int log_level;
} FB_GFX3_CONTEXT_CONFIG;

typedef struct FB_GFX3_CONTEXT FB_GFX3_CONTEXT;

typedef struct FB_GFX3_SURFACE {
	FB_GFX3_CONTEXT *context;
	FB_GFX3_HANDLE handle;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t usage;
} FB_GFX3_SURFACE;

/*
	CPU FB.IMAGE objects do not carry a gfxlib3-owned mutation hook. Images
	within the per-entry and cache-wide limits therefore retain an exact packed
	CPU snapshot. A partial PUT compares only the source region it can observe,
	which keeps tile draws from rescanning a complete sprite sheet. Oversized
	images retain the bounded content-signature fallback so the cache cannot
	silently double an application's image memory use.
*/
#define FB_GFX3_IMAGE_CACHE_CAPACITY 128u
#define FB_GFX3_IMAGE_CACHE_LOOKUP_CAPACITY 256u
#define FB_GFX3_IMAGE_CACHE_SNAPSHOT_MAX_BYTES (16u * 1024u * 1024u)
/*
	The sprite atlas assigns one stable cell to every CPU image cache entry.
	A fixed mapping avoids a renderer-visible relocation when the LRU reuses an
	entry: queued uploads and blits keep their normal FIFO lifetime rule.

	Desktop compute renderers dynamically pack images into one 4096 by 4096
	surface. Unlike fixed cells, this also admits backgrounds and unusually wide
	sprites, so one public BLITS packet does not become several driver submissions
	merely because its source images have different dimensions.

	GLES retains 128 by 128 fixed cells because mobile drivers commonly store
	every logical depth in a 32-bit texture. That keeps the first-sprite
	allocation at 8 MiB on memory-constrained devices. Larger GLES images retain
	exact dedicated GPU surfaces.
*/
#define FB_GFX3_IMAGE_CACHE_ATLAS_DESKTOP_WIDTH 4096u
#define FB_GFX3_IMAGE_CACHE_ATLAS_DESKTOP_HEIGHT 4096u
#define FB_GFX3_IMAGE_CACHE_ATLAS_DESKTOP_MAX_IMAGE_SIZE 2048u
#define FB_GFX3_IMAGE_CACHE_ATLAS_DESKTOP_ALIGNMENT 4u
#define FB_GFX3_IMAGE_CACHE_ATLAS_GLES_CELL_SIZE 128u
#define FB_GFX3_IMAGE_CACHE_ATLAS_COLUMNS 16u
#define FB_GFX3_IMAGE_CACHE_ATLAS_ROWS 8u
#define FB_GFX3_CONTEXT_PENDING_COMMAND_LIMIT 1024u
/* OpenGL's ordered winner key reserves 13 bits for a line index. */
#define FB_GFX3_CONTEXT_PENDING_LINE_LIMIT 8191u
/* Matches the renderer's normal command drain and bounds producer storage. */
#define FB_GFX3_CONTEXT_PENDING_RECTANGLE_LIMIT 1024u
/* Storage ceiling for backend-selected packed BLIT packet limits. */
#define FB_GFX3_CONTEXT_PENDING_BLIT_LIMIT 8192u
/* Compact glyph records use the same bounded producer packet size. */
/* Match the ordered GPU glyph index space to avoid partial text submissions. */
#define FB_GFX3_CONTEXT_PENDING_GLYPH_LIMIT 8191u

typedef struct FB_GFX3_IMAGE_CACHE_ENTRY {
	const void *image_header;
	uint64_t content_hash;
	uint64_t last_use;
	/*
		Oversized CPU sprite sheets cannot fit in one device texture. Such an
		entry owns only the source rectangle named here; ordinary images use
		the complete-image rectangle starting at zero.
	*/
	uint32_t cached_source_x;
	uint32_t cached_source_y;
	uint32_t cached_source_width;
	uint32_t cached_source_height;
	int caches_source_region;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t bytes_per_pixel;
	/* Generation is trusted only for owned headers with no IMAGEINFO exposure. */
	uint32_t image_generation;
	int metadata_owned;
	/* Full-image native colour cached with the exact mutation snapshot. */
	uint32_t uniform_color;
	int uniform_full_image;
	unsigned char *snapshot;
	size_t snapshot_size;
	FB_GFX3_SURFACE surface;
	uint32_t atlas_x;
	uint32_t atlas_y;
	int uses_atlas;
} FB_GFX3_IMAGE_CACHE_ENTRY;

struct FB_GFX3_CONTEXT {
	FB_GFX3_RENDERER renderer;
	FB_GFX3_LOGGER logger;
	int renderer_initialized;
	int logger_initialized;
	/*
		Presentation revision tracking

		SLEEP and event polling may invoke the runtime view-update hook many
		thousands of times between two BASIC drawing frames. A second
		asynchronous PRESENT of the same visible surface revision cannot alter
		the front buffer, but it still consumes renderer and graphics-driver
		time. Writers advance visible_content_revision when they queue work for
		the selected surface. The most recently queued PRESENT records that
		revision so unchanged asynchronous requests can be collapsed safely.

		Explicit waiting presents are never collapsed because callers use them
		as a GPU synchronization boundary.
	*/
	_Atomic FB_GFX3_HANDLE visible_surface_handle;
	_Atomic uint64_t visible_content_revision;
	_Atomic uint64_t queued_present_revision;
	/*
		The common unchanged-frame path may return without taking
		submission_mutex only when no staged or queued producer work remains.
	*/
	_Atomic int pending_submission_work;
	/*
		CPU FB.IMAGE PUT operations upload into this reusable sampled surface.
		Commands remain ordered, so one upload followed by its blit completes
		before a later PUT overwrites the same staging storage.
	*/
	FB_GFX3_SURFACE image_upload_surface;
	/*
		Small stable FB.IMAGE objects share one sampled GPU allocation. This is
		the cross-backend sprite residency path: heterogeneous cached images can
		remain in one packed PUT batch because they now have one source handle.
	*/
	FB_GFX3_SURFACE image_cache_atlas;
	int image_cache_atlas_attempted;
	/*
		Desktop atlas shelf allocator

		Image cache entries never move while they remain live. New regions are
		assigned from the current shelf and renderer FIFO ordering makes an LRU
		replacement safe even though an older atlas command may still be queued.
		If the bounded atlas fills, later images use dedicated surfaces.
	*/
	uint32_t image_cache_atlas_next_x;
	uint32_t image_cache_atlas_next_y;
	uint32_t image_cache_atlas_row_height;
	int image_cache_atlas_dynamic;
	FB_GFX3_IMAGE_CACHE_ENTRY image_cache[FB_GFX3_IMAGE_CACHE_CAPACITY];
	/*
		Direct pointer lookup is advisory. Every hit is verified against the
		entry header, so collisions and stale slots after LRU reuse are harmless.
	*/
	uint32_t image_cache_lookup[FB_GFX3_IMAGE_CACHE_LOOKUP_CAPACITY];
	size_t image_cache_snapshot_bytes;
	uint64_t image_cache_clock;
	/*
		SCREENEVENT serializes through FB_GRAPHICS_LOCK, so one persistent
		payload-free poll command and completion can serve the complete mode.
	*/
	FB_GFX3_COMMAND *input_poll_command;
	FB_GFX3_COMPLETION input_poll_completion;
	int input_poll_completion_initialized;
	/*
		Asynchronous compatibility calls often arrive in sprite-sized runs. Keep
		their owned command records briefly so one queue lock and renderer wake
		can deliver an ordered batch. A synchronous command always drains this
		array first, preserving public request/response ordering.
	*/
	FBMUTEX *submission_mutex;
	FB_GFX3_COMMAND *pending_commands[FB_GFX3_CONTEXT_PENDING_COMMAND_LIMIT];
	size_t pending_command_count;
	/*
		Adjacent opaque LINE operations remain compact until a true ordering
		boundary. The render thread submits this storage to shader line batches.
	*/
	FB_GFX3_LINE_COMMAND
		pending_lines[FB_GFX3_CONTEXT_PENDING_LINE_LIMIT];
	FB_GFX3_HANDLE pending_line_target;
	size_t pending_line_count;
	/*
		Opaque rectangles can be packed until an ordering boundary. Backends
		advertising PACKED_RECTANGLES may mix filled and outline boxes.
	*/
	FB_GFX3_RECTANGLE_COMMAND
		pending_rectangles[FB_GFX3_CONTEXT_PENDING_RECTANGLE_LIMIT];
	FB_GFX3_HANDLE pending_rectangle_target;
	size_t pending_rectangle_count;
	/* Compatible non-self PUT operations share one producer-side packet. */
	FB_GFX3_BLIT_COMMAND pending_blits[FB_GFX3_CONTEXT_PENDING_BLIT_LIMIT];
	FB_GFX3_HANDLE pending_blit_target;
	FB_GFX3_HANDLE pending_blit_source;
	uint32_t pending_blit_mode;
	uint32_t pending_blit_alpha;
	size_t pending_blit_count;
	/* Built-in bitmap strings remain compact until an ordering boundary. */
	FB_GFX3_GLYPH pending_glyphs[FB_GFX3_CONTEXT_PENDING_GLYPH_LIMIT];
	FB_GFX3_RECT pending_glyph_clip;
	FB_GFX3_HANDLE pending_glyph_target;
	size_t pending_glyph_count;
};

int fb_gfx3_context_init(FB_GFX3_CONTEXT *context,
	const FB_GFX3_CONTEXT_CONFIG *config);
int fb_gfx3_context_shutdown(FB_GFX3_CONTEXT *context);
int fb_gfx3_context_submit_pending(FB_GFX3_CONTEXT *context);
int fb_gfx3_context_flush(FB_GFX3_CONTEXT *context);
int fb_gfx3_context_poll_platform(FB_GFX3_CONTEXT *context);
int fb_gfx3_context_set_palette(FB_GFX3_CONTEXT *context,
	const uint32_t *palette);
int fb_gfx3_context_set_window_title(FB_GFX3_CONTEXT *context,
	const char *title, size_t length);
int fb_gfx3_context_run_interop_callback(FB_GFX3_CONTEXT *context,
	FB_GFX3_INTEROP_CALLBACK callback, void *user_data);

int fb_gfx3_surface_create(FB_GFX3_CONTEXT *context,
	FB_GFX3_SURFACE *surface, uint32_t width, uint32_t height,
	uint32_t depth, uint32_t usage, uint32_t clear_color);
int fb_gfx3_surface_destroy(FB_GFX3_SURFACE *surface);
int fb_gfx3_surface_upload(FB_GFX3_SURFACE *surface, int x, int y,
	uint32_t width, uint32_t height, uint32_t pitch, const void *pixels);
int fb_gfx3_surface_download(FB_GFX3_SURFACE *surface, int x, int y,
	uint32_t width, uint32_t height, uint32_t pitch, void *pixels);
int fb_gfx3_surface_clear(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, uint32_t color, uint32_t flags);
int fb_gfx3_surface_points(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, const FB_GFX3_POINT *points, uint32_t count);
int fb_gfx3_surface_glyphs(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, const FB_GFX3_GLYPH *glyphs, uint32_t count);
int fb_gfx3_surface_line(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style, uint32_t flags);
int fb_gfx3_surface_line_graphics_locked(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style, uint32_t flags);
int fb_gfx3_surface_rectangle(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style, int filled, uint32_t flags);
/*
	Compatibility fast paths may avoid a second operating-system mutex only
	while they hold the runtime-wide FB_GRAPHICS_LOCK. These functions have the
	same validation and ordering rules as their normally synchronized forms.
*/
int fb_gfx3_surface_rectangle_graphics_locked(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style, int filled, uint32_t flags);
int fb_gfx3_surface_rectangles(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECTANGLE_COMMAND *rectangles, uint32_t count);
int fb_gfx3_surface_ellipse(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int center_x, int center_y,
	float radius_x, float radius_y, uint32_t color, int filled,
	uint32_t flags);
int fb_gfx3_surface_ellipse_graphics_locked(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int center_x, int center_y,
	float radius_x, float radius_y, uint32_t color, int filled,
	uint32_t flags);
int fb_gfx3_surface_paint(FB_GFX3_SURFACE *surface,
	const FB_GFX3_RECT *clip, int x, int y, uint32_t color,
	uint32_t border_color, uint32_t flags, uint32_t paint_mode,
	const unsigned char *pattern, size_t pattern_size,
	uint32_t pattern_origin_x, uint32_t pattern_origin_y);
int fb_gfx3_surface_blit(FB_GFX3_SURFACE *destination,
	const FB_GFX3_RECT *clip, const FB_GFX3_SURFACE *source,
	const FB_GFX3_RECT *source_rect, int destination_x, int destination_y,
	uint32_t mode, uint32_t alpha);
int fb_gfx3_surface_blit_graphics_locked(FB_GFX3_SURFACE *destination,
	const FB_GFX3_RECT *clip, const FB_GFX3_SURFACE *source,
	const FB_GFX3_RECT *source_rect, int destination_x, int destination_y,
	uint32_t mode, uint32_t alpha);
int fb_gfx3_surface_transform_blit(FB_GFX3_SURFACE *destination,
	const FB_GFX3_RECT *clip, const FB_GFX3_SURFACE *source,
	const FB_GFX3_RECT *source_rect, const FB_GFX3_RECT *destination_bounds,
	const float inverse[9], uint32_t mode, uint32_t alpha,
	uint32_t filter, uint32_t wrap);
int fb_gfx3_surface_read_pixel(FB_GFX3_SURFACE *surface, int x, int y,
	uint32_t *color);
int fb_gfx3_surface_set_visible(FB_GFX3_SURFACE *surface);
int fb_gfx3_surface_present(FB_GFX3_SURFACE *surface, int wait);

#endif

/* end of gfx3_context.h */
