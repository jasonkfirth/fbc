/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_platform.h

    Purpose:

        Define the private window-system contract used by GPU backends.

    Responsibilities:

        - create a native window independently of any graphics API
        - create a native window and an owning OpenGL context
        - load OpenGL entry points through the native context mechanism
        - expose native handles to graphics APIs which create their own surface
        - expose presentation and basic window operations to the render thread
        - keep native handles and platform headers out of renderer state

    This file intentionally does NOT contain:

        - Win32, X11, WGL, GLX, EGL, Vulkan, or OpenGL declarations
        - FreeBASIC graphics command semantics
        - input-event translation
*/

#ifndef __FB_GFX3_PLATFORM_H__
#define __FB_GFX3_PLATFORM_H__

#include "fb_gfx3.h"
#include "gfx3_presentation.h"

/* Public fbgfx.bi window flags passed through backend configuration. */
#define FB_GFX3_WINDOW_FULLSCREEN 0x00000001u
#define FB_GFX3_WINDOW_NO_SWITCH  0x00000004u
#define FB_GFX3_WINDOW_NO_FRAME   0x00000008u
#define FB_GFX3_WINDOW_RESIZABLE  0x00000400u

typedef struct FB_GFX3_PLATFORM_WINDOW_CONFIG {
	void *input;
	uint32_t width;
	uint32_t height;
	uint32_t flags;
	const char *title;
} FB_GFX3_PLATFORM_WINDOW_CONFIG;

typedef struct FB_GFX3_PLATFORM_OPENGL_CONFIG {
	void *input;
	uint32_t width;
	uint32_t height;
	uint32_t major_version;
	uint32_t minor_version;
	uint32_t flags;
	const char *title;
} FB_GFX3_PLATFORM_OPENGL_CONFIG;

/*
	Android's NativeActivity package exposes a small keyboard target above the
	game image.  It is presentation state, not a BASIC drawable surface: the
	GLES presentation shader consumes it after the logical page has been
	converted to the native window format.  Keeping it here avoids a CPU
	framebuffer detour and prevents the control from altering POINT/GET results.
*/
typedef struct FB_GFX3_ANDROID_KEYBOARD_OVERLAY {
	int x0;
	int y0;
	int x1;
	int y1;
	int visible;
	int keyboard_visible;
	int pressed;
} FB_GFX3_ANDROID_KEYBOARD_OVERLAY;

typedef struct FB_GFX3_PLATFORM_VTABLE {
	const char *name;
	int (*probe_opengl)(void);
	int (*create_window)(void **platform,
		const FB_GFX3_PLATFORM_WINDOW_CONFIG *config);
	int (*create_opengl)(void **platform,
		const FB_GFX3_PLATFORM_OPENGL_CONFIG *config);
	int (*native_handles)(void *platform, uintptr_t *instance,
		uintptr_t *window);
	void (*destroy)(void *platform);
	int (*load_opengl_function)(void *platform, const char *name,
		void *destination, size_t destination_size);
	int (*client_size)(void *platform, uint32_t *width, uint32_t *height);
	int (*desktop_info)(ssize_t *width, ssize_t *height, ssize_t *depth,
		ssize_t *refresh);
	int (*swap_buffers)(void *platform);
	void (*pump_events)(void *platform);
	int (*show_window)(void *platform);
	int (*set_window_title)(void *platform, const char *title);
} FB_GFX3_PLATFORM_VTABLE;

const FB_GFX3_PLATFORM_VTABLE *fb_gfx3_platform_default(void);

int fb_gfx3_platform_presentation_layout(uint32_t logical_width,
	uint32_t logical_height, uint32_t client_width, uint32_t client_height,
	FB_GFX3_PRESENTATION_LAYOUT *layout);
int fb_gfx3_platform_client_to_logical(
	const FB_GFX3_PRESENTATION_LAYOUT *layout, uint32_t logical_width,
	uint32_t logical_height, int client_x, int client_y, int *logical_x,
	int *logical_y);
int fb_gfx3_platform_logical_to_client(
	const FB_GFX3_PRESENTATION_LAYOUT *layout, uint32_t logical_width,
	uint32_t logical_height, int logical_x, int logical_y, int *client_x,
	int *client_y);

int fb_gfx3_platform_keyboard_overlay(void *platform,
	FB_GFX3_ANDROID_KEYBOARD_OVERLAY *overlay);

#endif

/* end of gfx3_platform.h */
