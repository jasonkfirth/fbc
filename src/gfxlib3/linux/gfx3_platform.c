/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: linux/gfx3_platform.c

    Purpose:

        Own the X11 window used by a gfxlib3 renderer and, when requested, its
        GLX context.

    Responsibilities:

        - create a correctly sized X11 window for Vulkan or OpenGL
        - create and own an OpenGL 4.3 or newer core GLX context
        - expose Display and Window values for Vulkan Xlib surfaces
        - translate X11 keyboard, mouse, focus, close, and resize events
        - apply synchronized cursor and window requests on the render thread

    This file intentionally does NOT contain:

        - shaders, GPU surfaces, or graphics primitive execution
        - Vulkan surface or swapchain lifecycle
        - persistent display-mode changes
*/

#include "../gfx3_platform.h"
#include "../gfx3_input.h"

#if defined(HOST_LINUX) && !defined(DISABLE_X11)

#include <dlfcn.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "../../rtlib/unix/fb_private_scancodes_x11.h"

#if !defined(DISABLE_OPENGL)
typedef struct FB_GFX3_GLX_CONTEXT_RECORD *FB_GFX3_GLX_CONTEXT;
typedef struct FB_GFX3_GLX_FB_CONFIG_RECORD *FB_GFX3_GLX_FB_CONFIG;
typedef XID FB_GFX3_GLX_DRAWABLE;
typedef void (*FB_GFX3_GLX_VOID_FUNCTION)(void);

typedef FB_GFX3_GLX_FB_CONFIG *(*FB_GFX3_GLX_CHOOSE_FB_CONFIG)(
	Display *display,
	int screen, const int *attributes, int *count);
typedef XVisualInfo *(*FB_GFX3_GLX_GET_VISUAL)(Display *display,
	FB_GFX3_GLX_FB_CONFIG config);
typedef FB_GFX3_GLX_VOID_FUNCTION (*FB_GFX3_GLX_GET_PROC_ADDRESS)(
	const unsigned char *name);
typedef void (*FB_GFX3_GLX_DESTROY_CONTEXT)(Display *display,
	FB_GFX3_GLX_CONTEXT context);
typedef Bool (*FB_GFX3_GLX_MAKE_CURRENT)(Display *display,
	FB_GFX3_GLX_DRAWABLE drawable, FB_GFX3_GLX_CONTEXT context);
typedef void (*FB_GFX3_GLX_SWAP_BUFFERS)(Display *display,
	FB_GFX3_GLX_DRAWABLE drawable);
typedef FB_GFX3_GLX_CONTEXT (*FB_GFX3_GLX_CREATE_CONTEXT_ATTRIBUTES)(
	Display *display, FB_GFX3_GLX_FB_CONFIG config,
	FB_GFX3_GLX_CONTEXT shared_context, Bool direct, const int *attributes);

/*
	Only the GLX 1.3 framebuffer-selection and ARB context attributes used by
	this adapter are repeated here.  Functions are loaded from libGL at run
	time, so gfxlib3 does not require OpenGL development headers or a direct
	libGL link merely to build the runtime.
*/
enum FB_GFX3_GLX_CONSTANT {
	FB_GFX3_GLX_DOUBLEBUFFER = 5,
	FB_GFX3_GLX_RED_SIZE = 8,
	FB_GFX3_GLX_GREEN_SIZE = 9,
	FB_GFX3_GLX_BLUE_SIZE = 10,
	FB_GFX3_GLX_ALPHA_SIZE = 11,
	FB_GFX3_GLX_X_VISUAL_TYPE = 0x22,
	FB_GFX3_GLX_TRUE_COLOR = 0x8002,
	FB_GFX3_GLX_DRAWABLE_TYPE = 0x8010,
	FB_GFX3_GLX_RENDER_TYPE = 0x8011,
	FB_GFX3_GLX_X_RENDERABLE = 0x8012,
	FB_GFX3_GLX_WINDOW_BIT = 0x00000001,
	FB_GFX3_GLX_RGBA_BIT = 0x00000001,
	FB_GFX3_GLX_CONTEXT_MAJOR_VERSION_ARB = 0x2091,
	FB_GFX3_GLX_CONTEXT_MINOR_VERSION_ARB = 0x2092,
	FB_GFX3_GLX_CONTEXT_PROFILE_MASK_ARB = 0x9126,
	FB_GFX3_GLX_CONTEXT_CORE_PROFILE_BIT_ARB = 0x00000001
};
#endif

#ifndef Button6
#define Button6 6
#endif
#ifndef Button7
#define Button7 7
#endif
#ifndef Button8
#define Button8 8
#endif
#ifndef Button9
#define Button9 9
#endif

typedef struct FB_GFX3_PLATFORM_X11 {
	Display *display;
	int screen;
	Window window;
	Colormap colormap;
	Cursor hidden_cursor;
	Atom delete_window;
	Atom net_wm_state;
	Atom net_wm_state_fullscreen;
	FB_GFX3_INPUT_STATE *input;
	void *opengl_library;
#if !defined(DISABLE_OPENGL)
	FB_GFX3_GLX_CONTEXT context;
	FB_GFX3_GLX_GET_PROC_ADDRESS glx_get_proc_address;
	FB_GFX3_GLX_DESTROY_CONTEXT glx_destroy_context;
	FB_GFX3_GLX_MAKE_CURRENT glx_make_current;
	FB_GFX3_GLX_SWAP_BUFFERS glx_swap_buffers;
#endif
	Time last_click_time;
	int owns_colormap;
	int shown;
	int close_requested;
	int cursor_visible;
	int mouse_clip;
	int pointer_grabbed;
	uint32_t flags;
	uint32_t logical_width;
	uint32_t logical_height;
	uint32_t view_width;
	uint32_t view_height;
} FB_GFX3_PLATFORM_X11;

/* ------------------------------------------------------------------------- */
/* Native window lifecycle                                                   */
/* ------------------------------------------------------------------------- */

static Cursor platform_x11_create_hidden_cursor(Display *display,
	Window window)
{
	static const char empty_data[1] = { 0 };
	XColor color;
	Pixmap bitmap;
	Cursor cursor = None;

	memset(&color, 0, sizeof(color));
	bitmap = XCreateBitmapFromData(display, window, empty_data, 1, 1);
	if (bitmap != None) {
		cursor = XCreatePixmapCursor(display, bitmap, bitmap, &color, &color,
			0, 0);
		XFreePixmap(display, bitmap);
	}
	return cursor;
}

static void platform_x11_client_to_logical(FB_GFX3_PLATFORM_X11 *platform,
	int client_x, int client_y, int *logical_x, int *logical_y)
{
	FB_GFX3_PRESENTATION_LAYOUT layout;

	if ((platform == NULL) || (platform->flags & FB_GFX3_WINDOW_RESIZABLE) ||
	    (fb_gfx3_platform_presentation_layout(platform->logical_width,
	     platform->logical_height, platform->view_width,
	     platform->view_height, &layout) != FB_GFX3_OK)) {
		if (logical_x != NULL)
			*logical_x = client_x;
		if (logical_y != NULL)
			*logical_y = client_y;
		return;
	}
	fb_gfx3_platform_client_to_logical(&layout, platform->logical_width,
		platform->logical_height, client_x, client_y, logical_x, logical_y);
}

static void platform_x11_logical_to_client(FB_GFX3_PLATFORM_X11 *platform,
	int logical_x, int logical_y, int *client_x, int *client_y)
{
	FB_GFX3_PRESENTATION_LAYOUT layout;

	if ((platform == NULL) || (platform->flags & FB_GFX3_WINDOW_RESIZABLE) ||
	    (fb_gfx3_platform_presentation_layout(platform->logical_width,
	     platform->logical_height, platform->view_width,
	     platform->view_height, &layout) != FB_GFX3_OK)) {
		if (client_x != NULL)
			*client_x = logical_x;
		if (client_y != NULL)
			*client_y = logical_y;
		return;
	}
	fb_gfx3_platform_logical_to_client(&layout, platform->logical_width,
		platform->logical_height, logical_x, logical_y, client_x, client_y);
}

static void platform_x11_publish_mouse_move(FB_GFX3_PLATFORM_X11 *platform,
	int client_x, int client_y)
{
	int logical_x;
	int logical_y;

	platform_x11_client_to_logical(platform, client_x, client_y,
		&logical_x, &logical_y);
	fb_gfx3_input_platform_mouse_move(platform->input, logical_x, logical_y);
}

static void platform_x11_publish_window_info(FB_GFX3_PLATFORM_X11 *platform)
{
	Window child;
	int x = 0;
	int y = 0;

	if (platform == NULL)
		return;
	XTranslateCoordinates(platform->display, platform->window,
		RootWindow(platform->display, platform->screen), 0, 0,
		&x, &y, &child);
	fb_gfx3_input_platform_window_info(platform->input,
		(uintptr_t)platform->window, (uintptr_t)platform->display, x, y,
		DisplayWidth(platform->display, platform->screen),
		DisplayHeight(platform->display, platform->screen));
}

/*
	Most modern X11 window managers implement the EWMH fullscreen request, but
	the protocol is deliberately a request rather than an order.  The Motif
	decoration hint supplies the matching no-frame intent before mapping, while
	the EWMH client message asks the window manager to make the window fullscreen
	after it becomes visible.  The direct root-size resize is a useful fallback
	for compact or older window managers that do not advertise EWMH support.

	gfxlib3 never changes an XRandR mode here.  Fullscreen consumes the current
	desktop area and leaves the user's display configuration untouched.
*/
typedef struct FB_GFX3_X11_MOTIF_HINTS {
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long input_mode;
	unsigned long status;
} FB_GFX3_X11_MOTIF_HINTS;

#define FB_GFX3_X11_MOTIF_HINTS_DECORATIONS (1UL << 1)

static void platform_x11_remove_decorations(FB_GFX3_PLATFORM_X11 *platform)
{
	FB_GFX3_X11_MOTIF_HINTS hints;
	Atom property;

	if (platform == NULL)
		return;
	property = XInternAtom(platform->display, "_MOTIF_WM_HINTS", False);
	if (property == None)
		return;
	memset(&hints, 0, sizeof(hints));
	hints.flags = FB_GFX3_X11_MOTIF_HINTS_DECORATIONS;
	hints.decorations = 0;
	XChangeProperty(platform->display, platform->window, property, property,
		32, PropModeReplace, (const unsigned char *)&hints, 5);
}

static void platform_x11_request_fullscreen(FB_GFX3_PLATFORM_X11 *platform)
{
	XEvent event;
	Window root;

	if ((platform == NULL) ||
	    !(platform->flags & FB_GFX3_WINDOW_FULLSCREEN))
		return;
	root = RootWindow(platform->display, platform->screen);
	XMoveResizeWindow(platform->display, platform->window, 0, 0,
		(unsigned int)DisplayWidth(platform->display, platform->screen),
		(unsigned int)DisplayHeight(platform->display, platform->screen));
	if ((platform->net_wm_state == None) ||
	    (platform->net_wm_state_fullscreen == None))
		return;
	memset(&event, 0, sizeof(event));
	event.xclient.type = ClientMessage;
	event.xclient.window = platform->window;
	event.xclient.message_type = platform->net_wm_state;
	event.xclient.format = 32;
	event.xclient.data.l[0] = 1; /* _NET_WM_STATE_ADD */
	event.xclient.data.l[1] = (long)platform->net_wm_state_fullscreen;
	XSendEvent(platform->display, root, False,
		SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

/*
	The caller transfers ownership of both the Display connection and any
	owned colormap at entry.  Keeping the GLX FBConfig, visual, colormap, and
	window on one Display is required by Xlib.  Resources created on one
	connection cannot safely be reused with another connection, even when both
	connections name the same X server.
*/
static int platform_x11_create_window_on_display(void **destination,
	void *input, uint32_t width, uint32_t height, const char *title,
	Display *display, int screen, Visual *visual, int depth,
	Colormap colormap, int owns_colormap, uint32_t flags)
{
	FB_GFX3_PLATFORM_X11 *platform;
	XSetWindowAttributes attributes;
	XSizeHints size_hints;
	unsigned long attribute_mask;

	if ((destination == NULL) || (display == NULL) || (screen < 0) ||
	    (screen >= ScreenCount(display)) || (width == 0) || (height == 0) ||
	    (width > INT_MAX) || (height > INT_MAX))
		goto invalid;
	*destination = NULL;
	platform = (FB_GFX3_PLATFORM_X11 *)calloc(1, sizeof(*platform));
	if (platform == NULL) {
		if (owns_colormap && (colormap != None))
			XFreeColormap(display, colormap);
		XCloseDisplay(display);
		return FB_GFX3_OUT_OF_MEMORY;
	}
	platform->display = display;
	platform->screen = screen;
	platform->input = (FB_GFX3_INPUT_STATE *)input;
	platform->cursor_visible = TRUE;
	platform->flags = flags;
	platform->logical_width = width;
	platform->logical_height = height;
	platform->view_width = width;
	platform->view_height = height;
	if (visual == NULL)
		visual = DefaultVisual(display, screen);
	if (depth == 0)
		depth = DefaultDepth(display, screen);
	if (colormap == None)
		colormap = DefaultColormap(display, screen);
	platform->colormap = colormap;
	platform->owns_colormap = owns_colormap;
	if (flags & FB_GFX3_WINDOW_FULLSCREEN) {
		width = (uint32_t)DisplayWidth(display, screen);
		height = (uint32_t)DisplayHeight(display, screen);
		if ((width == 0) || (height == 0))
			goto fail;
	}
	platform->view_width = width;
	platform->view_height = height;
	memset(&attributes, 0, sizeof(attributes));
	attributes.colormap = colormap;
	attributes.event_mask = KeyPressMask | KeyReleaseMask |
		ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
		EnterWindowMask | LeaveWindowMask | FocusChangeMask |
		StructureNotifyMask;
	attribute_mask = CWColormap | CWEventMask;
	platform->window = XCreateWindow(platform->display,
		RootWindow(platform->display, platform->screen), 0, 0,
		(unsigned int)width, (unsigned int)height, 0, depth, InputOutput,
		visual, attribute_mask, &attributes);
	if (platform->window == None)
		goto fail;
	memset(&size_hints, 0, sizeof(size_hints));
	if (flags & FB_GFX3_WINDOW_RESIZABLE) {
		size_hints.flags = PMinSize;
		size_hints.min_width = 8;
		/* The tallest built-in text font must retain one usable row. */
		size_hints.min_height = 16;
	} else {
		size_hints.flags = PMinSize | PMaxSize;
		size_hints.min_width = (int)width;
		size_hints.min_height = (int)height;
		if (flags & FB_GFX3_WINDOW_NO_SWITCH) {
			size_hints.max_width = (int)width;
			size_hints.max_height = (int)height;
		} else {
			size_hints.max_width = DisplayWidth(display, screen);
			size_hints.max_height = DisplayHeight(display, screen);
		}
	}
	XSetWMNormalHints(platform->display, platform->window, &size_hints);
	platform->delete_window = XInternAtom(platform->display,
		"WM_DELETE_WINDOW", False);
	platform->net_wm_state = XInternAtom(platform->display,
		"_NET_WM_STATE", False);
	platform->net_wm_state_fullscreen = XInternAtom(platform->display,
		"_NET_WM_STATE_FULLSCREEN", False);
	if (platform->delete_window != None)
		XSetWMProtocols(platform->display, platform->window,
			&platform->delete_window, 1);
	XStoreName(platform->display, platform->window,
		(title != NULL) ? title : "FreeBASIC gfxlib3");
	if (flags & (FB_GFX3_WINDOW_FULLSCREEN | FB_GFX3_WINDOW_NO_FRAME))
		platform_x11_remove_decorations(platform);
	platform->hidden_cursor = platform_x11_create_hidden_cursor(
		platform->display, platform->window);
	fb_hInitX11KeycodeToScancodeTb(platform->display, XDisplayKeycodes,
		XGetKeyboardMapping, XFree);
	platform_x11_publish_window_info(platform);
	XFlush(platform->display);
	*destination = platform;
	return FB_GFX3_OK;

fail:
	if (platform->window != None)
		XDestroyWindow(platform->display, platform->window);
	if (platform->owns_colormap && (platform->colormap != None))
		XFreeColormap(platform->display, platform->colormap);
	XCloseDisplay(platform->display);
	free(platform);
	return FB_GFX3_FAILED;

invalid:
	if (destination != NULL)
		*destination = NULL;
	if (display != NULL) {
		if (owns_colormap && (colormap != None))
			XFreeColormap(display, colormap);
		XCloseDisplay(display);
	}
	return FB_GFX3_INVALID;
}

static int platform_x11_create_window(void **destination,
	const FB_GFX3_PLATFORM_WINDOW_CONFIG *config)
{
	Display *display;

	if (config == NULL)
		return FB_GFX3_INVALID;
	display = XOpenDisplay(NULL);
	if (display == NULL)
		return FB_GFX3_UNSUPPORTED;
	return platform_x11_create_window_on_display(destination, config->input,
		config->width, config->height, config->title, display,
		DefaultScreen(display), NULL, 0, None, FALSE, config->flags);
}

static void platform_x11_destroy(void *state)
{
	FB_GFX3_PLATFORM_X11 *platform = (FB_GFX3_PLATFORM_X11 *)state;

	if (platform == NULL)
		return;
	fb_gfx3_input_platform_window_info(platform->input, 0, 0, 0, 0, 0, 0);
	if (platform->display != NULL) {
#if !defined(DISABLE_OPENGL)
		if ((platform->glx_make_current != NULL) &&
		    (platform->context != NULL))
			platform->glx_make_current(platform->display, None, NULL);
		if ((platform->glx_destroy_context != NULL) &&
		    (platform->context != NULL))
			platform->glx_destroy_context(platform->display,
				platform->context);
#endif
		if (platform->pointer_grabbed)
			XUngrabPointer(platform->display, CurrentTime);
		if (platform->hidden_cursor != None)
			XFreeCursor(platform->display, platform->hidden_cursor);
		if (platform->window != None)
			XDestroyWindow(platform->display, platform->window);
		if (platform->owns_colormap && (platform->colormap != None))
			XFreeColormap(platform->display, platform->colormap);
		XCloseDisplay(platform->display);
	}
	if (platform->opengl_library != NULL)
		dlclose(platform->opengl_library);
	free(platform);
}

/* ------------------------------------------------------------------------- */
/* GLX ownership                                                             */
/* ------------------------------------------------------------------------- */

#if !defined(DISABLE_OPENGL)

/*
	POSIX specifies that dlsym() can return function symbols through its void
	pointer result.  Copying the pointer representation avoids casts between
	object and function pointer types and keeps strict compiler diagnostics
	useful on platforms where those types are declared differently.
*/
static int platform_x11_load_library_function(void *library,
	const char *name, void *destination, size_t destination_size)
{
	void *symbol;

	if ((library == NULL) || (name == NULL) || (destination == NULL) ||
	    (destination_size != sizeof(symbol)))
		return FB_GFX3_INVALID;
	symbol = dlsym(library, name);
	if (symbol == NULL)
		return FB_GFX3_UNSUPPORTED;
	memcpy(destination, &symbol, sizeof(symbol));
	return FB_GFX3_OK;
}

static int platform_x11_load_extension_function(
	FB_GFX3_GLX_GET_PROC_ADDRESS get_proc_address, const char *name,
	void *destination, size_t destination_size)
{
	FB_GFX3_GLX_VOID_FUNCTION symbol;

	if ((get_proc_address == NULL) || (name == NULL) ||
	    (destination == NULL) || (destination_size != sizeof(symbol)))
		return FB_GFX3_INVALID;
	symbol = get_proc_address((const unsigned char *)name);
	if (symbol == NULL)
		return FB_GFX3_UNSUPPORTED;
	memcpy(destination, &symbol, sizeof(symbol));
	return FB_GFX3_OK;
}

static int platform_x11_probe_opengl(void)
{
	void *library = dlopen("libGL.so.1", RTLD_NOW | RTLD_LOCAL);

	if (library == NULL)
		return FB_GFX3_UNSUPPORTED;
	dlclose(library);
	return FB_GFX3_OK;
}

static int platform_x11_create_opengl(void **destination,
	const FB_GFX3_PLATFORM_OPENGL_CONFIG *config)
{
	FB_GFX3_PLATFORM_X11 *platform = NULL;
	FB_GFX3_GLX_CHOOSE_FB_CONFIG choose_fb_config = NULL;
	FB_GFX3_GLX_GET_VISUAL get_visual = NULL;
	FB_GFX3_GLX_GET_PROC_ADDRESS get_proc_address = NULL;
	FB_GFX3_GLX_DESTROY_CONTEXT destroy_context = NULL;
	FB_GFX3_GLX_MAKE_CURRENT make_current = NULL;
	FB_GFX3_GLX_SWAP_BUFFERS swap_buffers = NULL;
	FB_GFX3_GLX_CREATE_CONTEXT_ATTRIBUTES create_context = NULL;
	FB_GFX3_GLX_FB_CONFIG *configs = NULL;
	XVisualInfo *visual = NULL;
	Display *display = NULL;
	void *library = NULL;
	Colormap colormap = None;
	int config_count = 0;
	int framebuffer_attributes[] = {
		FB_GFX3_GLX_X_RENDERABLE, True,
		FB_GFX3_GLX_DRAWABLE_TYPE, FB_GFX3_GLX_WINDOW_BIT,
		FB_GFX3_GLX_RENDER_TYPE, FB_GFX3_GLX_RGBA_BIT,
		FB_GFX3_GLX_X_VISUAL_TYPE, FB_GFX3_GLX_TRUE_COLOR,
		FB_GFX3_GLX_RED_SIZE, 8,
		FB_GFX3_GLX_GREEN_SIZE, 8,
		FB_GFX3_GLX_BLUE_SIZE, 8,
		FB_GFX3_GLX_ALPHA_SIZE, 8,
		FB_GFX3_GLX_DOUBLEBUFFER, True,
		None
	};
	int context_attributes[] = {
		FB_GFX3_GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
		FB_GFX3_GLX_CONTEXT_MINOR_VERSION_ARB, 3,
		FB_GFX3_GLX_CONTEXT_PROFILE_MASK_ARB,
		FB_GFX3_GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		None
	};
	int result = FB_GFX3_FAILED;

	if ((destination == NULL) || (config == NULL) || (config->width == 0) ||
	    (config->height == 0) || (config->width > INT_MAX) ||
	    (config->height > INT_MAX) || (config->major_version > INT_MAX) ||
	    (config->minor_version > INT_MAX))
		return FB_GFX3_INVALID;
	*destination = NULL;
	display = XOpenDisplay(NULL);
	if (display == NULL)
		return FB_GFX3_UNSUPPORTED;
	library = dlopen("libGL.so.1", RTLD_NOW | RTLD_LOCAL);
	if (library == NULL) {
		result = FB_GFX3_UNSUPPORTED;
		goto cleanup;
	}
	if ((platform_x11_load_library_function(library, "glXChooseFBConfig",
	     &choose_fb_config, sizeof(choose_fb_config)) != FB_GFX3_OK) ||
	    (platform_x11_load_library_function(library,
	     "glXGetVisualFromFBConfig", &get_visual,
	     sizeof(get_visual)) != FB_GFX3_OK) ||
	    (platform_x11_load_library_function(library,
	     "glXGetProcAddressARB", &get_proc_address,
	     sizeof(get_proc_address)) != FB_GFX3_OK) ||
	    (platform_x11_load_library_function(library, "glXDestroyContext",
	     &destroy_context, sizeof(destroy_context)) != FB_GFX3_OK) ||
	    (platform_x11_load_library_function(library, "glXMakeCurrent",
	     &make_current, sizeof(make_current)) != FB_GFX3_OK) ||
	    (platform_x11_load_library_function(library, "glXSwapBuffers",
	     &swap_buffers, sizeof(swap_buffers)) != FB_GFX3_OK)) {
		result = FB_GFX3_UNSUPPORTED;
		goto cleanup;
	}
	configs = choose_fb_config(display, DefaultScreen(display),
		framebuffer_attributes,
		&config_count);
	if ((configs == NULL) || (config_count <= 0)) {
		result = FB_GFX3_UNSUPPORTED;
		goto cleanup;
	}
	visual = get_visual(display, configs[0]);
	if (visual == NULL) {
		result = FB_GFX3_UNSUPPORTED;
		goto cleanup;
	}
	colormap = XCreateColormap(display, RootWindow(display, visual->screen),
		visual->visual,
		AllocNone);
	if (colormap == None)
		goto cleanup;
	result = platform_x11_create_window_on_display(destination, config->input,
		config->width, config->height, config->title, display,
		visual->screen, visual->visual, visual->depth, colormap, TRUE,
		config->flags);
	/* The window helper owns these resources even when creation fails. */
	display = NULL;
	colormap = None;
	if (result != FB_GFX3_OK)
		goto cleanup;
	platform = (FB_GFX3_PLATFORM_X11 *)*destination;
	platform->opengl_library = library;
	library = NULL;
	platform->glx_get_proc_address = get_proc_address;
	platform->glx_destroy_context = destroy_context;
	platform->glx_make_current = make_current;
	platform->glx_swap_buffers = swap_buffers;
	if (platform_x11_load_extension_function(get_proc_address,
	    "glXCreateContextAttribsARB", &create_context,
	    sizeof(create_context)) != FB_GFX3_OK) {
		result = FB_GFX3_UNSUPPORTED;
		goto cleanup;
	}
	context_attributes[1] = (int)config->major_version;
	context_attributes[3] = (int)config->minor_version;
	platform->context = create_context(platform->display, configs[0], NULL,
		True, context_attributes);
	if ((platform->context == NULL) ||
	    !platform->glx_make_current(platform->display, platform->window,
	     platform->context)) {
		result = FB_GFX3_UNSUPPORTED;
		goto cleanup;
	}
	result = FB_GFX3_OK;

cleanup:
	if (visual != NULL)
		XFree(visual);
	if (configs != NULL)
		XFree(configs);
	if ((result != FB_GFX3_OK) && (platform != NULL)) {
		platform_x11_destroy(platform);
		*destination = NULL;
	}
	if (display != NULL) {
		if (colormap != None)
			XFreeColormap(display, colormap);
		XCloseDisplay(display);
	}
	if (library != NULL)
		dlclose(library);
	return result;
}

static int platform_x11_load_opengl_function(void *state, const char *name,
	void *destination, size_t destination_size)
{
	FB_GFX3_PLATFORM_X11 *platform = (FB_GFX3_PLATFORM_X11 *)state;

	if ((platform == NULL) || (name == NULL) || (destination == NULL))
		return FB_GFX3_INVALID;
	if (platform_x11_load_extension_function(platform->glx_get_proc_address,
	    name, destination, destination_size) == FB_GFX3_OK)
		return FB_GFX3_OK;
	return platform_x11_load_library_function(platform->opengl_library, name,
		destination, destination_size);
}

static int platform_x11_swap_buffers(void *state)
{
	FB_GFX3_PLATFORM_X11 *platform = (FB_GFX3_PLATFORM_X11 *)state;

	if ((platform == NULL) || (platform->context == NULL))
		return FB_GFX3_INVALID;
	platform->glx_swap_buffers(platform->display, platform->window);
	return FB_GFX3_OK;
}

#else

static int platform_x11_probe_opengl(void)
{
	return FB_GFX3_UNSUPPORTED;
}

#define platform_x11_create_opengl NULL
#define platform_x11_load_opengl_function NULL
#define platform_x11_swap_buffers NULL

#endif

/* ------------------------------------------------------------------------- */
/* Event translation and window operations                                   */
/* ------------------------------------------------------------------------- */

static int platform_x11_translate_key(XKeyEvent *event, int scancode)
{
	char text[8];
	int length;

	memset(text, 0, sizeof(text));
	length = XLookupString(event, text, sizeof(text), NULL, NULL);
	if (length == 1) {
		if ((unsigned char)text[0] == 0x7F)
			return KEY_DEL;
		return (unsigned char)text[0];
	}
	return fb_hScancodeToExtendedKey(scancode);
}

static int platform_x11_key_repeat(FB_GFX3_PLATFORM_X11 *platform,
	XKeyEvent *release, XKeyEvent *press)
{
	XEvent next;

	if (!XPending(platform->display))
		return FALSE;
	XPeekEvent(platform->display, &next);
	if ((next.type != KeyPress) ||
	    (next.xkey.keycode != release->keycode) ||
	    ((next.xkey.time - release->time) >= 2))
		return FALSE;
	XNextEvent(platform->display, &next);
	*press = next.xkey;
	return TRUE;
}

/*
	X11 pointer confinement is represented by two states.  mouse_clip is the
	BASIC program's durable request, while pointer_grabbed records whether the
	server currently granted the grab.  FocusOut must release the server grab;
	FocusIn may restore it without losing the caller's original request.
*/
static void platform_x11_set_pointer_grab(FB_GFX3_PLATFORM_X11 *platform,
	int grab)
{
	int result;

	if (!grab) {
		if (platform->pointer_grabbed)
			XUngrabPointer(platform->display, CurrentTime);
		platform->pointer_grabbed = FALSE;
		return;
	}
	if (platform->pointer_grabbed)
		return;
	result = XGrabPointer(platform->display, platform->window, True,
		PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
		GrabModeAsync, GrabModeAsync, platform->window, None, CurrentTime);
	platform->pointer_grabbed = (result == GrabSuccess);
}

static int platform_x11_scancode(unsigned int native_keycode)
{
	if (native_keycode >= sizeof(fb_x11keycode_to_scancode))
		return 0;
	return fb_x11keycode_to_scancode[native_keycode];
}

/* Buttons 8 and 9 are the conventional X11 side-button assignments. */
static int platform_x11_mouse_button(unsigned int native_button)
{
	switch (native_button) {
	case Button1:
		return BUTTON_LEFT;
	case Button2:
		return BUTTON_MIDDLE;
	case Button3:
		return BUTTON_RIGHT;
	case Button8:
		return BUTTON_X1;
	case Button9:
		return BUTTON_X2;
	default:
		return 0;
	}
}

static void platform_x11_apply_requests(FB_GFX3_PLATFORM_X11 *platform)
{
	FB_GFX3_MOUSE_REQUEST mouse;
	FB_GFX3_WINDOW_REQUEST window;
	int client_x;
	int client_y;

	if (fb_gfx3_input_platform_take_window_request(platform->input,
	    &window) && (window.flags & FB_GFX3_WINDOW_REQUEST_POSITION)) {
		XMoveWindow(platform->display, platform->window, window.x, window.y);
		fb_gfx3_input_platform_window_moved(platform->input,
			window.x, window.y);
	}
	if (!fb_gfx3_input_platform_take_mouse_request(platform->input, &mouse))
		return;
	if (mouse.flags & FB_GFX3_MOUSE_REQUEST_POSITION) {
		platform_x11_logical_to_client(platform, mouse.x, mouse.y,
			&client_x, &client_y);
		XWarpPointer(platform->display, None, platform->window, 0, 0, 0, 0,
			client_x, client_y);
	}
	if (mouse.flags & FB_GFX3_MOUSE_REQUEST_CURSOR) {
		platform->cursor_visible = (mouse.cursor != 0);
		if (platform->cursor_visible)
			XUndefineCursor(platform->display, platform->window);
		else if (platform->hidden_cursor != None)
			XDefineCursor(platform->display, platform->window,
				platform->hidden_cursor);
	}
	if (mouse.flags & FB_GFX3_MOUSE_REQUEST_CLIP) {
		platform->mouse_clip = (mouse.clip != 0);
		platform_x11_set_pointer_grab(platform, platform->mouse_clip);
	}
	XFlush(platform->display);
}

static void platform_x11_pump_events(void *state)
{
	FB_GFX3_PLATFORM_X11 *platform = (FB_GFX3_PLATFORM_X11 *)state;
	XEvent event;
	int button;

	if (platform == NULL)
		return;
	platform_x11_apply_requests(platform);
	while (XPending(platform->display)) {
		XNextEvent(platform->display, &event);
		switch (event.type) {
		case FocusIn:
			fb_gfx3_input_platform_focus(platform->input, TRUE);
			if (platform->mouse_clip)
				platform_x11_set_pointer_grab(platform, TRUE);
			break;
		case FocusOut:
			fb_gfx3_input_platform_focus(platform->input, FALSE);
			platform_x11_set_pointer_grab(platform, FALSE);
			break;
		case EnterNotify:
			fb_gfx3_input_platform_mouse_enter(platform->input);
			platform_x11_publish_mouse_move(platform,
				event.xcrossing.x, event.xcrossing.y);
			break;
		case LeaveNotify:
			fb_gfx3_input_platform_mouse_exit(platform->input);
			break;
		case MotionNotify:
			fb_gfx3_input_platform_mouse_enter(platform->input);
			platform_x11_publish_mouse_move(platform,
				event.xmotion.x, event.xmotion.y);
			break;
		case ButtonPress:
			fb_gfx3_input_platform_mouse_enter(platform->input);
			platform_x11_publish_mouse_move(platform,
				event.xbutton.x, event.xbutton.y);
			if ((event.xbutton.button >= Button4) &&
			    (event.xbutton.button <= Button7)) {
				fb_gfx3_input_platform_mouse_wheel(platform->input,
					event.xbutton.button >= Button6,
					((event.xbutton.button == Button4) ||
					 (event.xbutton.button == Button7)) ? 1 : -1);
				break;
			}
			button = platform_x11_mouse_button(event.xbutton.button);
			if (button == 0)
				break;
			fb_gfx3_input_platform_mouse_button(platform->input, button,
				TRUE, (event.xbutton.time - platform->last_click_time) <
				250);
			platform->last_click_time = event.xbutton.time;
			break;
		case ButtonRelease:
			if ((event.xbutton.button >= Button4) &&
			    (event.xbutton.button <= Button7))
				break;
			button = platform_x11_mouse_button(event.xbutton.button);
			if (button == 0)
				break;
			fb_gfx3_input_platform_mouse_button(platform->input, button,
				FALSE, FALSE);
			break;
		case KeyPress:
			{
				int scancode = platform_x11_scancode(
					event.xkey.keycode);
				int key = platform_x11_translate_key(&event.xkey,
					scancode);

				fb_gfx3_input_platform_key(platform->input,
					EVENT_KEY_PRESS, scancode,
					((key > 0) && (key <= 0xFF)) ? key : 0);
				fb_gfx3_input_platform_character(platform->input, key, 1);
			}
			break;
		case KeyRelease:
			{
				XKeyEvent press;
				int repeated = platform_x11_key_repeat(platform,
					&event.xkey, &press);
				XKeyEvent *translated = repeated ? &press : &event.xkey;
				int scancode = platform_x11_scancode(
					translated->keycode);
				int key = platform_x11_translate_key(translated,
					scancode);

				fb_gfx3_input_platform_key(platform->input,
					repeated ? EVENT_KEY_REPEAT : EVENT_KEY_RELEASE,
					scancode, ((key > 0) && (key <= 0xFF)) ? key : 0);
				if (repeated)
					fb_gfx3_input_platform_character(platform->input,
						key, 1);
			}
			break;
		case ConfigureNotify:
			platform_x11_publish_window_info(platform);
			if ((event.xconfigure.window == platform->window) &&
			    (event.xconfigure.width > 0) &&
			    (event.xconfigure.height > 0)) {
				platform->view_width = (uint32_t)event.xconfigure.width;
				platform->view_height = (uint32_t)event.xconfigure.height;
			}
			if ((platform->flags & FB_GFX3_WINDOW_RESIZABLE) &&
			    (event.xconfigure.window == platform->window) &&
			    (event.xconfigure.width > 0) &&
			    (event.xconfigure.height > 0))
				fb_gfx3_input_platform_resize(platform->input,
					(uint32_t)event.xconfigure.width,
					(uint32_t)event.xconfigure.height);
			break;
		case ClientMessage:
			if ((Atom)event.xclient.data.l[0] ==
			    platform->delete_window) {
				platform->close_requested = TRUE;
				XUnmapWindow(platform->display, platform->window);
				fb_gfx3_input_platform_close(platform->input);
			}
			break;
		default:
			break;
		}
	}
}

static int platform_x11_native_handles(void *state, uintptr_t *instance,
	uintptr_t *window)
{
	FB_GFX3_PLATFORM_X11 *platform = (FB_GFX3_PLATFORM_X11 *)state;

	if ((platform == NULL) || (instance == NULL) || (window == NULL))
		return FB_GFX3_INVALID;
	*instance = (uintptr_t)platform->display;
	*window = (uintptr_t)platform->window;
	return FB_GFX3_OK;
}

static int platform_x11_client_size(void *state, uint32_t *width,
	uint32_t *height)
{
	FB_GFX3_PLATFORM_X11 *platform = (FB_GFX3_PLATFORM_X11 *)state;
	XWindowAttributes attributes;

	if ((platform == NULL) || (width == NULL) || (height == NULL))
		return FB_GFX3_INVALID;
	if (!XGetWindowAttributes(platform->display, platform->window,
	    &attributes) || (attributes.width < 0) || (attributes.height < 0))
		return FB_GFX3_FAILED;
	*width = (uint32_t)attributes.width;
	*height = (uint32_t)attributes.height;
	return FB_GFX3_OK;
}

static int platform_x11_desktop_info(ssize_t *width, ssize_t *height,
	ssize_t *depth, ssize_t *refresh)
{
	Display *display;
	int screen;
	int desktop_width;
	int desktop_height;
	int desktop_depth;

	display = XOpenDisplay(NULL);
	if (display == NULL)
		return FB_GFX3_UNSUPPORTED;
	screen = DefaultScreen(display);
	desktop_width = DisplayWidth(display, screen);
	desktop_height = DisplayHeight(display, screen);
	desktop_depth = DefaultDepth(display, screen);
	XCloseDisplay(display);
	if ((desktop_width <= 0) || (desktop_height <= 0) ||
	    (desktop_depth <= 0))
		return FB_GFX3_FAILED;
	if (width != NULL)
		*width = desktop_width;
	if (height != NULL)
		*height = desktop_height;
	if (depth != NULL)
		*depth = desktop_depth;
	if (refresh != NULL)
		*refresh = 0;
	return FB_GFX3_OK;
}

static int platform_x11_show_window(void *state)
{
	FB_GFX3_PLATFORM_X11 *platform = (FB_GFX3_PLATFORM_X11 *)state;

	if (platform == NULL)
		return FB_GFX3_INVALID;
	if (!platform->shown) {
		XMapRaised(platform->display, platform->window);
		platform_x11_request_fullscreen(platform);
		XFlush(platform->display);
		platform->shown = TRUE;
	}
	return FB_GFX3_OK;
}

static int platform_x11_set_window_title(void *state, const char *title)
{
	FB_GFX3_PLATFORM_X11 *platform = (FB_GFX3_PLATFORM_X11 *)state;

	if ((platform == NULL) || (title == NULL))
		return FB_GFX3_INVALID;
	XStoreName(platform->display, platform->window, title);
	XFlush(platform->display);
	return FB_GFX3_OK;
}

static const FB_GFX3_PLATFORM_VTABLE __fb_gfx3_platform_x11 = {
	"X11",
	platform_x11_probe_opengl,
	platform_x11_create_window,
	platform_x11_create_opengl,
	platform_x11_native_handles,
	platform_x11_destroy,
	platform_x11_load_opengl_function,
	platform_x11_client_size,
	platform_x11_desktop_info,
	platform_x11_swap_buffers,
	platform_x11_pump_events,
	platform_x11_show_window,
	platform_x11_set_window_title
};

#else

static const FB_GFX3_PLATFORM_VTABLE __fb_gfx3_platform_x11 = {
	"X11 unavailable",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

#endif

int fb_gfx3_platform_keyboard_overlay(void *platform,
	FB_GFX3_ANDROID_KEYBOARD_OVERLAY *overlay)
{
	(void)platform;
	if (overlay != NULL)
		memset(overlay, 0, sizeof(*overlay));
	return FB_GFX3_UNSUPPORTED;
}

const FB_GFX3_PLATFORM_VTABLE *fb_gfx3_platform_default(void)
{
	return &__fb_gfx3_platform_x11;
}

/* end of linux/gfx3_platform.c */
