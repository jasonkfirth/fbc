/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_compat.h

    Purpose:

        Define the compatibility state between FreeBASIC graphics entry
        points and the typed gfxlib3 renderer context.

    Responsibilities:

        - own logical screen pages and visible-page state
        - hold one caller's VIEW, WINDOW, pen, and color state
        - translate initial pixel-oriented gfxlib2 operations to GPU commands

    This file intentionally does NOT contain:

        - exported fb_Gfx* ABI declarations
        - platform windows, input events, or graphical console state
        - CPU FB.IMAGE compatibility storage
*/

#ifndef __FB_GFX3_COMPAT_H__
#define __FB_GFX3_COMPAT_H__

#include "gfx3_context.h"
#include "gfx3_input.h"

enum FB_GFX3_COORDINATE_TYPE {
	FB_GFX3_COORDINATE_AA = 0,
	FB_GFX3_COORDINATE_AR = 1,
	FB_GFX3_COORDINATE_RA = 2,
	FB_GFX3_COORDINATE_RR = 3,
	FB_GFX3_COORDINATE_A = 4,
	FB_GFX3_COORDINATE_R = 5,
	FB_GFX3_COORDINATE_MASK = 7
};

#define FB_GFX3_DEFAULT_COLOR_1 0x80000000u
#define FB_GFX3_DEFAULT_COLOR_2 0x40000000u

enum FB_GFX3_LINE_TYPE {
	FB_GFX3_LINE_TYPE_LINE = 0,
	FB_GFX3_LINE_TYPE_BOX = 1,
	FB_GFX3_LINE_TYPE_FILLED_BOX = 2
};

enum FB_GFX3_DRAW_STATE_FLAGS {
	FB_GFX3_WINDOW_ACTIVE = 0x00000001u,
	FB_GFX3_WINDOW_SCREEN = 0x00000002u,
	FB_GFX3_VIEWPORT_SET = 0x00000004u,
	FB_GFX3_VIEW_SCREEN = 0x00000008u
};

enum FB_GFX3_MODE_FLAGS {
	/* Public FB.GFX_ALPHA_PRIMITIVES SCREEN flag from fbgfx.bi. */
	FB_GFX3_MODE_ALPHA_PRIMITIVES = 0x00000040u,
	/* Public FB.GFX_RESIZABLE SCREEN flag from fbgfx.bi. */
	FB_GFX3_MODE_RESIZABLE = 0x00000400u
};

/*
	One cached POINT result per screen page avoids repeatedly stalling the GPU
	when a BASIC loop reads a coordinate no intervening draw can affect. It is
	never a substitute for a real readback after a potentially overlapping draw.

	The pair count identifies software alpha and other read-modify-write loops
	which spell each pixel as POINT followed by PSET. After that pattern repeats,
	gfxlib3 can synchronize the page once instead of synchronizing every pixel.
*/
typedef struct FB_GFX3_POINT_CACHE {
	int32_t x;
	int32_t y;
	uint32_t color;
	uint32_t read_modify_pairs;
	unsigned char valid;
} FB_GFX3_POINT_CACHE;

typedef struct FB_GFX3_MODE {
	FB_GFX3_CONTEXT context;
	FB_GFX3_INPUT_STATE input;
	FB_GFX3_SURFACE *pages;
	unsigned char **shadow_pages;
	/* Saved SCREENLOCK image used to identify the rows changed by a caller. */
	unsigned char **shadow_snapshots;
	unsigned char *shadow_valid;
	unsigned char *shadow_dirty;
	/*
		PSET and locked primitive paths know which rows they changed. Retaining
		that range avoids a full-page upload at the next GPU ordering boundary.
		A raw SCREENPTR marks every row because its writes are not observable.
	*/
	uint32_t *shadow_dirty_first_line;
	uint32_t *shadow_dirty_last_line;
	unsigned char *shadow_snapshot_active;
	FB_GFX3_POINT_CACHE *point_cache;
	/* Reused by compatibility screen PAINT while mode->mutex is held. */
	unsigned char *paint_visited;
	size_t *paint_queue;
	size_t paint_scratch_capacity;
	FBMUTEX *mutex;
	uint64_t generation;
	uint64_t resize_serial;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t page_count;
	uint32_t console_font_width;
	uint32_t console_font_height;
	uint32_t console_rows;
	uint32_t visible_page;
	uint32_t shadow_pitch;
	uint32_t access_lock_count;
	uint32_t palette[256];
	/*
		PALETTE with a negative index restores the palette selected when the
		mode opened. Standard monochrome modes do not share SCREENRES's VGA
		256-colour default, so retain the exact mode-specific table.
	*/
	uint32_t default_palette[256];
	/*
		Legacy adapters select their active attributes from the first sixteen
		VGA-compatible colours. Keep that source table separate from palette
		registers which may be remapped while the mode is active.
	*/
	uint32_t standard_colors[16];
	uint32_t vga_palette_index;
	uint32_t vga_palette_color;
	uint32_t vga_palette_shift;
	uint32_t standard_foreground_color;
	int standard_mode;
	int alpha_primitives;
	int resizable;
	int initialized;
} FB_GFX3_MODE;

typedef struct FB_GFX3_DRAW_STATE {
	FB_GFX3_MODE *mode;
	uint64_t generation;
	uint64_t resize_serial;
	FB_GFX3_RECT view;
	FB_GFX3_RECT pending_points_clip;
	FB_GFX3_POINT *pending_points;
	/*
		Alpha PSET writes which map to the same physical pixel are assigned to
		successive layers. Each submitted layer therefore contains unique
		coordinates and can run in parallel without changing BASIC draw order.
	*/
	FB_GFX3_POINT *pending_point_submission;
	uint64_t *pending_point_keys;
	uint32_t *pending_point_indices;
	uint32_t *pending_point_generations;
	uint32_t *pending_point_layers;
	uint32_t *pending_point_layer_offsets;
	float window_x;
	float window_y;
	float window_width;
	float window_height;
	float last_x;
	float last_y;
	uint32_t foreground_color;
	uint32_t background_color;
	uint32_t work_page;
	uint32_t pending_points_count;
	uint32_t pending_points_capacity;
	uint32_t pending_points_page;
	uint32_t pending_point_key_capacity;
	uint32_t pending_point_generation;
	uint32_t pending_point_max_layer;
	uint32_t flags;
	float draw_scale;
	int draw_angle;
} FB_GFX3_DRAW_STATE;

int fb_gfx3_mode_init(FB_GFX3_MODE *mode,
	const FB_GFX3_CONTEXT_CONFIG *config, uint32_t clear_color);
int fb_gfx3_mode_shutdown(FB_GFX3_MODE *mode);
int fb_gfx3_mode_resize(FB_GFX3_MODE *mode, FB_GFX3_DRAW_STATE *state,
	uint32_t width, uint32_t height);
int fb_gfx3_draw_state_init(FB_GFX3_MODE *mode,
	FB_GFX3_DRAW_STATE *state);
int fb_gfx3_draw_state_sync_resize(FB_GFX3_DRAW_STATE *state);
void fb_gfx3_draw_state_destroy(FB_GFX3_DRAW_STATE *state);

/*
	PSET is commonly used as a software rasterizer's innermost operation.
	Accumulate compatible pixels in a caller-local buffer, then submit one GPU
	command at an operation that must observe their ordering.
*/
int fb_gfx3_compat_flush_points(FB_GFX3_DRAW_STATE *state);

/*
	Public fb_Gfx* entry points already hold FB_GRAPHICS_LOCK.  They may use
	this variant to preserve PSET-to-following-operation order without taking
	the mode mutex solely to discover that no PSET batch is pending.
*/
int fb_gfx3_compat_flush_points_graphics_locked(FB_GFX3_DRAW_STATE *state);

/*
	Commit the PSET compatibility shadow before an operation that reads or
	modifies the same GPU surface outside the PSET fast path.
*/
int fb_gfx3_compat_commit_shadow(FB_GFX3_DRAW_STATE *state);

/*
	A full-page clear replaces every old pixel, so it can make an existing or
	locked CPU shadow authoritative without first downloading or uploading the
	superseded page. The caller has already queued a successful GPU clear and
	holds FB_GRAPHICS_LOCK.
*/
int fb_gfx3_compat_replace_shadow_after_full_clear_graphics_locked(
	FB_GFX3_DRAW_STATE *state, uint32_t color);

void fb_gfx3_compat_invalidate_point_cache_rect_graphics_locked(
	FB_GFX3_DRAW_STATE *state, int x1, int y1, int x2, int y2);
void fb_gfx3_compat_invalidate_point_cache_graphics_locked(
	FB_GFX3_DRAW_STATE *state);

int fb_gfx3_page_set(FB_GFX3_DRAW_STATE *state, int work_page,
	int visible_page, int *previous_pages);
int fb_gfx3_page_copy(FB_GFX3_DRAW_STATE *state, int from_page,
	int to_page);

int fb_gfx3_compat_pset(FB_GFX3_DRAW_STATE *state, float x, float y,
	uint32_t color, uint32_t flags, int preset);
int fb_gfx3_compat_point(FB_GFX3_DRAW_STATE *state, float x, float y,
	uint32_t *color);
int fb_gfx3_compat_line(FB_GFX3_DRAW_STATE *state, float x1, float y1,
	float x2, float y2, uint32_t color, int type, uint32_t style,
	uint32_t flags);
int fb_gfx3_compat_ellipse(FB_GFX3_DRAW_STATE *state, float x, float y,
	float radius, uint32_t color, float aspect, int filled,
	uint32_t flags);
int fb_gfx3_compat_arc(FB_GFX3_DRAW_STATE *state, float x, float y,
	float radius, uint32_t color, float aspect, float start, float end,
	uint32_t flags);

int fb_gfx3_compat_view(FB_GFX3_DRAW_STATE *state, int x1, int y1,
	int x2, int y2, uint32_t fill_color, uint32_t border_color,
	uint32_t flags);
int fb_gfx3_compat_window(FB_GFX3_DRAW_STATE *state, float x1, float y1,
	float x2, float y2, int screen);
int fb_gfx3_compat_window_graphics_locked(FB_GFX3_DRAW_STATE *state,
	float x1, float y1, float x2, float y2, int screen);
int fb_gfx3_compat_pmap(FB_GFX3_DRAW_STATE *state, float coordinate,
	int function, float *result);
int fb_gfx3_compat_pmap_graphics_locked(FB_GFX3_DRAW_STATE *state,
	float coordinate, int function, float *result);
int fb_gfx3_compat_cursor(FB_GFX3_DRAW_STATE *state, int function,
	float *result);
int fb_gfx3_compat_cursor_graphics_locked(FB_GFX3_DRAW_STATE *state,
	int function, float *result);
int fb_gfx3_compat_resolve_point(FB_GFX3_DRAW_STATE *state, float *x,
	float *y, uint32_t flags, int *translated_x, int *translated_y);
int fb_gfx3_compat_points_logical(FB_GFX3_DRAW_STATE *state,
	const FB_GFX3_POINT *points, uint32_t count);
int fb_gfx3_compat_points_absolute(FB_GFX3_DRAW_STATE *state,
	const FB_GFX3_POINT *points, uint32_t count);
int fb_gfx3_compat_glyphs_absolute(FB_GFX3_DRAW_STATE *state,
	const FB_GFX3_GLYPH *glyphs, uint32_t count);
int fb_gfx3_compat_rectangles_absolute(FB_GFX3_DRAW_STATE *state,
	const FB_GFX3_RECTANGLE_COMMAND *rectangles, uint32_t count);
uint32_t fb_gfx3_compat_primitive_flags(const FB_GFX3_DRAW_STATE *state,
	uint32_t target_depth, uint32_t color);

#endif

/* end of gfx3_compat.h */
