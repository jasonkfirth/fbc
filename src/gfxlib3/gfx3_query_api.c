/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_query_api.c

    Purpose:

        Export graphics state queries and compatibility controls that do not
        belong to primitive rendering or image transfer modules.

    Responsibilities:

        - report active mode, driver, viewport, color, and pen state
        - provide a synchronized CPU shadow for SCREENPTR/SCREENLOCK
        - expose palette storage for indexed modes
        - provide bounded no-device behavior for pending input/event adapters

    This file intentionally does NOT contain:

        - platform windows, event pumping, joystick drivers, or presentation
        - primitive drawing or FB.IMAGE allocation
        - graphical console hooks
*/

#include "gfx3_api_internal.h"
#include "gfx3_backend_select.h"
#include "gfx3_compat.h"
#include "gfx3_joystick.h"
#include "gfx3_platform.h"
#include "gfx3_screenlist.h"

FBCALL void fb_GfxSetWindowTitle(FBSTRING *title);

enum FB_GFX3_CONTROL_ID {
	FB_GFX3_GET_WINDOW_POS = 0,
	FB_GFX3_GET_WINDOW_TITLE = 1,
	FB_GFX3_GET_WINDOW_HANDLE = 2,
	FB_GFX3_GET_DESKTOP_SIZE = 3,
	FB_GFX3_GET_SCREEN_SIZE = 4,
	FB_GFX3_GET_SCREEN_DEPTH = 5,
	FB_GFX3_GET_SCREEN_BPP = 6,
	FB_GFX3_GET_SCREEN_PITCH = 7,
	FB_GFX3_GET_SCREEN_REFRESH = 8,
	FB_GFX3_GET_DRIVER_NAME = 9,
	FB_GFX3_GET_TRANSPARENT_COLOR = 10,
	FB_GFX3_GET_VIEWPORT = 11,
	FB_GFX3_GET_PEN_POS = 12,
	FB_GFX3_GET_COLOR = 13,
	FB_GFX3_GET_ALPHA_PRIMITIVES = 14,
	FB_GFX3_GET_GL_EXTENSIONS = 15,
	FB_GFX3_GET_HIGH_PRIORITY = 16,
	FB_GFX3_GET_SCANLINE_SIZE = 17,
	FB_GFX3_GET_X86_MMX_ENABLED = 18,
	FB_GFX3_GET_GL_COLOR_BITS = 37,
	FB_GFX3_GET_GL_COLOR_RED_BITS = 38,
	FB_GFX3_GET_GL_COLOR_GREEN_BITS = 39,
	FB_GFX3_GET_GL_COLOR_BLUE_BITS = 40,
	FB_GFX3_GET_GL_COLOR_ALPHA_BITS = 41,
	FB_GFX3_GET_GL_DEPTH_BITS = 42,
	FB_GFX3_GET_GL_STENCIL_BITS = 43,
	FB_GFX3_GET_GL_ACCUM_BITS = 44,
	FB_GFX3_GET_GL_ACCUM_RED_BITS = 45,
	FB_GFX3_GET_GL_ACCUM_GREEN_BITS = 46,
	FB_GFX3_GET_GL_ACCUM_BLUE_BITS = 47,
	FB_GFX3_GET_GL_ACCUM_ALPHA_BITS = 48,
	FB_GFX3_GET_GL_NUM_SAMPLES = 49,
	FB_GFX3_GET_GL_2D_MODE = 82,
	FB_GFX3_GET_GL_SCALE = 83,
	FB_GFX3_SET_WINDOW_POS = 100,
	FB_GFX3_SET_WINDOW_TITLE = 101,
	FB_GFX3_SET_PEN_POS = 102,
	FB_GFX3_SET_DRIVER_NAME = 103,
	FB_GFX3_SET_ALPHA_PRIMITIVES = 104,
	FB_GFX3_SET_GL_COLOR_BITS = 105,
	FB_GFX3_SET_GL_COLOR_RED_BITS = 106,
	FB_GFX3_SET_GL_COLOR_GREEN_BITS = 107,
	FB_GFX3_SET_GL_COLOR_BLUE_BITS = 108,
	FB_GFX3_SET_GL_COLOR_ALPHA_BITS = 109,
	FB_GFX3_SET_GL_DEPTH_BITS = 110,
	FB_GFX3_SET_GL_STENCIL_BITS = 111,
	FB_GFX3_SET_GL_ACCUM_BITS = 112,
	FB_GFX3_SET_GL_ACCUM_RED_BITS = 113,
	FB_GFX3_SET_GL_ACCUM_GREEN_BITS = 114,
	FB_GFX3_SET_GL_ACCUM_BLUE_BITS = 115,
	FB_GFX3_SET_GL_ACCUM_ALPHA_BITS = 116,
	FB_GFX3_SET_GL_NUM_SAMPLES = 117,
	FB_GFX3_SET_X86_MMX_ENABLED = 118,
	FB_GFX3_SET_GL_2D_MODE = 150,
	FB_GFX3_SET_GL_SCALE = 151,
	FB_GFX3_POLL_EVENTS = 200
};

/*
	gfxlib2 keeps these values independently from a live GL context: programs
	can configure a future mode before SCREENRES, and a setter made while a mode
	is active changes the compatibility query without mutating that context.

	gfxlib3 has the same state boundary. Its compute renderers always retain
	their own context and presentation format, so this state is never used to
	issue unsynchronized GL calls or alter a live pipeline.
*/
typedef struct FB_GFX3_GL_CONTROL_VALUES {
	ssize_t color_bits;
	ssize_t color_red_bits;
	ssize_t color_green_bits;
	ssize_t color_blue_bits;
	ssize_t color_alpha_bits;
	ssize_t depth_bits;
	ssize_t stencil_bits;
	ssize_t accum_bits;
	ssize_t accum_red_bits;
	ssize_t accum_green_bits;
	ssize_t accum_blue_bits;
	ssize_t accum_alpha_bits;
	ssize_t samples;
	ssize_t init_mode_2d;
	ssize_t mode_2d;
	ssize_t init_scale;
	ssize_t scale;
} FB_GFX3_GL_CONTROL_VALUES;

static FB_GFX3_GL_CONTROL_VALUES gl_control_values = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 1, 1
};

/* ------------------------------------------------------------------------- */
/* Shared mode information                                                   */
/* ------------------------------------------------------------------------- */

static uint32_t query_bytes_per_pixel(uint32_t depth)
{
	return (depth + 7u) / 8u;
}

static const char *query_driver_name(const FB_GFX3_MODE *mode)
{
	const FB_GFX3_BACKEND_VTABLE *backend;

	if (mode == NULL)
		return "";
	backend = mode->context.renderer.backend_vtable;
	if ((backend == NULL) || (backend->name == NULL))
		return "gfxlib3";
	return backend->name;
}

static void query_assign_string(FBSTRING *destination, const char *value)
{
	FBSTRING *source;
	size_t length;

	if ((destination == NULL) || (value == NULL))
		return;
	length = strlen(value);
	if (length >= (size_t)SSIZE_MAX)
		return;
	source = fb_StrAllocTempDescF(value, (ssize_t)(length + 1u));
	if (source != NULL)
		fb_StrAssign(destination, -1, source, -1, FB_FALSE);
}

static const FB_GFX3_BACKEND_GL_INFO *query_gl_info(
	const FB_GFX3_MODE *mode)
{
	const FB_GFX3_BACKEND *backend;

	if (mode == NULL)
		return NULL;
	backend = &mode->context.renderer.backend;
	return backend->gl_info.available ? &backend->gl_info : NULL;
}

/* ------------------------------------------------------------------------- */
/* Legacy GL control state                                                   */
/* ------------------------------------------------------------------------- */

static int query_gl_control_get(int what, ssize_t *value)
{
	if (value == NULL)
		return -1;
	switch (what) {
	case FB_GFX3_GET_GL_COLOR_BITS:
		*value = gl_control_values.color_bits;
		break;
	case FB_GFX3_GET_GL_COLOR_RED_BITS:
		*value = gl_control_values.color_red_bits;
		break;
	case FB_GFX3_GET_GL_COLOR_GREEN_BITS:
		*value = gl_control_values.color_green_bits;
		break;
	case FB_GFX3_GET_GL_COLOR_BLUE_BITS:
		*value = gl_control_values.color_blue_bits;
		break;
	case FB_GFX3_GET_GL_COLOR_ALPHA_BITS:
		*value = gl_control_values.color_alpha_bits;
		break;
	case FB_GFX3_GET_GL_DEPTH_BITS:
		*value = gl_control_values.depth_bits;
		break;
	case FB_GFX3_GET_GL_STENCIL_BITS:
		*value = gl_control_values.stencil_bits;
		break;
	case FB_GFX3_GET_GL_ACCUM_BITS:
		*value = gl_control_values.accum_bits;
		break;
	case FB_GFX3_GET_GL_ACCUM_RED_BITS:
		*value = gl_control_values.accum_red_bits;
		break;
	case FB_GFX3_GET_GL_ACCUM_GREEN_BITS:
		*value = gl_control_values.accum_green_bits;
		break;
	case FB_GFX3_GET_GL_ACCUM_BLUE_BITS:
		*value = gl_control_values.accum_blue_bits;
		break;
	case FB_GFX3_GET_GL_ACCUM_ALPHA_BITS:
		*value = gl_control_values.accum_alpha_bits;
		break;
	case FB_GFX3_GET_GL_NUM_SAMPLES:
		*value = gl_control_values.samples;
		break;
	case FB_GFX3_GET_GL_2D_MODE:
		*value = gl_control_values.mode_2d;
		break;
	case FB_GFX3_GET_GL_SCALE:
		*value = gl_control_values.scale;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

static int query_compat_control_set(int what, ssize_t value)
{
	switch (what) {
	case FB_GFX3_SET_GL_COLOR_BITS:
		gl_control_values.color_bits = value;
		break;
	case FB_GFX3_SET_GL_COLOR_RED_BITS:
		gl_control_values.color_red_bits = value;
		break;
	case FB_GFX3_SET_GL_COLOR_GREEN_BITS:
		gl_control_values.color_green_bits = value;
		break;
	case FB_GFX3_SET_GL_COLOR_BLUE_BITS:
		gl_control_values.color_blue_bits = value;
		break;
	case FB_GFX3_SET_GL_COLOR_ALPHA_BITS:
		gl_control_values.color_alpha_bits = value;
		break;
	case FB_GFX3_SET_GL_DEPTH_BITS:
		gl_control_values.depth_bits = value;
		break;
	case FB_GFX3_SET_GL_STENCIL_BITS:
		gl_control_values.stencil_bits = value;
		break;
	case FB_GFX3_SET_GL_ACCUM_BITS:
		gl_control_values.accum_bits = value;
		break;
	case FB_GFX3_SET_GL_ACCUM_RED_BITS:
		gl_control_values.accum_red_bits = value;
		break;
	case FB_GFX3_SET_GL_ACCUM_GREEN_BITS:
		gl_control_values.accum_green_bits = value;
		break;
	case FB_GFX3_SET_GL_ACCUM_BLUE_BITS:
		gl_control_values.accum_blue_bits = value;
		break;
	case FB_GFX3_SET_GL_ACCUM_ALPHA_BITS:
		gl_control_values.accum_alpha_bits = value;
		break;
	case FB_GFX3_SET_GL_NUM_SAMPLES:
		gl_control_values.samples = value;
		break;
	case FB_GFX3_SET_GL_2D_MODE:
		gl_control_values.init_mode_2d = value;
		break;
	case FB_GFX3_SET_GL_SCALE:
		gl_control_values.init_scale = value;
		break;
	case FB_GFX3_SET_X86_MMX_ENABLED:
		/*
			gfxlib3 never selects a CPU MMX blitter. Keep the legacy setter
			accepted on every platform, while GET_X86_MMX_ENABLED remains false
			because no renderer work can be switched by this compatibility knob.
		*/
		(void)value;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void fb_gfx3_api_gl_control_mode_opened_locked(const FB_GFX3_MODE *mode)
{
	const FB_GFX3_BACKEND_GL_INFO *info = query_gl_info(mode);

	if (info == NULL)
		return;
	gl_control_values.color_bits = (ssize_t)info->color_bits;
	gl_control_values.color_red_bits = (ssize_t)info->color_red_bits;
	gl_control_values.color_green_bits = (ssize_t)info->color_green_bits;
	gl_control_values.color_blue_bits = (ssize_t)info->color_blue_bits;
	gl_control_values.color_alpha_bits = (ssize_t)info->color_alpha_bits;
	gl_control_values.depth_bits = (ssize_t)info->depth_bits;
	gl_control_values.stencil_bits = (ssize_t)info->stencil_bits;
	gl_control_values.accum_bits = (ssize_t)info->accum_bits;
	gl_control_values.accum_red_bits = (ssize_t)info->accum_red_bits;
	gl_control_values.accum_green_bits = (ssize_t)info->accum_green_bits;
	gl_control_values.accum_blue_bits = (ssize_t)info->accum_blue_bits;
	gl_control_values.accum_alpha_bits = (ssize_t)info->accum_alpha_bits;
	gl_control_values.samples = (ssize_t)info->samples;
	/* gfxlib3 does not expose gfxlib2's legacy direct-GL 2D modes. */
	gl_control_values.mode_2d = 0;
	gl_control_values.scale = 1;
}

/*
	gfxlib2 reports the desktop mode when SCREENINFO is called before SCREEN or
	SCREENRES. The GPU context does not exist at that point, so this query must
	go through the selected native adapter instead of renderer-owned state.
*/
static void query_desktop_info(ssize_t *width, ssize_t *height,
	ssize_t *depth, ssize_t *refresh)
{
	const FB_GFX3_PLATFORM_VTABLE *platform = fb_gfx3_platform_default();

	if (width != NULL)
		*width = 0;
	if (height != NULL)
		*height = 0;
	if (depth != NULL)
		*depth = 0;
	if (refresh != NULL)
		*refresh = 0;
	if ((platform != NULL) && (platform->desktop_info != NULL))
		platform->desktop_info(width, height, depth, refresh);
}

FBCALL void fb_GfxScreenInfo(ssize_t *width, ssize_t *height,
	ssize_t *depth, ssize_t *bytes_per_pixel, ssize_t *pitch,
	ssize_t *refresh, FBSTRING *driver)
{
	FB_GFX3_DRAW_STATE *state;
	const char *driver_name = "";

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state == NULL) {
		query_desktop_info(width, height, depth, refresh);
		if (bytes_per_pixel != NULL)
			*bytes_per_pixel = 0;
		if (pitch != NULL)
			*pitch = 0;
	} else {
		FB_GFX3_MODE *mode = state->mode;
		uint32_t bpp = query_bytes_per_pixel(mode->depth);

		if (width != NULL)
			*width = mode->width;
		if (height != NULL)
			*height = mode->height;
		if (depth != NULL)
			*depth = mode->depth;
		if (bytes_per_pixel != NULL)
			*bytes_per_pixel = bpp;
		if (pitch != NULL)
			*pitch = (ssize_t)mode->width * bpp;
		if (refresh != NULL)
			*refresh = 0;
		driver_name = query_driver_name(mode);
	}
	if ((driver != NULL) && fb_hStrDelTemp(driver)) {
		size_t length = strlen(driver_name);

		if (fb_hStrRealloc(driver, (ssize_t)length, FB_FALSE) != NULL)
			memcpy(driver->data, driver_name, length + 1);
	}
	FB_GRAPHICS_UNLOCK();
}

FBCALL void fb_GfxScreenInfo32(int *width, int *height, int *depth,
	int *bytes_per_pixel, int *pitch, int *refresh, FBSTRING *driver)
{
	ssize_t native_width = 0;
	ssize_t native_height = 0;
	ssize_t native_depth = 0;
	ssize_t native_bpp = 0;
	ssize_t native_pitch = 0;
	ssize_t native_refresh = 0;

	fb_GfxScreenInfo(&native_width, &native_height, &native_depth,
		&native_bpp, &native_pitch, &native_refresh, driver);
	if (width != NULL)
		*width = (int)native_width;
	if (height != NULL)
		*height = (int)native_height;
	if (depth != NULL)
		*depth = (int)native_depth;
	if (bytes_per_pixel != NULL)
		*bytes_per_pixel = (int)native_bpp;
	if (pitch != NULL)
		*pitch = (int)native_pitch;
	if (refresh != NULL)
		*refresh = (int)native_refresh;
}

FBCALL void fb_GfxScreenInfo64(long long *width, long long *height,
	long long *depth, long long *bytes_per_pixel, long long *pitch,
	long long *refresh, FBSTRING *driver)
{
	ssize_t native_width = 0;
	ssize_t native_height = 0;
	ssize_t native_depth = 0;
	ssize_t native_bpp = 0;
	ssize_t native_pitch = 0;
	ssize_t native_refresh = 0;

	fb_GfxScreenInfo(&native_width, &native_height, &native_depth,
		&native_bpp, &native_pitch, &native_refresh, driver);
	if (width != NULL)
		*width = (long long)native_width;
	if (height != NULL)
		*height = (long long)native_height;
	if (depth != NULL)
		*depth = (long long)native_depth;
	if (bytes_per_pixel != NULL)
		*bytes_per_pixel = (long long)native_bpp;
	if (pitch != NULL)
		*pitch = (long long)native_pitch;
	if (refresh != NULL)
		*refresh = (long long)native_refresh;
}

/* ------------------------------------------------------------------------- */
/* Standard SCREEN mode iterator                                             */
/* ------------------------------------------------------------------------- */

/*
	SCREENLIST has an iterator ABI: a positive depth starts a new list and
	subsequent zero-depth calls return its remaining entries.  gfxlib2 obtains
	that list from its platform drivers.  gfxlib3 is windowed and accepts an
	arbitrary SCREENRES extent, so it cannot honestly present a finite list of
	physical display modes.  On Win32 it asks the platform boundary for those
	modes first.  The static table remains the deterministic fallback for a
	headless host, a platform without a native enumerator, or historical depths
	that the current desktop does not advertise.

	The entries are pre-sorted by packed width/height.  Keeping the table here
	avoids an allocation, provides deterministic headless behavior, and retains
	the restart/resume contract that old programs rely on.
*/
enum FB_GFX3_SCREENLIST_DEPTH {
	FB_GFX3_SCREENLIST_1 = 1u << 0,
	FB_GFX3_SCREENLIST_2 = 1u << 1,
	FB_GFX3_SCREENLIST_4 = 1u << 2,
	FB_GFX3_SCREENLIST_8 = 1u << 3,
	FB_GFX3_SCREENLIST_16 = 1u << 4,
	FB_GFX3_SCREENLIST_32 = 1u << 5
};

typedef struct FB_GFX3_SCREENLIST_MODE {
	uint16_t width;
	uint16_t height;
	uint8_t supported_depths;
} FB_GFX3_SCREENLIST_MODE;

static const FB_GFX3_SCREENLIST_MODE query_screenlist_modes[] = {
	{ 320, 200, FB_GFX3_SCREENLIST_2 | FB_GFX3_SCREENLIST_4 |
		FB_GFX3_SCREENLIST_8 },
	{ 320, 240, FB_GFX3_SCREENLIST_1 | FB_GFX3_SCREENLIST_2 |
		FB_GFX3_SCREENLIST_4 | FB_GFX3_SCREENLIST_8 |
		FB_GFX3_SCREENLIST_16 | FB_GFX3_SCREENLIST_32 },
	{ 400, 300, FB_GFX3_SCREENLIST_1 | FB_GFX3_SCREENLIST_2 |
		FB_GFX3_SCREENLIST_4 | FB_GFX3_SCREENLIST_8 |
		FB_GFX3_SCREENLIST_16 | FB_GFX3_SCREENLIST_32 },
	{ 512, 384, FB_GFX3_SCREENLIST_1 | FB_GFX3_SCREENLIST_2 |
		FB_GFX3_SCREENLIST_4 | FB_GFX3_SCREENLIST_8 |
		FB_GFX3_SCREENLIST_16 | FB_GFX3_SCREENLIST_32 },
	{ 640, 200, FB_GFX3_SCREENLIST_1 | FB_GFX3_SCREENLIST_4 },
	{ 640, 350, FB_GFX3_SCREENLIST_1 | FB_GFX3_SCREENLIST_4 },
	{ 640, 400, FB_GFX3_SCREENLIST_1 | FB_GFX3_SCREENLIST_2 |
		FB_GFX3_SCREENLIST_4 | FB_GFX3_SCREENLIST_8 |
		FB_GFX3_SCREENLIST_16 | FB_GFX3_SCREENLIST_32 },
	{ 640, 480, FB_GFX3_SCREENLIST_1 | FB_GFX3_SCREENLIST_2 |
		FB_GFX3_SCREENLIST_4 | FB_GFX3_SCREENLIST_8 |
		FB_GFX3_SCREENLIST_16 | FB_GFX3_SCREENLIST_32 },
	{ 800, 600, FB_GFX3_SCREENLIST_1 | FB_GFX3_SCREENLIST_2 |
		FB_GFX3_SCREENLIST_4 | FB_GFX3_SCREENLIST_8 |
		FB_GFX3_SCREENLIST_16 | FB_GFX3_SCREENLIST_32 },
	{ 1024, 768, FB_GFX3_SCREENLIST_1 | FB_GFX3_SCREENLIST_2 |
		FB_GFX3_SCREENLIST_4 | FB_GFX3_SCREENLIST_8 |
		FB_GFX3_SCREENLIST_16 | FB_GFX3_SCREENLIST_32 },
	{ 1280, 1024, FB_GFX3_SCREENLIST_1 | FB_GFX3_SCREENLIST_2 |
		FB_GFX3_SCREENLIST_4 | FB_GFX3_SCREENLIST_8 |
		FB_GFX3_SCREENLIST_16 | FB_GFX3_SCREENLIST_32 }
};

static uint8_t query_screenlist_depth_mask(int depth)
{
	switch (depth) {
	case 1:
		return FB_GFX3_SCREENLIST_1;
	case 2:
		return FB_GFX3_SCREENLIST_2;
	case 4:
		return FB_GFX3_SCREENLIST_4;
	case 8:
		return FB_GFX3_SCREENLIST_8;
	case 16:
	case 15:
		return FB_GFX3_SCREENLIST_16;
	case 32:
	case 24:
		return FB_GFX3_SCREENLIST_32;
	default:
		return 0u;
	}
}

FBCALL int fb_GfxScreenList(int depth)
{
	static uint8_t requested_depth;
	static size_t current;
	static int *native_modes;
	static size_t native_mode_count;
	int result = 0;

	FB_GRAPHICS_LOCK();
	if (depth > 0) {
		free(native_modes);
		native_modes = NULL;
		native_mode_count = 0;
		requested_depth = query_screenlist_depth_mask(depth);
		current = 0u;
		if (requested_depth != 0u)
			fb_gfx3_platform_screenlist_modes(depth, &native_modes,
				&native_mode_count);
	}
	if (requested_depth != 0u) {
		if (native_mode_count > 0u) {
			if (current < native_mode_count)
				result = native_modes[current++];
			FB_GRAPHICS_UNLOCK();
			return result;
		}
		while (current < (sizeof(query_screenlist_modes) /
			sizeof(query_screenlist_modes[0]))) {
			const FB_GFX3_SCREENLIST_MODE *mode =
				&query_screenlist_modes[current++];

			if ((mode->supported_depths & requested_depth) == 0u)
				continue;
			result = ((int)mode->width << 16) | mode->height;
			break;
		}
	}
	FB_GRAPHICS_UNLOCK();
	return result;
}

/* ------------------------------------------------------------------------- */
/* Synchronized screen shadow                                                */
/* ------------------------------------------------------------------------- */

static int query_ensure_shadow(FB_GFX3_DRAW_STATE *state)
{
	FB_GFX3_MODE *mode;
	uint32_t page;
	uint32_t bpp;
	uint64_t row_bytes;
	size_t allocation_size;
	int result;

	if ((state == NULL) || (state->mode == NULL))
		return FB_GFX3_INVALID;
	mode = state->mode;
	page = state->work_page;
	bpp = query_bytes_per_pixel(mode->depth);
	row_bytes = (uint64_t)mode->width * bpp;
	if ((mode->width == 0u) || (mode->height == 0u) || (bpp == 0u) ||
	    (mode->shadow_pages == NULL) || (mode->shadow_valid == NULL) ||
	    (mode->pages == NULL) || (page >= mode->page_count) ||
	    (row_bytes > UINT32_MAX) ||
	    (fb_gfx3_size_multiply((size_t)row_bytes, mode->height,
	     &allocation_size) != FB_GFX3_OK) || (allocation_size == 0u))
		return FB_GFX3_INVALID;
	mode->shadow_pitch = (uint32_t)row_bytes;
	if (mode->shadow_pages[page] == NULL) {
		mode->shadow_pages[page] = (unsigned char *)malloc(allocation_size);
		if (mode->shadow_pages[page] == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		mode->shadow_valid[page] = FALSE;
	}
	if (mode->shadow_valid[page])
		return FB_GFX3_OK;
	result = fb_gfx3_surface_download(&mode->pages[page], 0, 0,
		mode->width, mode->height, mode->shadow_pitch,
		mode->shadow_pages[page]);
	if (result == FB_GFX3_OK)
		mode->shadow_valid[page] = TRUE;
	return result;
}

/*
	A nested SCREENLOCK caller can alter any byte without calling back into
	gfxlib. Keep a private pre-nested-lock copy so its SCREENUNLOCK can submit
	the actual changed rows instead of pessimistically uploading the entire
	display page. The copy is retained per page: allocations occur at most once,
	while the active marker defines the lifetime of one nested transaction.

	The overwhelmingly common single lock does not need this copy. Its eventual
	GPU ordering boundary already commits the dirty shadow as one full upload,
	and copying every screen row at SCREENLOCK time makes one-byte SCREENPTR
	writes needlessly CPU-bound.

	Failure to allocate the optional comparison copy must not make SCREENPTR
	stop working.  SCREENUNLOCK falls back to its documented caller-provided
	line range in that low-memory case.
*/
static void query_begin_shadow_snapshot_locked(FB_GFX3_DRAW_STATE *state)
{
	FB_GFX3_MODE *mode;
	uint32_t page;
	size_t size;

	if ((state == NULL) || ((mode = state->mode) == NULL) ||
	    (mode->shadow_pages == NULL) || (mode->shadow_snapshots == NULL) ||
	    (mode->shadow_snapshot_active == NULL))
		return;
	page = state->work_page;
	if ((page >= mode->page_count) || (mode->shadow_pages[page] == NULL) ||
	    (mode->shadow_pitch == 0u) ||
	    (fb_gfx3_size_multiply(mode->shadow_pitch, mode->height, &size) !=
	     FB_GFX3_OK))
		return;
	if (mode->shadow_snapshots[page] == NULL) {
		mode->shadow_snapshots[page] = (unsigned char *)malloc(size);
		if (mode->shadow_snapshots[page] == NULL)
			return;
	}
	memcpy(mode->shadow_snapshots[page], mode->shadow_pages[page], size);
	mode->shadow_snapshot_active[page] = TRUE;
}

/*
	Return the smallest inclusive row range whose pixels changed since the last
	snapshot.  A range upload is preferable to many one-row transfers because
	the upload remains one ordered renderer command, while still avoiding the
	full-page traffic caused by small SCREENPTR edits.
*/
static int query_shadow_changed_range_locked(const FB_GFX3_MODE *mode,
	uint32_t page, uint32_t *first_line, uint32_t *last_line)
{
	uint32_t line;
	uint32_t first = UINT32_MAX;
	uint32_t last = 0u;

	if ((mode == NULL) || (first_line == NULL) || (last_line == NULL) ||
	    (page >= mode->page_count) || (mode->shadow_pages == NULL) ||
	    (mode->shadow_snapshots == NULL) ||
	    (mode->shadow_snapshot_active == NULL) ||
	    !mode->shadow_snapshot_active[page] ||
	    (mode->shadow_pages[page] == NULL) ||
	    (mode->shadow_snapshots[page] == NULL) ||
	    (mode->shadow_pitch == 0u))
		return FALSE;
	for (line = 0u; line < mode->height; line++) {
		size_t offset = (size_t)line * mode->shadow_pitch;

		if (memcmp(mode->shadow_pages[page] + offset,
		    mode->shadow_snapshots[page] + offset,
		    mode->shadow_pitch) == 0)
			continue;
		if (line < first)
			first = line;
		last = line;
	}
	if (first == UINT32_MAX)
		return 0;
	*first_line = first;
	*last_line = last;
	return 1;
}

static void query_update_shadow_snapshot_locked(FB_GFX3_MODE *mode,
	uint32_t page, uint32_t first_line, uint32_t last_line)
{
	size_t offset;
	size_t size;

	if ((mode == NULL) || (page >= mode->page_count) ||
	    (first_line > last_line) || (last_line >= mode->height) ||
	    (mode->shadow_pages == NULL) || (mode->shadow_snapshots == NULL) ||
	    (mode->shadow_pages[page] == NULL) ||
	    (mode->shadow_snapshots[page] == NULL) ||
	    (mode->shadow_pitch == 0u))
		return;
	offset = (size_t)first_line * mode->shadow_pitch;
	if (fb_gfx3_size_multiply((size_t)last_line - first_line + 1u,
	    mode->shadow_pitch, &size) != FB_GFX3_OK)
		return;
	memcpy(mode->shadow_snapshots[page] + offset,
		mode->shadow_pages[page] + offset, size);
}

FBCALL void fb_GfxLock(void)
{
	FB_GFX3_DRAW_STATE *state;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state == NULL)
		return;
	fb_MutexLock(state->mode->mutex);
	/*
		SCREENLOCK is commonly used only to group ordinary graphics calls. It
		does not itself expose a framebuffer pointer, so downloading the complete
		GPU page here wastes a transfer and stalls the BASIC thread in programs
		which never call SCREENPTR.

		If an outer lock has already exposed writable memory, preserve the nested
		transaction snapshot before increasing the depth. SCREENPTR handles the
		other case, where the first pointer is requested after nesting began.
	*/
	if ((state->mode->access_lock_count != 0u) &&
	    (state->work_page < state->mode->page_count) &&
	    (state->mode->shadow_dirty != NULL) &&
	    state->mode->shadow_dirty[state->work_page])
		query_begin_shadow_snapshot_locked(state);
	state->mode->access_lock_count++;
	fb_MutexUnlock(state->mode->mutex);
}

FBCALL void fb_GfxUnlock(int start_line, int end_line)
{
	FB_GFX3_DRAW_STATE *state = fb_gfx3_api_get_draw_state_locked();
	int commit = FALSE;
	int flush = FALSE;
	int present = FALSE;
	int submit = FALSE;

	if (state != NULL) {
		FB_GFX3_MODE *mode = state->mode;
		uint32_t page = state->work_page;
		uint32_t first_changed_line;
		uint32_t last_changed_line;

		fb_MutexLock(mode->mutex);
		/*
			A lock by itself does not make the CPU shadow authoritative.  Ordinary
			gfxlib drawing is still legal between SCREENLOCK and SCREENUNLOCK, and
			those commands write directly to the GPU page.  Upload only after
			SCREENPTR exposed the shadow to the caller; otherwise an untouched
			shadow would overwrite the just-drawn GPU frame.

			Keep the marker through nested unlocks.  A pointer acquired by an outer
			lock remains writable after an inner SCREENUNLOCK, so every requested
			range must upload until that outer lock is released.
		*/
		/*
			An outer SCREENUNLOCK need not immediately copy CPU memory to the
			device.  A following SCREENLOCK can continue using the valid shadow;
			the next GPU primitive, page operation, or presentation commits it in
			order.  Nested locks remain eager because their outer pointer is still
			live and a caller may draw between the two unlocks.
		*/
		if ((mode->access_lock_count > 1u) && mode->shadow_dirty[page]) {
			int changed = query_shadow_changed_range_locked(mode, page,
				&first_changed_line, &last_changed_line);

			if (changed <= 0) {
				/*
					The optional snapshot is absent for the normal first lock, or
					could not be allocated for this nested lock. The public unlock
					range is the compatibility fallback in both cases.
				*/
				if (start_line < 0)
					start_line = 0;
				if (end_line < 0)
					end_line = (int)mode->height - 1;
				if (start_line < 0)
					start_line = 0;
				if (end_line >= (int)mode->height)
					end_line = (int)mode->height - 1;
				if (start_line <= end_line) {
					first_changed_line = (uint32_t)start_line;
					last_changed_line = (uint32_t)end_line;
					changed = TRUE;
				}
			}
			if (changed > 0) {
				uint32_t height = last_changed_line - first_changed_line + 1u;

				if (fb_gfx3_surface_upload(&mode->pages[page], 0,
				    (int)first_changed_line, mode->width, height,
				    mode->shadow_pitch, mode->shadow_pages[page] +
				    ((size_t)first_changed_line * mode->shadow_pitch)) ==
				    FB_GFX3_OK) {
					mode->shadow_valid[page] = TRUE;
					query_update_shadow_snapshot_locked(mode, page,
						first_changed_line, last_changed_line);
				}
			}
		}
		if (mode->access_lock_count > 0) {
			mode->access_lock_count--;
			if (mode->access_lock_count == 0) {
				if (mode->shadow_snapshot_active != NULL)
					mode->shadow_snapshot_active[page] = FALSE;
				/*
					An outer SCREENUNLOCK is the frame boundary used by programs
					which call SLEEP with event processing disabled. gfxlib2's
					driver thread refreshes such a frame without a view-update
					hook. gfxlib3 must therefore present the visible page here.

					GPU-only drawing remains asynchronous. If SCREENPTR exposed
					writable memory, upload its dirty rows first; direct CPU
					writes cannot become visible without that transfer.
				*/
				commit = mode->shadow_dirty[page];
				submit = !commit;
				present = page == mode->visible_page;
			} else {
				flush = TRUE;
			}
		}
		fb_gfx3_log_write(&mode->context.logger, FB_GFX3_LOG_TRACE,
			"SCREENUNLOCK complete: page %u, dirty %d, commit %d, "
			"present %d, submit %d, flush %d",
			page, mode->shadow_dirty[page] != 0, commit, present, submit,
			flush);
		fb_MutexUnlock(mode->mutex);
		/*
			Nested SCREENLOCK still has a live outer pointer and therefore keeps
			its completion. Outer presentation remains asynchronous; POINT, GET,
			and SCREENSYNC retain their explicit completion boundaries.
		*/
		if (commit &&
		    (fb_gfx3_compat_commit_shadow(state) != FB_GFX3_OK)) {
			commit = FALSE;
			submit = FALSE;
			present = FALSE;
		}
		if ((commit || submit || flush) &&
		    (fb_gfx3_compat_flush_points(state) == FB_GFX3_OK)) {
			if (flush) {
				fb_gfx3_context_flush(&mode->context);
			} else if (present) {
				fb_gfx3_surface_present(&mode->pages[page], FALSE);
			} else {
				fb_gfx3_context_submit_pending(&mode->context);
			}
		}
	}
	FB_GRAPHICS_UNLOCK();
}

FBCALL void *fb_GfxScreenPtr(void)
{
	FB_GFX3_DRAW_STATE *state;
	void *result = NULL;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state != NULL) {
		FB_GFX3_MODE *mode = state->mode;

		/*
			PSET batches remain caller-local until an ordering boundary.
			SCREENPTR is such a boundary: its CPU view must include every pixel
			drawn before the call. Submit the batch before a possible page
			download, while the runtime graphics lock still serializes mode and
			page changes.
		*/
		if (fb_gfx3_compat_flush_points_graphics_locked(state) !=
		    FB_GFX3_OK)
			goto unlock;
		fb_MutexLock(mode->mutex);
		if (query_ensure_shadow(state) == FB_GFX3_OK) {
			/*
				A pointer first acquired inside a nested lock still needs a
				pre-write snapshot. The caller can modify it before the inner
				SCREENUNLOCK, then continue using the same pointer in the outer
				lock.
			*/
			if ((mode->access_lock_count > 1u) &&
			    (mode->shadow_snapshot_active != NULL) &&
			    !mode->shadow_snapshot_active[state->work_page])
				query_begin_shadow_snapshot_locked(state);
			result = mode->shadow_pages[state->work_page];
			/*
				SCREENPTR returns writable storage even when the caller did not
				first use SCREENLOCK. Old software renderers rely on that form and
				expect SCREENCOPY or the next graphics operation to observe their
				raw writes. The runtime cannot see which bytes change, so mark the
				full page conservatively at pointer exposure.
			*/
			mode->shadow_dirty[state->work_page] = TRUE;
			if (mode->shadow_dirty_first_line != NULL)
				mode->shadow_dirty_first_line[state->work_page] = 0u;
			if (mode->shadow_dirty_last_line != NULL)
				mode->shadow_dirty_last_line[state->work_page] =
					mode->height - 1u;
			/*
				Writable CPU memory can invalidate a cached POINT value even
				before the changed rows return to the GPU.
			*/
			if (mode->point_cache != NULL)
				mode->point_cache[state->work_page].valid = FALSE;
		}
		fb_MutexUnlock(mode->mutex);
	}

unlock:
	FB_GRAPHICS_UNLOCK();
	return result;
}

FBCALL int fb_GfxWaitVSync(void)
{
	FB_GFX3_DRAW_STATE *state;
	int result;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state == NULL) {
		result = FB_GFX3_INVALID;
	} else {
		/*
			PSET batches are intentionally retained on the BASIC thread until
			an ordering boundary. SCREENSYNC is the explicit display boundary,
			so presenting before this flush would omit the final point packet
			from the frame even though the synchronous call had completed.
		*/
		result = fb_gfx3_compat_flush_points_graphics_locked(state);
		if (result == FB_GFX3_OK)
			result = fb_gfx3_compat_commit_shadow(state);
		if (result == FB_GFX3_OK)
			result = fb_gfx3_surface_present(
				&state->mode->pages[state->mode->visible_page], TRUE);
	}
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

/* ------------------------------------------------------------------------- */
/* SCREENCONTROL and empty event queue                                       */
/* ------------------------------------------------------------------------- */

FBCALL void fb_GfxControl_s(int what, FBSTRING *parameter)
{
	FB_GFX3_DRAW_STATE *state;
	const FB_GFX3_BACKEND_GL_INFO *gl_info;

	if (parameter == NULL)
		return;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	gl_info = (state == NULL) ? NULL : query_gl_info(state->mode);
	switch (what) {
	case FB_GFX3_GET_WINDOW_TITLE:
		query_assign_string(parameter,
			fb_gfx3_api_get_window_title_locked());
		break;
	case FB_GFX3_GET_DRIVER_NAME:
		query_assign_string(parameter,
			(state == NULL) ? "" : query_driver_name(state->mode));
		break;
	case FB_GFX3_GET_GL_EXTENSIONS:
		query_assign_string(parameter,
			(gl_info == NULL) ? "" : gl_info->extensions);
		break;
	case FB_GFX3_SET_WINDOW_TITLE:
		if (parameter->data != NULL) {
			ssize_t length = FB_STRSIZE(parameter);

			if (length >= 0)
				fb_gfx3_api_set_window_title_locked(parameter->data,
					(size_t)length);
		}
		break;
	case FB_GFX3_SET_DRIVER_NAME:
		if (parameter->data != NULL) {
			ssize_t length = FB_STRSIZE(parameter);

			if (length >= 0)
				fb_gfx3_backend_set_requested_name(parameter->data,
					(size_t)length);
		} else {
			fb_gfx3_backend_set_requested_name(NULL, 0);
		}
		break;
	default:
		break;
	}
	FB_GRAPHICS_UNLOCK();
}

FBCALL void fb_GfxControl_i(int what, ssize_t *parameter1,
	ssize_t *parameter2, ssize_t *parameter3, ssize_t *parameter4)
{
	FB_GFX3_DRAW_STATE *state;
	ssize_t ignored1 = (ssize_t)0x80000000;
	ssize_t ignored2 = (ssize_t)0x80000000;
	ssize_t ignored3 = (ssize_t)0x80000000;
	ssize_t ignored4 = (ssize_t)0x80000000;
	ssize_t result1 = 0;
	ssize_t result2 = 0;
	ssize_t result3 = 0;
	ssize_t result4 = 0;

	if (parameter1 == NULL)
		parameter1 = &ignored1;
	if (parameter2 == NULL)
		parameter2 = &ignored2;
	if (parameter3 == NULL)
		parameter3 = &ignored3;
	if (parameter4 == NULL)
		parameter4 = &ignored4;
	FB_GRAPHICS_LOCK();
	if (query_compat_control_set(what, *parameter1)) {
		FB_GRAPHICS_UNLOCK();
		return;
	}
	if (query_gl_control_get(what, &result1)) {
		FB_GRAPHICS_UNLOCK();
		*parameter1 = result1;
		*parameter2 = result2;
		*parameter3 = result3;
		*parameter4 = result4;
		return;
	}
	state = fb_gfx3_api_get_draw_state_locked();
	if (state != NULL) {
		FB_GFX3_MODE *mode = state->mode;
		uint32_t bpp = query_bytes_per_pixel(mode->depth);

		switch (what) {
		case FB_GFX3_GET_WINDOW_POS:
			{
				int x = 0;
				int y = 0;

				fb_gfx3_context_flush(&mode->context);
				if (fb_gfx3_input_window_snapshot(&mode->input,
				    NULL, NULL, &x, &y, NULL, NULL) == FB_GFX3_OK) {
					result1 = x;
					result2 = y;
				}
			}
			break;
		case FB_GFX3_GET_WINDOW_HANDLE:
			{
				uintptr_t native_window = 0;
				uintptr_t native_display = 0;

				fb_gfx3_context_flush(&mode->context);
				if (fb_gfx3_input_window_snapshot(&mode->input,
				    &native_window, &native_display, NULL, NULL,
				    NULL, NULL) == FB_GFX3_OK) {
					result1 = (ssize_t)(intptr_t)native_window;
					result2 = (ssize_t)(intptr_t)native_display;
				}
			}
			break;
		case FB_GFX3_GET_DESKTOP_SIZE:
			{
				int width = 0;
				int height = 0;

				if (fb_gfx3_input_window_snapshot(&mode->input,
				    NULL, NULL, NULL, NULL, &width, &height) ==
				    FB_GFX3_OK) {
					result1 = width;
					result2 = height;
				}
			}
			break;
		case FB_GFX3_GET_SCREEN_SIZE:
			result1 = mode->width;
			result2 = mode->height;
			break;
		case FB_GFX3_GET_SCREEN_DEPTH:
			result1 = mode->depth;
			break;
		case FB_GFX3_GET_SCREEN_BPP:
			result1 = bpp;
			break;
		case FB_GFX3_GET_SCREEN_PITCH:
			result1 = (ssize_t)mode->width * bpp;
			break;
		case FB_GFX3_GET_TRANSPARENT_COLOR:
			result1 = (bpp > 1) ? 0xFF00FF : 0;
			break;
		case FB_GFX3_GET_VIEWPORT:
			result1 = state->view.x1;
			result2 = state->view.y1;
			result3 = state->view.x2;
			result4 = state->view.y2;
			break;
		case FB_GFX3_GET_PEN_POS:
			result1 = CINT(state->last_x);
			result2 = CINT(state->last_y);
			break;
		case FB_GFX3_GET_COLOR:
			result1 = state->foreground_color;
			result2 = state->background_color;
			break;
		case FB_GFX3_GET_ALPHA_PRIMITIVES:
			result1 = mode->alpha_primitives ? FB_TRUE : FB_FALSE;
			break;
		case FB_GFX3_GET_SCANLINE_SIZE:
			result1 = 1;
			break;
		case FB_GFX3_SET_PEN_POS:
			if (*parameter1 != (ssize_t)0x80000000)
				state->last_x = (float)*parameter1;
			if (*parameter2 != (ssize_t)0x80000000)
				state->last_y = (float)*parameter2;
			break;
		case FB_GFX3_SET_ALPHA_PRIMITIVES:
			if (*parameter1 != (ssize_t)0x80000000)
				mode->alpha_primitives = *parameter1 != 0;
			break;
		case FB_GFX3_POLL_EVENTS:
			fb_gfx3_context_flush(&mode->context);
			break;
		case FB_GFX3_SET_WINDOW_POS:
			if (((*parameter1 == (ssize_t)0x80000000) ||
			     ((*parameter1 >= INT_MIN) && (*parameter1 <= INT_MAX))) &&
			    ((*parameter2 == (ssize_t)0x80000000) ||
			     ((*parameter2 >= INT_MIN) && (*parameter2 <= INT_MAX))) &&
			    (fb_gfx3_input_request_window_position(&mode->input,
			     (int)*parameter1, (int)*parameter2) == FB_GFX3_OK))
				fb_gfx3_context_flush(&mode->context);
			break;
		case FB_GFX3_GET_SCREEN_REFRESH:
		case FB_GFX3_GET_HIGH_PRIORITY:
		case FB_GFX3_GET_X86_MMX_ENABLED:
			break;
		default:
			break;
		}
	}
	FB_GRAPHICS_UNLOCK();
	if (what < FB_GFX3_SET_WINDOW_POS) {
		*parameter1 = result1;
		*parameter2 = result2;
		*parameter3 = result3;
		*parameter4 = result4;
	}
}

FBCALL void fb_GfxControl_i32(int what, int *parameter1, int *parameter2,
	int *parameter3, int *parameter4)
{
	ssize_t value1 = (parameter1 == NULL) ? (ssize_t)0x80000000 : *parameter1;
	ssize_t value2 = (parameter2 == NULL) ? (ssize_t)0x80000000 : *parameter2;
	ssize_t value3 = (parameter3 == NULL) ? (ssize_t)0x80000000 : *parameter3;
	ssize_t value4 = (parameter4 == NULL) ? (ssize_t)0x80000000 : *parameter4;

	fb_GfxControl_i(what, &value1, &value2, &value3, &value4);
	if (parameter1 != NULL)
		*parameter1 = (int)value1;
	if (parameter2 != NULL)
		*parameter2 = (int)value2;
	if (parameter3 != NULL)
		*parameter3 = (int)value3;
	if (parameter4 != NULL)
		*parameter4 = (int)value4;
}

FBCALL void fb_GfxControl_i64(int what, long long *parameter1,
	long long *parameter2, long long *parameter3, long long *parameter4)
{
	ssize_t value1 = (parameter1 == NULL) ? (ssize_t)0x80000000 :
		(ssize_t)*parameter1;
	ssize_t value2 = (parameter2 == NULL) ? (ssize_t)0x80000000 :
		(ssize_t)*parameter2;
	ssize_t value3 = (parameter3 == NULL) ? (ssize_t)0x80000000 :
		(ssize_t)*parameter3;
	ssize_t value4 = (parameter4 == NULL) ? (ssize_t)0x80000000 :
		(ssize_t)*parameter4;

	fb_GfxControl_i(what, &value1, &value2, &value3, &value4);
	if (parameter1 != NULL)
		*parameter1 = value1;
	if (parameter2 != NULL)
		*parameter2 = value2;
	if (parameter3 != NULL)
		*parameter3 = value3;
	if (parameter4 != NULL)
		*parameter4 = value4;
}

FBCALL void *fb_GfxGetGLProcAddress(const char *procedure)
{
	return fb_gfx3_renderer_callback_gl_proc(procedure);
}

/* ------------------------------------------------------------------------- */
/* Indexed palette compatibility                                             */
/* ------------------------------------------------------------------------- */

static uint32_t query_palette_color_from_packed(int color)
{
	uint32_t red = ((uint32_t)color & 0x3Fu) * 255u / 63u;
	uint32_t green = (((uint32_t)color >> 8) & 0x3Fu) * 255u / 63u;
	uint32_t blue = (((uint32_t)color >> 16) & 0x3Fu) * 255u / 63u;

	return red | (green << 8) | (blue << 16);
}

static void query_restore_default_palette(FB_GFX3_MODE *mode)
{
	if (mode == NULL)
		return;
	memcpy(mode->palette, mode->default_palette, sizeof(mode->palette));
	if (mode->standard_mode == 4)
		mode->standard_foreground_color = 15u;
}

static void query_publish_palette(FB_GFX3_DRAW_STATE *state)
{
	if ((state != NULL) && (state->mode != NULL))
		fb_gfx3_context_set_palette(&state->mode->context,
			state->mode->palette);
}

FBCALL void fb_GfxPalette(int index, int red, int green, int blue)
{
	FB_GFX3_DRAW_STATE *state;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state != NULL) && (state->mode->depth <= 8)) {
		if (index < 0) {
			query_restore_default_palette(state->mode);
		} else {
			uint32_t entries = 1u << state->mode->depth;
			uint32_t palette_index = (uint32_t)index & (entries - 1u);

			if ((green < 0) || (blue < 0))
				state->mode->palette[palette_index] =
					query_palette_color_from_packed(red);
			else
				state->mode->palette[palette_index] =
					((uint32_t)red & 0xFFu) |
					(((uint32_t)green & 0xFFu) << 8) |
					(((uint32_t)blue & 0xFFu) << 16);
		}
		query_publish_palette(state);
	}
	FB_GRAPHICS_UNLOCK();
}

FBCALL void fb_GfxPaletteGet(int index, int *red, int *green, int *blue)
{
	FB_GFX3_DRAW_STATE *state;

	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state != NULL) && (red != NULL)) {
		uint32_t entries = (state->mode->depth <= 8) ?
			(1u << state->mode->depth) : 256u;
		uint32_t color = state->mode->palette[(uint32_t)index &
			(entries - 1u)];

		if (green == NULL) {
			*red = (int)((color & 0xFCFCFCu) >> 2);
		} else {
			*red = (int)(color & 0xFFu);
			*green = (int)((color >> 8) & 0xFFu);
			if (blue != NULL)
				*blue = (int)((color >> 16) & 0xFFu);
		}
	}
	FB_GRAPHICS_UNLOCK();
}

FBCALL void fb_GfxPaletteGet64(int index, long long *red,
	long long *green, long long *blue)
{
	int red32 = 0;
	int green32 = 0;
	int blue32 = 0;

	if (red == NULL)
		return;
	fb_GfxPaletteGet(index, &red32, (green == NULL) ? NULL : &green32,
		(blue == NULL) ? NULL : &blue32);
	*red = red32;
	if (green != NULL)
		*green = green32;
	if (blue != NULL)
		*blue = blue32;
}

FBCALL void fb_GfxPaletteUsing(int *data)
{
	FB_GFX3_DRAW_STATE *state;
	uint32_t i;

	if (data == NULL)
		return;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state != NULL) && (state->mode->depth <= 8)) {
		uint32_t entries = 1u << state->mode->depth;

		for (i = 0; i < entries; i++) {
			if (data[i] >= 0)
				state->mode->palette[i] =
					query_palette_color_from_packed(data[i]);
		}
		query_publish_palette(state);
	}
	FB_GRAPHICS_UNLOCK();
}

FBCALL void fb_GfxPaletteUsing64(long long *data)
{
	FB_GFX3_DRAW_STATE *state;
	uint32_t i;

	if (data == NULL)
		return;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if ((state != NULL) && (state->mode->depth <= 8)) {
		uint32_t entries = 1u << state->mode->depth;

		for (i = 0; i < entries; i++) {
			if (data[i] >= 0)
				state->mode->palette[i] =
					query_palette_color_from_packed((int)data[i]);
		}
		query_publish_palette(state);
	}
	FB_GRAPHICS_UNLOCK();
}

FBCALL void fb_GfxPaletteGetUsing(int *data)
{
	FB_GFX3_DRAW_STATE *state;
	uint32_t i;

	if (data == NULL)
		return;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state != NULL) {
		uint32_t entries = (state->mode->depth <= 8) ?
			(1u << state->mode->depth) : 256u;

		for (i = 0; i < entries; i++)
			data[i] = (int)((state->mode->palette[i] & 0xFCFCFCu) >> 2);
	}
	FB_GRAPHICS_UNLOCK();
}

FBCALL void fb_GfxPaletteGetUsing64(long long *data)
{
	FB_GFX3_DRAW_STATE *state;
	uint32_t i;

	if (data == NULL)
		return;
	FB_GRAPHICS_LOCK();
	state = fb_gfx3_api_get_draw_state_locked();
	if (state != NULL) {
		uint32_t entries = (state->mode->depth <= 8) ?
			(1u << state->mode->depth) : 256u;

		for (i = 0; i < entries; i++)
			data[i] = (state->mode->palette[i] & 0xFCFCFCu) >> 2;
	}
	FB_GRAPHICS_UNLOCK();
}

/* ------------------------------------------------------------------------- */
/* Joystick and XPad snapshots                                               */
/* ------------------------------------------------------------------------- */

static FB_GFX3_INPUT_STATE *query_gamepad_input_locked(void)
{
	FB_GFX3_DRAW_STATE *state = fb_gfx3_api_get_draw_state_locked();

	if (state == NULL)
		return NULL;
	/*
		Gamepad callbacks publish a complete mutex-protected snapshot. An
		asynchronous controller query must not wait for unrelated rendering;
		doing so introduced a GPU round trip in every input sample.
	*/
	return &state->mode->input;
}

FBCALL int fb_GfxGetJoystick(int id, ssize_t *buttons, float *axis1,
	float *axis2, float *axis3, float *axis4, float *axis5, float *axis6,
	float *axis7, float *axis8)
{
	float *axes[] = { axis1, axis2, axis3, axis4, axis5, axis6, axis7, axis8 };
	FB_GFX3_GAMEPAD_STATE gamepad;
	FB_GFX3_INPUT_STATE *input;
	size_t i;
	int available;

	if (fb_gfx3_platform_joystick_has_native_polling()) {
		return fb_ErrorSetNum(fb_gfx3_platform_joystick_get(id, buttons,
			axis1, axis2, axis3, axis4, axis5, axis6, axis7, axis8) ==
			FB_GFX3_OK ? FB_RTERROR_OK :
			FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	FB_GRAPHICS_LOCK();
	input = query_gamepad_input_locked();
	available = fb_gfx3_input_gamepad_snapshot(input, id, &gamepad);
	FB_GRAPHICS_UNLOCK();
	if (!available) {
		if (buttons != NULL)
			*buttons = -1;
		for (i = 0; i < sizeof(axes) / sizeof(axes[0]); i++) {
			if (axes[i] != NULL)
				*axes[i] = -1000.0f;
		}
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	if (!gamepad.connected) {
		if (buttons != NULL)
			*buttons = -1;
		for (i = 0; i < sizeof(axes) / sizeof(axes[0]); i++) {
			if (axes[i] != NULL)
				*axes[i] = -1000.0f;
		}
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	if (buttons != NULL)
		*buttons = gamepad.buttons;
	for (i = 0; i < sizeof(axes) / sizeof(axes[0]); i++) {
		if (axes[i] != NULL)
			*axes[i] = gamepad.axis[i];
	}
	return fb_ErrorSetNum(FB_RTERROR_OK);
}

FBCALL int fb_GfxGetXPad(int id, ssize_t *buttons, float *left_x,
	float *left_y, float *right_x, float *right_y, float *left_trigger,
	float *right_trigger, ssize_t *dpad)
{
	float *axes[] = { left_x, left_y, right_x, right_y, left_trigger,
		right_trigger };
	FB_GFX3_GAMEPAD_STATE gamepad;
	FB_GFX3_INPUT_STATE *input;
	size_t i;
	int available;

	FB_GRAPHICS_LOCK();
	input = query_gamepad_input_locked();
	available = fb_gfx3_input_gamepad_snapshot(input, id, &gamepad);
	FB_GRAPHICS_UNLOCK();
	if (!available) {
		if (buttons != NULL)
			*buttons = 0;
		if (dpad != NULL)
			*dpad = 0;
		for (i = 0; i < sizeof(axes) / sizeof(axes[0]); i++) {
			if (axes[i] != NULL)
				*axes[i] = 0.0f;
		}
		fb_ErrorSetNum(FB_RTERROR_OK);
		return XPAD_STATUS_MISSING;
	}
	if (!gamepad.connected) {
		if (buttons != NULL)
			*buttons = 0;
		for (i = 0; i < sizeof(axes) / sizeof(axes[0]); i++) {
			if (axes[i] != NULL)
				*axes[i] = 0.0f;
		}
		if (dpad != NULL)
			*dpad = 0;
		fb_ErrorSetNum(FB_RTERROR_OK);
		return XPAD_STATUS_DISCONNECTED;
	}
	if (buttons != NULL) {
		*buttons = gamepad.buttons;
		if (gamepad.left_trigger > 0.5f)
			*buttons |= XPAD_BUTTON_L2;
		if (gamepad.right_trigger > 0.5f)
			*buttons |= XPAD_BUTTON_R2;
	}
	if (left_x != NULL)
		*left_x = gamepad.axis[0];
	if (left_y != NULL)
		*left_y = -gamepad.axis[1];
	if (right_x != NULL)
		*right_x = gamepad.axis[2];
	if (right_y != NULL)
		*right_y = -gamepad.axis[3];
	if (left_trigger != NULL)
		*left_trigger = gamepad.left_trigger;
	if (right_trigger != NULL)
		*right_trigger = gamepad.right_trigger;
	if (dpad != NULL)
		*dpad = gamepad.dpad;
	fb_ErrorSetNum(FB_RTERROR_OK);
	return XPAD_STATUS_CONNECTED;
}

/* ------------------------------------------------------------------------- */
/* Touch queries with mouse fallback                                         */
/* ------------------------------------------------------------------------- */

static int query_mouse_touch(FB_GFX3_INPUT_STATE *input, int *x, int *y,
	int *id)
{
	int available = FALSE;

	if ((input == NULL) || (input->mutex == NULL))
		return FALSE;
	fb_MutexLock(input->mutex);
	if (input->initialized && input->focused && input->mouse_inside &&
	    ((input->mouse_buttons & BUTTON_LEFT) != 0)) {
		if (x != NULL)
			*x = input->mouse_x;
		if (y != NULL)
			*y = input->mouse_y;
		if (id != NULL)
			*id = 0;
		available = TRUE;
	}
	fb_MutexUnlock(input->mutex);
	return available;
}

static FB_GFX3_INPUT_STATE *query_touch_input_locked(void)
{
	FB_GFX3_DRAW_STATE *state = fb_gfx3_api_get_draw_state_locked();

	if (state == NULL)
		return NULL;
	/*
		Native callbacks and the 16 ms renderer-side platform pump publish a
		complete touch snapshot under input->mutex. A touch query must not insert
		a GPU barrier merely to read that CPU-owned state.
	*/
	return &state->mode->input;
}

FBCALL ssize_t fb_GfxGetTouchCount(void)
{
	FB_GFX3_INPUT_STATE *input;
	ssize_t count = 0;
	int native_touch = FALSE;

	FB_GRAPHICS_LOCK();
	input = query_touch_input_locked();
	if (input != NULL)
		native_touch = fb_gfx3_input_touch_snapshot(input, -1, &count,
			NULL, NULL, NULL);
	if (!native_touch && (input != NULL) &&
	    query_mouse_touch(input, NULL, NULL, NULL))
		count = 1;
	FB_GRAPHICS_UNLOCK();
	fb_ErrorSetNum(FB_RTERROR_OK);
	return count;
}

FBCALL ssize_t fb_GfxGetTouch(ssize_t index, ssize_t *x, ssize_t *y,
	ssize_t *id)
{
	FB_GFX3_INPUT_STATE *input;
	int touch_x = -1;
	int touch_y = -1;
	int touch_id = -1;
	int available = FALSE;
	int native_touch = FALSE;

	FB_GRAPHICS_LOCK();
	input = query_touch_input_locked();
	if (input != NULL)
		native_touch = fb_gfx3_input_touch_snapshot(input, index, NULL,
			&touch_x, &touch_y, &touch_id);
	if (native_touch && (index >= 0))
		available = (touch_x >= 0) && (touch_y >= 0) && (touch_id >= 0);
	else if ((index == 0) && (input != NULL))
		available = query_mouse_touch(input, &touch_x, &touch_y,
			&touch_id);
	FB_GRAPHICS_UNLOCK();
	if (!available) {
		if (x != NULL)
			*x = -1;
		if (y != NULL)
			*y = -1;
		if (id != NULL)
			*id = -1;
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	if (x != NULL)
		*x = touch_x;
	if (y != NULL)
		*y = touch_y;
	if (id != NULL)
		*id = touch_id;
	return fb_ErrorSetNum(FB_RTERROR_OK);
}

FBCALL ssize_t fb_GfxGetTouchHit(ssize_t x1, ssize_t y1, ssize_t x2,
	ssize_t y2)
{
	ssize_t touch_count;
	ssize_t touch_x;
	ssize_t touch_y;
	ssize_t touch_id;
	ssize_t temporary;
	ssize_t index;

	if (x1 > x2) {
		temporary = x1;
		x1 = x2;
		x2 = temporary;
	}
	if (y1 > y2) {
		temporary = y1;
		y1 = y2;
		y2 = temporary;
	}
	touch_count = fb_GfxGetTouchCount();
	for (index = 0; index < touch_count; ++index) {
		if ((fb_GfxGetTouch(index, &touch_x, &touch_y, &touch_id) ==
		     FB_RTERROR_OK) && (touch_x >= x1) && (touch_x <= x2) &&
		    (touch_y >= y1) && (touch_y <= y2)) {
			fb_ErrorSetNum(FB_RTERROR_OK);
			return TRUE;
		}
	}
	fb_ErrorSetNum(FB_RTERROR_OK);
	return FALSE;
}

FBCALL ssize_t fb_GfxGetTouchHitCircle(ssize_t x, ssize_t y,
	ssize_t radius)
{
	ssize_t touch_count;
	ssize_t touch_x;
	ssize_t touch_y;
	ssize_t touch_id;
	ssize_t index;
	long double delta_x;
	long double delta_y;
	long double checked_radius = (long double)radius;
	long double distance_squared;

	if (checked_radius < 0.0L)
		checked_radius = -checked_radius;
	touch_count = fb_GfxGetTouchCount();
	for (index = 0; index < touch_count; ++index) {
		if (fb_GfxGetTouch(index, &touch_x, &touch_y, &touch_id) !=
		    FB_RTERROR_OK)
			continue;
		delta_x = (long double)touch_x - (long double)x;
		delta_y = (long double)touch_y - (long double)y;
		distance_squared = (delta_x * delta_x) + (delta_y * delta_y);
		if (distance_squared <= checked_radius * checked_radius) {
			fb_ErrorSetNum(FB_RTERROR_OK);
			return TRUE;
		}
	}
	fb_ErrorSetNum(FB_RTERROR_OK);
	return FALSE;
}

/* end of gfx3_query_api.c */
