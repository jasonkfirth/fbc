/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_api.c

    Purpose:

        Export the first complete FreeBASIC graphics symbol families backed by
        gfxlib3 logical modes and GPU surfaces.

    Responsibilities:

        - open SCREENRES and standard SCREEN modes
        - bind caller-local drawing state to the runtime GFX TLS slot
        - export initial page, coordinate, and primitive entry points
        - translate gfxlib3 failures into FreeBASIC runtime errors

    This file intentionally does NOT contain:

		- native window creation, input translation, or console hooks
        - PAINT, DRAW, text, palette, GET, or PUT public entry points
        - CPU FB.IMAGE storage and transfer implementation
*/

#include "gfx3_backend_null.h"
#include "gfx3_backend_opengl.h"
#include "gfx3_backend_select.h"
#include "gfx3_backend_vulkan.h"
#include "gfx3_api_internal.h"
#include "gfx3_compat.h"
#include "gfx3_console.h"
#include "gfx3_gpu_surface.h"
#include "gfx3_line_input.h"
#include "gfx3_target.h"
#include "gfx3_vga_api.h"

#include "gfx3_image.h"
#include "gfx3_input.h"

typedef struct FB_GFX3_STANDARD_MODE {
	uint16_t width;
	uint16_t height;
	uint8_t depth;
	uint8_t pages;
	uint8_t console_font_width;
	uint8_t console_font_height;
	uint8_t console_rows;
} FB_GFX3_STANDARD_MODE;

static const FB_GFX3_STANDARD_MODE standard_modes[] = {
	{    0,    0, 0, 1, 8,  8,  0 }, /* Text mode / close graphics */
	{  320,  200, 2, 8, 8,  8, 25 }, /* CGA */
	{  640,  200, 1, 1, 8,  8, 25 }, /* CGA monochrome */
	{  720,  348, 1, 2, 9, 14, 25 }, /* Hercules */
	{  640,  400, 1, 1, 8, 16, 25 }, /* Olivetti and AT&T */
	{  320,  200, 4, 1, 8,  8, 25 }, /* PCjr and Tandy 16-colour */
	{  640,  200, 2, 1, 8,  8, 25 }, /* PCjr and Tandy 4-colour */
	{  320,  200, 4, 8, 8,  8, 25 }, /* EGA */
	{  640,  200, 4, 4, 8,  8, 25 }, /* EGA */
	{  640,  350, 4, 2, 8, 14, 25 }, /* EGA */
	{  640,  350, 1, 2, 8, 14, 25 }, /* EGA monochrome */
	{  640,  480, 1, 1, 8, 16, 30 }, /* VGA monochrome */
	{  640,  480, 4, 1, 8, 16, 30 }, /* VGA */
	{  320,  200, 8, 1, 8,  8, 25 }, /* VGA / MCGA */
	{  320,  240, 8, 1, 8,  8, 30 },
	{  400,  300, 8, 1, 8,  8, 37 },
	{  512,  384, 8, 1, 8, 16, 24 },
	{  640,  400, 8, 1, 8, 16, 25 },
	{  640,  480, 8, 1, 8, 16, 30 },
	{  800,  600, 8, 1, 8, 16, 37 },
	{ 1024,  768, 8, 1, 8, 16, 48 },
	{ 1280, 1024, 8, 1, 8, 16, 64 }
};

static FB_GFX3_MODE active_mode;
static int mode_is_active;
static char window_title[128] = "FreeBASIC gfxlib3";
/*
	The runtime GFX TLS slot owns the complete caller-local drawing state. Most
	graphics calls come from the same BASIC thread, however, so asking the
	runtime TLS registry for that same slot on every sprite is unnecessary.

	The mode generation is checked before the cached pointer is dereferenced.
	Closing or replacing a mode therefore invalidates the cache without reading
	a state object which its TLS destructor may already have released.
*/
static _Thread_local FB_GFX3_DRAW_STATE *api_cached_draw_state;
static _Thread_local uint64_t api_cached_draw_state_generation;

#define FB_GFX3_SCREEN_FULLSCREEN 0x00000001u
#define FB_GFX3_SCREEN_NO_FRAME   0x00000008u
#define FB_GFX3_SCREEN_SHAPED     0x00000010u
#define FB_GFX3_SCREEN_RESIZABLE  0x00000400u

FBCALL float fb_GfxCursor(int function);

/* ------------------------------------------------------------------------- */
/* Runtime state and error handling                                          */
/* ------------------------------------------------------------------------- */

int fb_gfx3_api_runtime_error(int result)
{
	if (result == FB_GFX3_OK)
		return FB_RTERROR_OK;
	if (result == FB_GFX3_OUT_OF_MEMORY)
		return FB_RTERROR_OUTOFMEM;
	return FB_RTERROR_ILLEGALFUNCTIONCALL;
}

static int api_runtime_error(int result)
{
	return fb_gfx3_api_runtime_error(result);
}

static int api_apply_standard_palette(FB_GFX3_MODE *mode,
	int standard_mode)
{
	if (mode == NULL)
		return FB_GFX3_INVALID;
	/*
		The shared 256-colour table begins with the VGA 16-colour sequence,
		where attribute 1 is blue. Hercules and Olivetti graphics expose only
		black and one foreground attribute, whose power-on colour is white.
	*/
	if ((standard_mode == 3) || (standard_mode == 4)) {
		/* Attribute zero is black and attribute one starts as bright white. */
		mode->palette[0] = mode->standard_colors[0];
		mode->palette[1] = mode->standard_colors[15];
		memcpy(mode->default_palette, mode->palette,
			sizeof(mode->default_palette));
		return fb_gfx3_context_set_palette(&mode->context, mode->palette);
	}
	return FB_GFX3_OK;
}

static int api_log_level(void)
{
	const char *value = getenv("FBGFX3_LOG");

	/*
		An explicit profile request must be useful without a second environment
		option.  The renderer still uses the centralized logger, so applications
		with a custom callback receive the same bounded messages.
	*/
	if (value == NULL) {
		value = getenv("FBGFX3_PROFILE");
		if ((value != NULL) && (value[0] != '\0') &&
		    (strcmp(value, "0") != 0) && (strcmp(value, "off") != 0) &&
		    (strcmp(value, "OFF") != 0) && (strcmp(value, "false") != 0) &&
		    (strcmp(value, "FALSE") != 0))
			return FB_GFX3_LOG_INFO;
		return FB_GFX3_LOG_WARNING;
	}
	if ((strcmp(value, "error") == 0) || (strcmp(value, "ERROR") == 0))
		return FB_GFX3_LOG_ERROR;
	if ((strcmp(value, "info") == 0) || (strcmp(value, "INFO") == 0))
		return FB_GFX3_LOG_INFO;
	if ((strcmp(value, "trace") == 0) || (strcmp(value, "TRACE") == 0))
		return FB_GFX3_LOG_TRACE;
	return FB_GFX3_LOG_WARNING;
}

static void api_tls_destructor(void *data)
{
	FB_GFXCTX *runtime_context = (FB_GFXCTX *)data;

	api_cached_draw_state = NULL;
	api_cached_draw_state_generation = 0;
	fb_gfx3_draw_state_destroy(
		(FB_GFX3_DRAW_STATE *)runtime_context->line);
	free((void *)runtime_context->line);
	runtime_context->line = NULL;
}

/* Caller holds FB_GRAPHICS_LOCK(). */
int fb_gfx3_api_apply_pending_resize_locked(FB_GFX3_DRAW_STATE *state)
{
	uint32_t width;
	uint32_t height;
	int result = FB_GFX3_OK;

	if ((state == NULL) || (state->mode == NULL))
		return FB_GFX3_INVALID;
	/*
		A fixed-size mode can never receive a logical framebuffer resize. Most
		games use this form, so do not take the input and mode mutexes on every
		primitive merely to rediscover that no resize is possible.

		FB_GRAPHICS_LOCK protects mode replacement while this test runs. The
		resizable property is immutable for the lifetime of one active mode.
	*/
	if (!state->mode->resizable)
		return FB_GFX3_OK;
	if (fb_gfx3_input_take_resize(&state->mode->input, &width, &height)) {
		/* Keep the old framebuffer active while a SCREENLOCK pointer is live. */
		if ((width >= state->mode->console_font_width) &&
		    (height >= state->mode->console_font_height))
			result = fb_gfx3_mode_resize(state->mode, state, width, height);
	}
	if (fb_gfx3_draw_state_sync_resize(state) != FB_GFX3_OK)
		return FB_GFX3_FAILED;
	return result;
}

/* Caller holds FB_GRAPHICS_LOCK(). */
FB_GFX3_DRAW_STATE *fb_gfx3_api_get_draw_state_locked(void)
{
	FB_GFXCTX *runtime_context;
	FB_GFX3_DRAW_STATE *state;
	uint64_t generation;

	if (!mode_is_active)
		return NULL;
	generation = active_mode.generation;
	if ((api_cached_draw_state != NULL) &&
	    (api_cached_draw_state_generation == generation)) {
		fb_gfx3_api_apply_pending_resize_locked(api_cached_draw_state);
		return api_cached_draw_state;
	}
	runtime_context = (FB_GFXCTX *)fb_TlsGetCtx(FB_TLSKEY_GFX,
		sizeof(*runtime_context), api_tls_destructor);
	if (runtime_context == NULL)
		return NULL;
	state = (FB_GFX3_DRAW_STATE *)runtime_context->line;
	if ((state == NULL) || (state->mode != &active_mode) ||
	    (state->generation != active_mode.generation)) {
		fb_gfx3_draw_state_destroy(state);
		free(state);
		state = (FB_GFX3_DRAW_STATE *)calloc(1, sizeof(*state));
		if (state == NULL) {
			runtime_context->line = NULL;
			return NULL;
		}
		if (fb_gfx3_draw_state_init(&active_mode, state) != FB_GFX3_OK) {
			free(state);
			runtime_context->line = NULL;
			return NULL;
		}
		runtime_context->line = (unsigned char **)state;
		runtime_context->id = (int)(active_mode.generation & INT_MAX);
	}
	api_cached_draw_state = state;
	api_cached_draw_state_generation = generation;
	fb_gfx3_api_apply_pending_resize_locked(state);
	return state;
}

static FB_GFX3_DRAW_STATE *api_get_draw_state(void)
{
	return fb_gfx3_api_get_draw_state_locked();
}

/* Caller holds FB_LOCK() followed by FB_GRAPHICS_LOCK(). */
static int api_close_mode(void)
{
	int result = FB_GFX3_OK;

	if (mode_is_active) {
		fb_gfx3_console_shutdown_locked(&active_mode);
		fb_gfx3_gpu_surfaces_destroy_all_locked(&active_mode);
		mode_is_active = FALSE;
		fb_TlsDelCtx(FB_TLSKEY_GFX);
		result = fb_gfx3_mode_shutdown(&active_mode);
	}
	return result;
}

static void api_exit(void)
{
	FB_LOCK();
	FB_GRAPHICS_LOCK();
	api_close_mode();
	if (__fb_ctx.exit_gfxlib2 == api_exit)
		__fb_ctx.exit_gfxlib2 = NULL;
	FB_GRAPHICS_UNLOCK();
	FB_UNLOCK();
}

/* ------------------------------------------------------------------------- */
/* SCREEN mode lifecycle                                                     */
/* ------------------------------------------------------------------------- */

static int api_screen_res(int width, int height, int depth, int page_count,
	int flags, int refresh_rate, uint32_t console_font_width,
	uint32_t console_font_height, uint32_t console_rows, int standard_mode)
{
	const FB_GFX3_BACKEND_VTABLE *backend_plan[
		FB_GFX3_BACKEND_PLAN_CAPACITY];
	int backend_attempt_flags[FB_GFX3_BACKEND_PLAN_CAPACITY];
	const char *requested_driver;
	FB_GFX3_CONTEXT_CONFIG config;
	size_t backend_count;
	size_t backend_index;
	int result;

	(void)refresh_rate;
	/*
		gfxlib2 stores both legacy true-colour spellings in its native GPU
		formats: 15-bit requests become RGB565 and 24-bit requests become
		32-bit BGRA.  Keeping that normalization at the public mode boundary
		ensures every later page, lock shadow, and SCREENINFO query agrees on
		the actual storage format.
	*/
	if (depth == 15)
		depth = 16;
	else if (depth == 24)
		depth = 32;
	if ((width <= 0) || (height <= 0) ||
	    !((depth == 1) || (depth == 2) || (depth == 4) ||
	      (depth == 8) || (depth == 16) || (depth == 32)))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	if (page_count <= 0)
		page_count = 1;
	if (page_count > 255)
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	if ((flags != -1) && (((uint32_t)flags & FB_GFX3_SCREEN_RESIZABLE) != 0u) &&
	    (((uint32_t)flags & (FB_GFX3_SCREEN_FULLSCREEN |
	      FB_GFX3_SCREEN_NO_FRAME | FB_GFX3_SCREEN_SHAPED)) != 0u))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	if ((flags != -1) &&
	    !fb_gfx3_target_screen_flags_valid((uint32_t)flags))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

	FB_LOCK();
	FB_GRAPHICS_LOCK();
	api_close_mode();
	memset(&config, 0, sizeof(config));
	config.title = window_title;
	config.width = (uint32_t)width;
	config.height = (uint32_t)height;
	config.depth = (uint32_t)depth;
	config.page_count = (uint32_t)page_count;
	config.console_font_height = console_font_height;
	config.flags = (uint32_t)flags;
	/*
		A frame can legitimately contain several thousand independent BASIC
		primitives or sprites.  A 1024-entry queue forced the caller to stop
		four times during the standard 4096-sprite workload, fragmenting the
		render thread's ordered GPU batches.  Eight thousand entries retain
		bounded memory use while allowing one complete busy frame to reach the
		backend as a single submission run.
	*/
	config.queue_capacity = 8192;
	config.resource_capacity = (size_t)page_count + 64;
	config.log_level = api_log_level();
	requested_driver = fb_gfx3_backend_requested_name();
	if (requested_driver == NULL)
		requested_driver = getenv("FBGFX");
	backend_count = fb_gfx3_backend_attempt_plan(flags, requested_driver,
		backend_plan, backend_attempt_flags, FB_GFX3_BACKEND_PLAN_CAPACITY);
	result = FB_GFX3_UNSUPPORTED;
	for (backend_index = 0; backend_index < backend_count; backend_index++) {
		config.backend = backend_plan[backend_index];
		config.flags = (uint32_t)backend_attempt_flags[backend_index];
		config.idle_poll_milliseconds =
			(config.backend == &__fb_gfx3_backend_null) ? 0u : 10u;
		result = fb_gfx3_mode_init(&active_mode, &config,
			(depth == 32) ? 0xFF000000u : 0u);
		if (result == FB_GFX3_OK)
			break;
	}
	if (result == FB_GFX3_OK) {
		active_mode.console_font_width = console_font_width;
		active_mode.console_rows = console_rows;
		active_mode.standard_mode = standard_mode;
		mode_is_active = TRUE;
		result = api_apply_standard_palette(&active_mode, standard_mode);
	}
	if (result == FB_GFX3_OK) {
		fb_gfx3_api_gl_control_mode_opened_locked(&active_mode);
		/*
			GFX_NULL is often used as a tiny off-screen image factory. gfxlib2
			permits modes smaller than one 8-pixel console cell, so do not turn
			a valid graphics mode into a SCREENRES failure merely because text
			console hooks cannot be represented there.
		*/
		if ((active_mode.width >= active_mode.console_font_width) &&
		    (active_mode.height >= active_mode.console_font_height))
			result = fb_gfx3_console_init_locked(&active_mode);
		else
			result = FB_GFX3_OK;
		if (result == FB_GFX3_OK) {
			fb_gfx3_input_install_hooks_locked();
			fb_gfx3_line_input_install_hooks_locked();
			fb_gfx3_vga_install_hooks_locked();
			__fb_ctx.exit_gfxlib2 = api_exit;
		} else
			api_close_mode();
	} else if (mode_is_active)
		api_close_mode();
	FB_GRAPHICS_UNLOCK();
	FB_UNLOCK();
	return fb_ErrorSetNum(api_runtime_error(result));
}

FBCALL int fb_GfxScreenRes(int width, int height, int depth, int page_count,
	int flags, int refresh_rate)
{
	return api_screen_res(width, height, depth, page_count, flags,
		refresh_rate, 8u, 8u, 0u, -1);
}

FBCALL int fb_GfxScreen(int mode, int depth, int page_count, int flags,
	int refresh_rate)
{
	const FB_GFX3_STANDARD_MODE *mode_info;

	if ((mode < 0) ||
	    ((size_t)mode >= sizeof(standard_modes) / sizeof(standard_modes[0])))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	if (mode == 0) {
		FB_LOCK();
		FB_GRAPHICS_LOCK();
		api_close_mode();
		FB_GRAPHICS_UNLOCK();
		FB_UNLOCK();
		return fb_ErrorSetNum(FB_RTERROR_OK);
	}
	mode_info = &standard_modes[mode];
	if ((mode_info->width == 0) || (mode_info->height == 0))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	if ((mode <= 13) ||
	    !((depth == 1) || (depth == 2) || (depth == 4) ||
	      (depth == 8) || (depth == 15) || (depth == 16) ||
	      (depth == 24) || (depth == 32)))
		depth = mode_info->depth;
	if (page_count <= 0)
		page_count = mode_info->pages;
	return api_screen_res(mode_info->width, mode_info->height, depth,
		page_count, flags, refresh_rate, mode_info->console_font_width,
		mode_info->console_font_height, mode_info->console_rows, mode);
}

FBCALL int fb_GfxScreenQB(int mode, int visible_page, int active_page)
{
	FB_GFX3_DRAW_STATE *state;
	int result;

	result = fb_GfxScreen(mode, 0, 0, 0, 0);
	if ((result != FB_RTERROR_OK) ||
	    ((visible_page < 0) && (active_page < 0)))
		return result;
	FB_GRAPHICS_LOCK();
	state = api_get_draw_state();
	/*
	    QB SCREEN supplies its page arguments in the historical
	    (visible, active) ABI order.  gfxlib2 forwards them unchanged to
	    fb_PageSet(), where that order maps to (active, visible).  Preserve the
	    same swap here so SCREEN mode, , page selects the drawing page.
	*/
	result = (state == NULL) ? FB_GFX3_OUT_OF_MEMORY :
		fb_gfx3_page_set(state, visible_page, active_page, NULL);
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(api_runtime_error(result));
}

FBCALL void fb_GfxSetWindowTitle(FBSTRING *title)
{
	ssize_t length;

	if ((title == NULL) || (title->data == NULL))
		return;
	length = FB_STRSIZE(title);
	if (length < 0)
		return;
	FB_GRAPHICS_LOCK();
	fb_gfx3_api_set_window_title_locked(title->data, (size_t)length);
	FB_GRAPHICS_UNLOCK();
}

const char *fb_gfx3_api_get_window_title_locked(void)
{
	return window_title;
}

/* Caller holds FB_GRAPHICS_LOCK(). */
void fb_gfx3_api_set_window_title_locked(const char *title, size_t length)
{
	if (title == NULL)
		return;
	if (length >= sizeof(window_title))
		length = sizeof(window_title) - 1u;
	memcpy(window_title, title, length);
	window_title[length] = '\0';
	if (mode_is_active)
		fb_gfx3_context_set_window_title(&active_mode.context,
			window_title, length);
}

/* ------------------------------------------------------------------------- */
/* Initial drawing and coordinate ABI                                        */
/* ------------------------------------------------------------------------- */

FBCALL void fb_GfxPset(void *target, float x, float y, unsigned int color,
	int flags, int preset)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *gpu_surface;
	FB_GFX3_IMAGE_VIEW image;

	FB_GRAPHICS_LOCK();
	state = api_get_draw_state();
	if (state != NULL) {
		if (target == NULL)
			fb_gfx3_compat_pset(state, x, y, color,
				(uint32_t)flags, preset);
		else if (fb_gfx3_gpu_surface_lookup_locked(target, state->mode,
		    &gpu_surface) == FB_GFX3_OK)
			fb_gfx3_gpu_surface_pset(gpu_surface, state, x, y, color,
				(uint32_t)flags, preset);
		else if (fb_gfx3_image_parse(target, &image) == FB_GFX3_OK) {
			fb_gfx3_image_pset(&image, state, x, y, color,
				(uint32_t)flags, preset);
			fb_gfx3_image_cache_metadata_touch(&image);
		}
	}
	FB_GRAPHICS_UNLOCK();
}

FBCALL unsigned int fb_GfxPoint(void *target, float x, float y)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *gpu_surface;
	FB_GFX3_IMAGE_VIEW image;
	uint32_t color = UINT32_MAX;

	if (y == -8388607.0f)
		return (unsigned int)fb_GfxCursor((int)x);
	FB_GRAPHICS_LOCK();
	state = api_get_draw_state();
	if (state != NULL) {
		if (target == NULL)
			fb_gfx3_compat_point(state, x, y, &color);
		else if (fb_gfx3_gpu_surface_lookup_locked(target, state->mode,
		    &gpu_surface) == FB_GFX3_OK)
			fb_gfx3_gpu_surface_point(gpu_surface, x, y, &color);
		else if (fb_gfx3_image_parse(target, &image) == FB_GFX3_OK)
			color = fb_gfx3_image_point(&image, x, y);
	}
	FB_GRAPHICS_UNLOCK();
	return color;
}

FBCALL void fb_GfxLine(void *target, float x1, float y1, float x2, float y2,
	unsigned int color, int type, unsigned int style, int flags)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *gpu_surface;
	FB_GFX3_IMAGE_VIEW image;

	FB_GRAPHICS_LOCK();
	state = api_get_draw_state();
	if (state != NULL) {
		if (target == NULL)
			fb_gfx3_compat_line(state, x1, y1, x2, y2, color, type,
				style, (uint32_t)flags);
		else if (fb_gfx3_gpu_surface_lookup_locked(target, state->mode,
		    &gpu_surface) == FB_GFX3_OK)
			fb_gfx3_gpu_surface_line(gpu_surface, state, x1, y1, x2, y2,
				color, type, style, (uint32_t)flags);
		else if (fb_gfx3_image_parse(target, &image) == FB_GFX3_OK) {
			fb_gfx3_image_line(&image, state, x1, y1, x2, y2, color,
				type, style, (uint32_t)flags);
			fb_gfx3_image_cache_metadata_touch(&image);
		}
	}
	FB_GRAPHICS_UNLOCK();
}

FBCALL void fb_GfxEllipse(void *target, float x, float y, float radius,
	unsigned int color, float aspect, float start, float end, int filled,
	int flags)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_SURFACE *gpu_surface;
	FB_GFX3_IMAGE_VIEW image;

	FB_GRAPHICS_LOCK();
	state = api_get_draw_state();
	if (state != NULL) {
		if (target != NULL) {
			if (fb_gfx3_gpu_surface_lookup_locked(target, state->mode,
			    &gpu_surface) == FB_GFX3_OK)
				fb_gfx3_gpu_surface_ellipse(gpu_surface, state, x, y,
					radius, color, aspect, start, end, filled,
					(uint32_t)flags);
			else if (fb_gfx3_image_parse(target, &image) == FB_GFX3_OK) {
				fb_gfx3_image_ellipse(&image, state, x, y, radius,
					color, aspect, start, end, filled,
					(uint32_t)flags);
				fb_gfx3_image_cache_metadata_touch(&image);
			}
		} else if ((start == 0.0f) && (end == 6.283186f)) {
			fb_gfx3_compat_ellipse(state, x, y, radius, color, aspect,
				filled, (uint32_t)flags);
		} else {
			fb_gfx3_compat_arc(state, x, y, radius, color, aspect,
				start, end, (uint32_t)flags);
		}
	}
	FB_GRAPHICS_UNLOCK();
}

FBCALL void fb_GfxView(int x1, int y1, int x2, int y2,
	unsigned int fill_color, unsigned int border_color, int flags)
{
	FB_GFX3_DRAW_STATE *state;

	FB_GRAPHICS_LOCK();
	state = api_get_draw_state();
	if (state != NULL)
		fb_gfx3_compat_view(state, x1, y1, x2, y2, fill_color,
			border_color, (uint32_t)flags);
	FB_GRAPHICS_UNLOCK();
}

FBCALL void fb_GfxWindow(float x1, float y1, float x2, float y2, int screen)
{
	FB_GFX3_DRAW_STATE *state;

	FB_GRAPHICS_LOCK();
	state = api_get_draw_state();
	if (state != NULL)
		fb_gfx3_compat_window_graphics_locked(state, x1, y1, x2, y2,
			screen);
	FB_GRAPHICS_UNLOCK();
}

FBCALL float fb_GfxPMap(float coordinate, int function)
{
	FB_GFX3_DRAW_STATE *state;
	float result = 0.0f;

	FB_GRAPHICS_LOCK();
	state = api_get_draw_state();
	if (state != NULL)
		fb_gfx3_compat_pmap_graphics_locked(state, coordinate, function,
			&result);
	FB_GRAPHICS_UNLOCK();
	return result;
}

FBCALL float fb_GfxCursor(int function)
{
	FB_GFX3_DRAW_STATE *state;
	float result = 0.0f;

	FB_GRAPHICS_LOCK();
	state = api_get_draw_state();
	if (state != NULL)
		fb_gfx3_compat_cursor_graphics_locked(state, function, &result);
	FB_GRAPHICS_UNLOCK();
	return result;
}

/* ------------------------------------------------------------------------- */
/* Page ABI                                                                  */
/* ------------------------------------------------------------------------- */

int fb_GfxPageSet(int work_page, int visible_page)
{
	FB_GFX3_DRAW_STATE *state;
	int previous = -1;

	FB_GRAPHICS_LOCK();
	state = api_get_draw_state();
	if (state != NULL)
		fb_gfx3_page_set(state, work_page, visible_page, &previous);
	FB_GRAPHICS_UNLOCK();
	return previous;
}

int fb_GfxPageCopy(int from_page, int to_page)
{
	FB_GFX3_DRAW_STATE *state;
	int result;

	FB_GRAPHICS_LOCK();
	state = api_get_draw_state();
	result = (state == NULL) ? FB_GFX3_INVALID :
		fb_gfx3_page_copy(state, from_page, to_page);
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(api_runtime_error(result));
}

FBCALL int fb_GfxFlip(int from_page, int to_page)
{
	/* gfxlib2 exposes FLIP and page-copy as the same operation. */
	return fb_GfxPageCopy(from_page, to_page);
}

/* end of gfx3_api.c */
