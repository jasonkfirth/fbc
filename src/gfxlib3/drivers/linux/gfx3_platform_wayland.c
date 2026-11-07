/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: linux/gfx3_platform_wayland.c

    Purpose:

        Create a compositor-managed Wayland window and EGL context for gfxlib3.

    Responsibilities:

        - bind an xdg-shell toplevel without requiring Wayland build headers
        - create desktop OpenGL or OpenGL ES contexts through EGL
        - report compositor window geometry and present rendered frames
        - keep the compositor in control of display modes and window placement

    This file intentionally does NOT contain:

        - GLES rendering commands or shader resources
        - DRM/KMS ownership or X11 compatibility behavior
        - application-specific mobile layout decisions

    The renderer thread owns the Wayland and EGL objects. It pumps nonblocking
    Wayland events before querying geometry or swapping the next frame.
*/

#define _POSIX_C_SOURCE 200809L

#include "gfx3_platform_wayland.h"

#include "../../gfx3_input.h"
#include "../../../gfxlib2/drivers/linux/fb_linux_device.h"
#include "../../../gfxlib2/drivers/linux/fb_linux_maliit.h"
#include "../../../gfxlib2/drivers/linux/fb_wayland_client.h"
#include "../../../gfxlib2/drivers/linux/fb_wayland_display.h"
#include "../../../gfxlib2/drivers/linux/fb_wayland_keymap.h"
#include "../../../gfxlib2/drivers/linux/fb_wayland_text_v3.h"
#include "../../../gfxlib2/drivers/mobile_control.h"

#include <dlfcn.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

struct wl_display;
struct wl_proxy;
struct wl_interface;
struct wl_array;
struct xkb_context;
struct xkb_keymap;
struct xkb_state;

extern const struct wl_interface xdg_wm_base_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_toplevel_interface;
extern const struct wl_interface zwp_text_input_manager_v3_interface;
extern const struct wl_interface zwp_text_input_v3_interface;
extern const struct wl_interface zwp_text_input_manager_v2_interface;
extern const struct wl_interface zwp_text_input_v2_interface;

#define FB_GFX3_WL_SEAT_GET_POINTER 0u
#define FB_GFX3_WL_SEAT_GET_KEYBOARD 1u
#define FB_GFX3_WL_SEAT_GET_TOUCH 2u
#define FB_GFX3_WL_SEAT_RELEASE 3u
#define FB_GFX3_WL_POINTER_RELEASE 1u
#define FB_GFX3_WL_KEYBOARD_RELEASE 0u
#define FB_GFX3_WL_TOUCH_RELEASE 0u
#define FB_GFX3_WL_TEXT_INPUT_MANAGER_GET_TEXT_INPUT 1u
#define FB_GFX3_WL_TEXT_INPUT_ENABLE 1u
#define FB_GFX3_WL_TEXT_INPUT_DISABLE 2u
#define FB_GFX3_WL_TEXT_INPUT_SET_CONTENT_TYPE 5u
#define FB_GFX3_WL_TEXT_INPUT_SET_CURSOR_RECTANGLE 6u
#define FB_GFX3_WL_TEXT_INPUT_COMMIT 7u
#define FB_GFX3_WL_TEXT_INPUT_V2_GET_TEXT_INPUT 1u
#define FB_GFX3_WL_TEXT_INPUT_V2_ENABLE 1u
#define FB_GFX3_WL_TEXT_INPUT_V2_DISABLE 2u
#define FB_GFX3_WL_TEXT_INPUT_V2_SHOW_PANEL 3u
#define FB_GFX3_WL_TEXT_INPUT_V2_HIDE_PANEL 4u
#define FB_GFX3_WL_TEXT_INPUT_V2_SET_CONTENT_TYPE 6u
#define FB_GFX3_WL_TEXT_INPUT_V2_SET_CURSOR_RECTANGLE 7u
#define FB_GFX3_WL_TEXT_INPUT_V2_UPDATE_STATE 9u
#define FB_GFX3_WL_TEXT_INPUT_V2_UPDATE_STATE_CHANGE 0u
#define FB_GFX3_WL_TEXT_INPUT_V2_UPDATE_STATE_ENTER 3u
#define FB_GFX3_WL_SEAT_CAPABILITY_POINTER 1u
#define FB_GFX3_WL_SEAT_CAPABILITY_KEYBOARD 2u
#define FB_GFX3_WL_SEAT_CAPABILITY_TOUCH 4u
#define FB_GFX3_WL_KEYMAP_FORMAT_TEXT_V1 1u
#define FB_GFX3_WL_EVDEV_KEY_MAX 768u
#define FB_GFX3_WL_KEYMAP_MAX_BYTES (16u * 1024u * 1024u)

struct wl_array {
	size_t size;
	size_t alloc;
	void *data;
};

typedef struct FB_GFX3_WAYLAND_SEAT_LISTENER {
	void (*capabilities)(void *, struct wl_proxy *, uint32_t);
	void (*name)(void *, struct wl_proxy *, const char *);
} FB_GFX3_WAYLAND_SEAT_LISTENER;

typedef struct FB_GFX3_WAYLAND_POINTER_LISTENER {
	void (*enter)(void *, struct wl_proxy *, uint32_t, struct wl_proxy *, int32_t, int32_t);
	void (*leave)(void *, struct wl_proxy *, uint32_t, struct wl_proxy *);
	void (*motion)(void *, struct wl_proxy *, uint32_t, int32_t, int32_t);
	void (*button)(void *, struct wl_proxy *, uint32_t, uint32_t, uint32_t, uint32_t);
	void (*axis)(void *, struct wl_proxy *, uint32_t, uint32_t, int32_t);
} FB_GFX3_WAYLAND_POINTER_LISTENER;

typedef struct FB_GFX3_WAYLAND_KEYBOARD_LISTENER {
	void (*keymap)(void *, struct wl_proxy *, uint32_t, int32_t, uint32_t);
	void (*enter)(void *, struct wl_proxy *, uint32_t, struct wl_proxy *, struct wl_array *);
	void (*leave)(void *, struct wl_proxy *, uint32_t, struct wl_proxy *);
	void (*key)(void *, struct wl_proxy *, uint32_t, uint32_t, uint32_t, uint32_t);
	void (*modifiers)(void *, struct wl_proxy *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
	void (*repeat_info)(void *, struct wl_proxy *, int32_t, int32_t);
} FB_GFX3_WAYLAND_KEYBOARD_LISTENER;

typedef struct FB_GFX3_WAYLAND_TOUCH_LISTENER {
	void (*down)(void *, struct wl_proxy *, uint32_t, uint32_t, struct wl_proxy *, int32_t, int32_t, int32_t);
	void (*up)(void *, struct wl_proxy *, uint32_t, uint32_t, int32_t);
	void (*motion)(void *, struct wl_proxy *, uint32_t, int32_t, int32_t, int32_t);
	void (*frame)(void *, struct wl_proxy *);
	void (*cancel)(void *, struct wl_proxy *);
} FB_GFX3_WAYLAND_TOUCH_LISTENER;

typedef struct FB_GFX3_WAYLAND_TEXT_INPUT_V3_LISTENER {
	void (*enter)(void *, struct wl_proxy *, struct wl_proxy *);
	void (*leave)(void *, struct wl_proxy *, struct wl_proxy *);
	void (*preedit_string)(void *, struct wl_proxy *, const char *, int32_t, int32_t);
	void (*commit_string)(void *, struct wl_proxy *, const char *);
	void (*delete_surrounding_text)(void *, struct wl_proxy *, uint32_t, uint32_t);
	void (*done)(void *, struct wl_proxy *, uint32_t);
} FB_GFX3_WAYLAND_TEXT_INPUT_V3_LISTENER;

typedef struct FB_GFX3_WAYLAND_TEXT_INPUT_V2_LISTENER {
	void (*enter)(void *, struct wl_proxy *, uint32_t, struct wl_proxy *);
	void (*leave)(void *, struct wl_proxy *, uint32_t, struct wl_proxy *);
	void (*input_panel_state)(void *, struct wl_proxy *, uint32_t, int32_t, int32_t, int32_t, int32_t);
	void (*preedit_string)(void *, struct wl_proxy *, const char *, const char *);
	void (*preedit_styling)(void *, struct wl_proxy *, uint32_t, uint32_t, uint32_t);
	void (*preedit_cursor)(void *, struct wl_proxy *, int32_t);
	void (*commit_string)(void *, struct wl_proxy *, const char *);
	void (*cursor_position)(void *, struct wl_proxy *, int32_t, int32_t);
	void (*delete_surrounding_text)(void *, struct wl_proxy *, uint32_t, uint32_t);
	void (*modifiers_map)(void *, struct wl_proxy *, struct wl_array *);
	void (*keysym)(void *, struct wl_proxy *, uint32_t, uint32_t, uint32_t, uint32_t);
	void (*language)(void *, struct wl_proxy *, const char *);
	void (*text_direction)(void *, struct wl_proxy *, uint32_t);
	void (*configure_surrounding_text)(void *, struct wl_proxy *, int32_t, int32_t);
	void (*input_method_changed)(void *, struct wl_proxy *, uint32_t, uint32_t);
} FB_GFX3_WAYLAND_TEXT_INPUT_V2_LISTENER;

typedef struct FB_GFX3_WAYLAND_XKB_API {
	void *library;
	struct xkb_context *(*context_new)(int);
	void (*context_unref)(struct xkb_context *);
	struct xkb_keymap *(*keymap_new_from_string)(struct xkb_context *, const char *, int, int);
	void (*keymap_unref)(struct xkb_keymap *);
	struct xkb_state *(*state_new)(struct xkb_keymap *);
	void (*state_unref)(struct xkb_state *);
	uint32_t (*state_update_key)(struct xkb_state *, uint32_t, uint32_t);
	uint32_t (*state_update_mask)(struct xkb_state *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
	int (*state_key_get_utf8)(struct xkb_state *, uint32_t, char *, size_t);
	uint32_t (*state_key_get_one_sym)(struct xkb_state *, uint32_t);
} FB_GFX3_WAYLAND_XKB_API;

typedef struct FB_GFX3_WAYLAND_EGL {
	void *library;
	void *wayland_egl_library;
	void *gles_library;
	void *gl_library;
	void *(*get_proc_address)(const char *name);
	void *(*get_display)(void *native_display);
	void *(*get_platform_display)(uint32_t platform, void *native_display,
		const int *attributes);
	unsigned int (*initialize)(void *display, int *major, int *minor);
	unsigned int (*terminate)(void *display);
	unsigned int (*bind_api)(uint32_t api);
	unsigned int (*choose_config)(void *display, const int *attributes,
		void **configs, int config_size, int *config_count);
	void *(*create_context)(void *display, void *config, void *share,
		const int *attributes);
	unsigned int (*destroy_context)(void *display, void *context);
	void *(*create_window_surface)(void *display, void *config,
		void *native_window, const int *attributes);
	unsigned int (*destroy_surface)(void *display, void *surface);
	unsigned int (*make_current)(void *display, void *draw, void *read,
		void *context);
	unsigned int (*swap_buffers)(void *display, void *surface);
	unsigned int (*swap_interval)(void *display, int interval);
	void *(*window_create)(void *surface, int width, int height);
	void (*window_destroy)(void *window);
	void (*window_resize)(void *window, int width, int height,
		int dx, int dy);
} FB_GFX3_WAYLAND_EGL;

typedef struct FB_GFX3_PLATFORM_WAYLAND {
	const FB_WAYLAND_API *api;
	struct wl_display *display;
	struct wl_proxy *registry;
	struct wl_proxy *compositor;
	struct wl_proxy *surface;
	struct wl_proxy *seat;
	struct wl_proxy *pointer;
	struct wl_proxy *keyboard;
	struct wl_proxy *touch;
	struct wl_proxy *text_input_manager;
	struct wl_proxy *text_input;
	struct wl_proxy *text_input_manager_v2;
	struct wl_proxy *text_input_v2;
	struct wl_proxy *wm_base;
	struct wl_proxy *xdg_surface;
	struct wl_proxy *toplevel;
	struct wl_interface *registry_interface;
	struct wl_interface *compositor_interface;
	struct wl_interface *surface_interface;
	struct wl_interface *seat_interface;
	struct wl_interface *output_interface;
	struct wl_interface *pointer_interface;
	struct wl_interface *keyboard_interface;
	struct wl_interface *touch_interface;
	FB_GFX3_WAYLAND_EGL egl;
	FB_GFX3_INPUT_STATE *input;
	void *egl_display;
	void *egl_config;
	void *egl_context;
	void *egl_surface;
	void *egl_window;
	char title[256];
	uint32_t global_compositor;
	uint32_t global_wm_base;
	uint32_t compositor_version;
	uint32_t wm_base_version;
	uint32_t global_seat;
	uint32_t global_text_input_manager;
	uint32_t global_text_input_manager_v2;
	uint32_t seat_version;
	uint32_t flags;
	uint32_t view_width;
	uint32_t view_height;
	int configured;
	int shown;
	int close_requested;
	int owns_egl;
	int pointer_focused;
	int keyboard_focused;
	int text_input_entered;
	int keyboard_requested;
	int keyboard_visible;
	FB_WAYLAND_TEXT_V3_PENDING text_v3_pending;
	int region_x;
	int region_y;
	int region_width;
	int region_height;
	int pointer_buttons;
	int primary_touch_id;
	int have_primary_touch;
	int repeat_scancode;
	int repeat_key;
	int repeat_ascii;
	uint32_t repeat_unicode;
	int repeat_text_only;
	uint32_t repeat_evdev_key;
	char repeat_text[16];
	uint32_t repeat_delay_ms;
	uint32_t repeat_interval_ms;
	uint64_t repeat_deadline_ms;
	uint32_t last_click_time;
	uint32_t text_input_serial;
	int last_click_button;
	FB_GFX3_TOUCH_CONTACT touch_contacts[FB_GFX3_INPUT_TOUCH_MAX];
	uint32_t touch_count;
	unsigned char key_down[FB_GFX3_INPUT_KEY_COUNT];
	unsigned char evdev_scancode[FB_GFX3_WL_EVDEV_KEY_MAX];
	FB_GFX3_WAYLAND_XKB_API xkb;
	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;
} FB_GFX3_PLATFORM_WAYLAND;

/*
	Mesa can register thread-local destructors in these driver libraries.
	Keep one shared reference for the process lifetime so renderer-thread exit
	never calls a destructor from an unloaded module.
*/
static pthread_mutex_t fb_gfx3_wayland_egl_mutex = PTHREAD_MUTEX_INITIALIZER;
static FB_GFX3_WAYLAND_EGL fb_gfx3_wayland_egl_cache;
static int fb_gfx3_wayland_egl_cache_ready;
static int fb_gfx3_wayland_egl_cache_result = FB_GFX3_FAILED;

enum FB_GFX3_WAYLAND_EGL_CONSTANTS {
	FB_GFX3_EGL_NONE = 0x3038,
	FB_GFX3_EGL_SURFACE_TYPE = 0x3033,
	FB_GFX3_EGL_WINDOW_BIT = 0x0004,
	FB_GFX3_EGL_RENDERABLE_TYPE = 0x3040,
	FB_GFX3_EGL_OPENGL_BIT = 0x0008,
	FB_GFX3_EGL_OPENGL_ES3_BIT = 0x0040,
	FB_GFX3_EGL_RED_SIZE = 0x3024,
	FB_GFX3_EGL_GREEN_SIZE = 0x3023,
	FB_GFX3_EGL_BLUE_SIZE = 0x3022,
	FB_GFX3_EGL_ALPHA_SIZE = 0x3021,
	FB_GFX3_EGL_DEPTH_SIZE = 0x3025,
	FB_GFX3_EGL_OPENGL_API = 0x30A2,
	FB_GFX3_EGL_OPENGL_ES_API = 0x30A0,
	FB_GFX3_EGL_PLATFORM_WAYLAND = 0x31D8,
	FB_GFX3_EGL_CONTEXT_CLIENT_VERSION = 0x3098,
	FB_GFX3_EGL_CONTEXT_MINOR_VERSION = 0x30FB,
	FB_GFX3_EGL_CONTEXT_PROFILE_MASK = 0x30FD,
	FB_GFX3_EGL_CONTEXT_OPENGL_CORE_PROFILE = 0x00000001
};

enum FB_GFX3_WAYLAND_REQUESTS {
	FB_GFX3_WL_DISPLAY_GET_REGISTRY = 1,
	FB_GFX3_WL_COMPOSITOR_CREATE_SURFACE = 0,
	FB_GFX3_WL_SURFACE_DESTROY = 0,
	FB_GFX3_WL_SURFACE_COMMIT = 6,
	FB_GFX3_XDG_WM_BASE_DESTROY = 0,
	FB_GFX3_XDG_WM_BASE_GET_SURFACE = 2,
	FB_GFX3_XDG_WM_BASE_PONG = 3,
	FB_GFX3_XDG_SURFACE_DESTROY = 0,
	FB_GFX3_XDG_SURFACE_GET_TOPLEVEL = 1,
	FB_GFX3_XDG_SURFACE_ACK_CONFIGURE = 4,
	FB_GFX3_XDG_TOPLEVEL_DESTROY = 0,
	FB_GFX3_XDG_TOPLEVEL_SET_TITLE = 2,
	FB_GFX3_XDG_TOPLEVEL_SET_APP_ID = 3,
	FB_GFX3_XDG_TOPLEVEL_SET_MAXIMIZED = 9,
	FB_GFX3_XDG_TOPLEVEL_SET_FULLSCREEN = 11
};

typedef struct FB_GFX3_WAYLAND_REGISTRY_LISTENER {
	void (*global)(void *data, struct wl_proxy *registry, uint32_t name,
		const char *interface, uint32_t version);
	void (*global_remove)(void *data, struct wl_proxy *registry,
		uint32_t name);
} FB_GFX3_WAYLAND_REGISTRY_LISTENER;

typedef struct FB_GFX3_WAYLAND_WM_LISTENER {
	void (*ping)(void *data, struct wl_proxy *wm_base, uint32_t serial);
} FB_GFX3_WAYLAND_WM_LISTENER;

typedef struct FB_GFX3_WAYLAND_SURFACE_LISTENER {
	void (*configure)(void *data, struct wl_proxy *xdg_surface,
		uint32_t serial);
} FB_GFX3_WAYLAND_SURFACE_LISTENER;

typedef struct FB_GFX3_WAYLAND_TOPLEVEL_LISTENER {
	void (*configure)(void *data, struct wl_proxy *toplevel, int32_t width,
		int32_t height, struct wl_array *states);
	void (*close)(void *data, struct wl_proxy *toplevel);
	void (*configure_bounds)(void *data, struct wl_proxy *toplevel,
		int32_t width, int32_t height);
	void (*wm_capabilities)(void *data, struct wl_proxy *toplevel,
		struct wl_array *capabilities);
} FB_GFX3_WAYLAND_TOPLEVEL_LISTENER;

static int platform_wayland_copy_symbol(void *destination,
	size_t destination_size, void *symbol)
{
	if ((destination == NULL) || (symbol == NULL) ||
	    (destination_size != sizeof(symbol)))
		return -1;
	memcpy(destination, &symbol, sizeof(symbol));
	return 0;
}

static int platform_wayland_registry_bind(void *state, uint32_t name,
	const char *interface_name, struct wl_proxy **destination,
	uint32_t maximum_version)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)state;
	struct wl_interface *interface = NULL;
	uint32_t version;

	if ((platform == NULL) || (interface_name == NULL) ||
	    (destination == NULL) || (name == 0u) ||
	    (maximum_version == 0u))
		return FB_GFX3_INVALID;
	if (strcmp(interface_name, "wl_compositor") == 0)
		interface = platform->compositor_interface;
	else if (strcmp(interface_name, "wl_seat") == 0)
		interface = platform->seat_interface;
	else if (strcmp(interface_name, "wl_output") == 0)
		interface = platform->output_interface;
	else if (strcmp(interface_name, "xdg_wm_base") == 0)
		interface = (struct wl_interface *)&xdg_wm_base_interface;
	else if (strcmp(interface_name, "zwp_text_input_manager_v3") == 0)
		interface = (struct wl_interface *)&zwp_text_input_manager_v3_interface;
	else if (strcmp(interface_name, "zwp_text_input_manager_v2") == 0)
		interface = (struct wl_interface *)&zwp_text_input_manager_v2_interface;
	if (interface == NULL)
		return FB_GFX3_UNSUPPORTED;
	if (interface->version < 1)
		return FB_GFX3_UNSUPPORTED;
	version = ((uint32_t)interface->version < maximum_version) ?
		(uint32_t)interface->version : maximum_version;
	*destination = platform->api->proxy_marshal_constructor_versioned(
		platform->registry, 0u, interface, version, name, interface->name,
		version, NULL);
	return (*destination != NULL) ? FB_GFX3_OK : FB_GFX3_FAILED;
}

static void platform_wayland_registry_global(void *data,
	struct wl_proxy *registry, uint32_t name, const char *interface,
	uint32_t version)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)registry;
	if ((platform == NULL) || (interface == NULL))
		return;
	if ((strcmp(interface, "wl_compositor") == 0) &&
	    (platform->global_compositor == 0u)) {
		platform->global_compositor = name;
		platform->compositor_version = version;
	} else if ((strcmp(interface, "xdg_wm_base") == 0) &&
	    (platform->global_wm_base == 0u)) {
		platform->global_wm_base = name;
		platform->wm_base_version = version;
	} else if ((strcmp(interface, "wl_seat") == 0) &&
	    (platform->global_seat == 0u)) {
		platform->global_seat = name;
		platform->seat_version = version;
	} else if ((strcmp(interface, "zwp_text_input_manager_v3") == 0) &&
	    (platform->global_text_input_manager == 0u)) {
		platform->global_text_input_manager = name;
	} else if ((strcmp(interface, "zwp_text_input_manager_v2") == 0) &&
	    (platform->global_text_input_manager_v2 == 0u)) {
		platform->global_text_input_manager_v2 = name;
	}
}

static void platform_wayland_registry_global_remove(void *data,
	struct wl_proxy *registry, uint32_t name)
{
	(void)data;
	(void)registry;
	(void)name;
}

static int platform_wayland_evdev_scancode(uint32_t key)
{
	if ((key >= 2u) && (key <= 11u)) return (int)(SC_1 + key - 2u);
	if ((key >= 16u) && (key <= 25u)) return (int)(SC_Q + key - 16u);
	if ((key >= 30u) && (key <= 38u)) return (int)(SC_A + key - 30u);
	if ((key >= 44u) && (key <= 50u)) return (int)(SC_Z + key - 44u);
	if ((key >= 59u) && (key <= 68u)) return (int)(SC_F1 + key - 59u);
	switch (key) {
	case 1: return SC_ESCAPE;
	case 12: return SC_MINUS;
	case 13: return SC_EQUALS;
	case 14: return SC_BACKSPACE;
	case 15: return SC_TAB;
	case 26: return SC_LEFTBRACKET;
	case 27: return SC_RIGHTBRACKET;
	case 28: return SC_ENTER;
	case 29: return SC_CONTROL;
	case 39: return SC_SEMICOLON;
	case 40: return SC_QUOTE;
	case 41: return SC_TILDE;
	case 42: return SC_LSHIFT;
	case 43: return SC_BACKSLASH;
	case 51: return SC_COMMA;
	case 52: return SC_PERIOD;
	case 53: return SC_SLASH;
	case 54: return SC_RSHIFT;
	case 55: return SC_MULTIPLY;
	case 56: return SC_ALT;
	case 57: return SC_SPACE;
	case 58: return SC_CAPSLOCK;
	case 69: return SC_NUMLOCK;
	case 70: return SC_SCROLLLOCK;
	case 71: case 102: return SC_HOME;
	case 72: case 103: return SC_UP;
	case 73: case 104: return SC_PAGEUP;
	case 75: case 105: return SC_LEFT;
	case 77: case 106: return SC_RIGHT;
	case 79: case 107: return SC_END;
	case 80: case 108: return SC_DOWN;
	case 81: case 109: return SC_PAGEDOWN;
	case 82: case 110: return SC_INSERT;
	case 83: case 111: return SC_DELETE;
	case 87: return SC_F11;
	case 88: return SC_F12;
	case 96: return SC_ENTER;
	case 97: return SC_CONTROL;
	case 100: return SC_ALTGR;
	case 125: return SC_LWIN;
	case 126: return SC_RWIN;
	case 127: return SC_MENU;
	default: return 0;
	}
}

static uint32_t platform_wayland_first_codepoint(const char *text,
	size_t available)
{
	const unsigned char *p = (const unsigned char *)text;
	if ((p == NULL) || (available == 0u) || (p[0] == 0)) return 0;
	if (p[0] < 0x80) return p[0];
	if ((available >= 2u) && (p[0] >= 0xC2) && (p[0] <= 0xDF) &&
	    ((p[1] & 0xC0) == 0x80))
		return ((uint32_t)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
	if ((available >= 3u) && (p[0] >= 0xE0) && (p[0] <= 0xEF) &&
	    ((p[1] & 0xC0) == 0x80) && ((p[2] & 0xC0) == 0x80) &&
	    !((p[0] == 0xE0) && (p[1] < 0xA0)) &&
	    !((p[0] == 0xED) && (p[1] >= 0xA0)))
		return ((uint32_t)(p[0] & 0x0F) << 12) |
			((uint32_t)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
	if ((available >= 4u) && (p[0] >= 0xF0) && (p[0] <= 0xF4) &&
	    ((p[1] & 0xC0) == 0x80) && ((p[2] & 0xC0) == 0x80) &&
	    ((p[3] & 0xC0) == 0x80) &&
	    !((p[0] == 0xF0) && (p[1] < 0x90)) &&
	    !((p[0] == 0xF4) && (p[1] >= 0x90)))
		return ((uint32_t)(p[0] & 7) << 18) |
			((uint32_t)(p[1] & 0x3F) << 12) |
			((uint32_t)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
	return 0;
}

static uint64_t platform_wayland_time_ms(void)
{
	struct timespec now;
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
		return 0;
	return (uint64_t)now.tv_sec * 1000u + (uint64_t)now.tv_nsec / 1000000u;
}

static int platform_wayland_xkb_load(FB_GFX3_PLATFORM_WAYLAND *platform)
{
	void *symbol;
	if (platform->xkb.library != NULL)
		return 0;
	platform->xkb.library = dlopen("libxkbcommon.so.0", RTLD_NOW | RTLD_LOCAL);
	if (platform->xkb.library == NULL)
		return -1;
#define FB_GFX3_WAYLAND_LOAD_XKB(member, symbol_name) \
	do { \
		symbol = dlsym(platform->xkb.library, symbol_name); \
		if (platform_wayland_copy_symbol(&platform->xkb.member, \
			sizeof(platform->xkb.member), symbol) != 0) goto failed; \
	} while (0)
	FB_GFX3_WAYLAND_LOAD_XKB(context_new, "xkb_context_new");
	FB_GFX3_WAYLAND_LOAD_XKB(context_unref, "xkb_context_unref");
	FB_GFX3_WAYLAND_LOAD_XKB(keymap_new_from_string, "xkb_keymap_new_from_string");
	FB_GFX3_WAYLAND_LOAD_XKB(keymap_unref, "xkb_keymap_unref");
	FB_GFX3_WAYLAND_LOAD_XKB(state_new, "xkb_state_new");
	FB_GFX3_WAYLAND_LOAD_XKB(state_unref, "xkb_state_unref");
	FB_GFX3_WAYLAND_LOAD_XKB(state_update_key, "xkb_state_update_key");
	FB_GFX3_WAYLAND_LOAD_XKB(state_key_get_utf8, "xkb_state_key_get_utf8");
	symbol = dlsym(platform->xkb.library, "xkb_state_update_mask");
	(void)platform_wayland_copy_symbol(&platform->xkb.state_update_mask,
		sizeof(platform->xkb.state_update_mask), symbol);
	symbol = dlsym(platform->xkb.library, "xkb_state_key_get_one_sym");
	(void)platform_wayland_copy_symbol(&platform->xkb.state_key_get_one_sym,
		sizeof(platform->xkb.state_key_get_one_sym), symbol);
#undef FB_GFX3_WAYLAND_LOAD_XKB
	return 0;

failed:
#undef FB_GFX3_WAYLAND_LOAD_XKB
	dlclose(platform->xkb.library);
	memset(&platform->xkb, 0, sizeof(platform->xkb));
	return -1;
}

static void platform_wayland_xkb_clear(FB_GFX3_PLATFORM_WAYLAND *platform)
{
	if (platform->xkb_state != NULL)
		platform->xkb.state_unref(platform->xkb_state);
	if (platform->xkb_keymap != NULL)
		platform->xkb.keymap_unref(platform->xkb_keymap);
	if (platform->xkb_context != NULL)
		platform->xkb.context_unref(platform->xkb_context);
	platform->xkb_state = NULL;
	platform->xkb_keymap = NULL;
	platform->xkb_context = NULL;
}

static void platform_wayland_xkb_unload(FB_GFX3_PLATFORM_WAYLAND *platform)
{
	platform_wayland_xkb_clear(platform);
	if (platform->xkb.library != NULL)
		dlclose(platform->xkb.library);
	memset(&platform->xkb, 0, sizeof(platform->xkb));
}

static void platform_wayland_keyboard_keymap_cb(void *data,
	struct wl_proxy *keyboard, uint32_t format, int32_t fd, uint32_t size)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	char *text;
	void *mapping;
	(void)keyboard;
	if ((fd < 0) || (format != FB_GFX3_WL_KEYMAP_FORMAT_TEXT_V1) ||
	    (size == 0u) || (size > FB_GFX3_WL_KEYMAP_MAX_BYTES)) {
		if (fd >= 0) close(fd);
		return;
	}
	if (platform_wayland_xkb_load(platform) != 0) {
		close(fd);
		return;
	}
	mapping = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (mapping == MAP_FAILED)
		return;
	text = (char *)malloc((size_t)size + 1u);
	if (text == NULL) {
		munmap(mapping, size);
		return;
	}
	memcpy(text, mapping, size);
	text[size] = '\0';
	munmap(mapping, size);
	platform_wayland_xkb_clear(platform);
	platform->xkb_context = platform->xkb.context_new(0);
	if (platform->xkb_context != NULL)
		platform->xkb_keymap = platform->xkb.keymap_new_from_string(
			platform->xkb_context, text, 1, 0);
	free(text);
	if (platform->xkb_keymap != NULL)
		platform->xkb_state = platform->xkb.state_new(platform->xkb_keymap);
}

static void platform_wayland_emit_text(FB_GFX3_PLATFORM_WAYLAND *platform,
	const char *text)
{
	size_t position = 0;
	size_t bytes;
	uint32_t emitted = 0;
	if ((platform->input == NULL) || (text == NULL))
		return;
	bytes = strnlen(text, 4096u);
	while ((position < bytes) &&
	       (emitted < FB_GFX3_INPUT_KEY_BUFFER_LENGTH - 1u)) {
		uint32_t codepoint = platform_wayland_first_codepoint(text + position,
			bytes - position);
		unsigned char lead = (unsigned char)text[position];
		size_t length = (lead < 0x80) ? 1u :
			((lead < 0xE0) ? 2u : ((lead < 0xF0) ? 3u : 4u));
		if ((codepoint == 0u) || (position + length > bytes))
			break;
		fb_gfx3_input_platform_text(platform->input, codepoint);
		position += length;
		emitted++;
	}
}

static void platform_wayland_apply_keyboard(FB_GFX3_PLATFORM_WAYLAND *platform)
{
	if (platform->text_input != NULL) {
		if (!platform->text_input_entered)
			return;
		/* Enabling or disabling v3 invalidates its uncommitted edit. */
		fb_hWaylandTextV3Reset(&platform->text_v3_pending);
		if (platform->keyboard_requested) {
			platform->api->proxy_marshal(platform->text_input,
				FB_GFX3_WL_TEXT_INPUT_ENABLE);
			platform->api->proxy_marshal(platform->text_input,
				FB_GFX3_WL_TEXT_INPUT_SET_CONTENT_TYPE, 0u, 0u);
			platform->api->proxy_marshal(platform->text_input,
				FB_GFX3_WL_TEXT_INPUT_SET_CURSOR_RECTANGLE,
				platform->region_x, platform->region_y,
				platform->region_width, platform->region_height);
			platform->keyboard_visible = -2;
		} else {
			platform->api->proxy_marshal(platform->text_input,
				FB_GFX3_WL_TEXT_INPUT_DISABLE);
			platform->keyboard_visible = 0;
		}
		platform->api->proxy_marshal(platform->text_input,
			FB_GFX3_WL_TEXT_INPUT_COMMIT);
	} else if (platform->text_input_v2 != NULL) {
		if (!platform->text_input_entered)
			return;
		if (platform->keyboard_requested) {
			platform->api->proxy_marshal(platform->text_input_v2,
				FB_GFX3_WL_TEXT_INPUT_V2_SET_CONTENT_TYPE, 0u, 0u);
			platform->api->proxy_marshal(platform->text_input_v2,
				FB_GFX3_WL_TEXT_INPUT_V2_SET_CURSOR_RECTANGLE,
				platform->region_x, platform->region_y,
				platform->region_width, platform->region_height);
			platform->api->proxy_marshal(platform->text_input_v2,
				FB_GFX3_WL_TEXT_INPUT_V2_ENABLE, platform->surface);
			platform->api->proxy_marshal(platform->text_input_v2,
				FB_GFX3_WL_TEXT_INPUT_V2_SHOW_PANEL);
			platform->api->proxy_marshal(platform->text_input_v2,
				FB_GFX3_WL_TEXT_INPUT_V2_UPDATE_STATE,
				platform->text_input_serial,
				FB_GFX3_WL_TEXT_INPUT_V2_UPDATE_STATE_ENTER);
			platform->keyboard_visible = -2;
		} else {
			platform->api->proxy_marshal(platform->text_input_v2,
				FB_GFX3_WL_TEXT_INPUT_V2_HIDE_PANEL);
			platform->api->proxy_marshal(platform->text_input_v2,
				FB_GFX3_WL_TEXT_INPUT_V2_DISABLE, platform->surface);
			platform->keyboard_visible = 0;
		}
	}
	if (platform->api->display_flush != NULL)
		(void)platform->api->display_flush(platform->display);
	if (platform->input != NULL)
		fb_gfx3_input_platform_keyboard_state(platform->input,
			(platform->text_input != NULL) ||
			(platform->text_input_v2 != NULL), platform->keyboard_visible);
}

static void platform_wayland_update_text_region(
	FB_GFX3_PLATFORM_WAYLAND *platform)
{
	if (!platform->keyboard_requested || !platform->text_input_entered)
		return;

	/* A v3 enable resets input state; changing the rectangle must not do so. */
	if (platform->text_input != NULL) {
		platform->api->proxy_marshal(platform->text_input,
			FB_GFX3_WL_TEXT_INPUT_SET_CURSOR_RECTANGLE,
			platform->region_x, platform->region_y,
			platform->region_width, platform->region_height);
		platform->api->proxy_marshal(platform->text_input,
			FB_GFX3_WL_TEXT_INPUT_COMMIT);
	} else if (platform->text_input_v2 != NULL) {
		platform->api->proxy_marshal(platform->text_input_v2,
			FB_GFX3_WL_TEXT_INPUT_V2_SET_CURSOR_RECTANGLE,
			platform->region_x, platform->region_y,
			platform->region_width, platform->region_height);
		platform->api->proxy_marshal(platform->text_input_v2,
			FB_GFX3_WL_TEXT_INPUT_V2_UPDATE_STATE,
			platform->text_input_serial,
			FB_GFX3_WL_TEXT_INPUT_V2_UPDATE_STATE_CHANGE);
	} else {
		return;
	}
	if (platform->api->display_flush != NULL)
		(void)platform->api->display_flush(platform->display);
}

static int platform_wayland_key_repeatable(uint32_t key)
{
	/* Linux evdev modifiers, lock keys and system keys do not repeat. */
	switch (key) {
	case 29u: case 42u: case 54u: case 56u: case 58u:
	case 97u: case 100u: case 125u: case 126u:
		return FALSE;
	default:
		return TRUE;
	}
}

static void platform_wayland_key_event(FB_GFX3_PLATFORM_WAYLAND *platform,
	uint32_t evdev_key, int pressed, int type)
{
	char text[16] = { 0 };
	uint32_t codepoint = 0;
	uint32_t symbol = 0;
	int text_length;
	int scancode;
	int key_value;
	if ((platform->input == NULL) || (evdev_key >= FB_GFX3_WL_EVDEV_KEY_MAX))
		return;
	scancode = platform_wayland_evdev_scancode(evdev_key);
	if (platform->xkb_state != NULL) {
		uint32_t xkb_key = evdev_key + 8u;
		(void)platform->xkb.state_update_key(platform->xkb_state, xkb_key,
			pressed ? 1u : 0u);
		if (pressed) {
			if (platform->xkb.state_key_get_one_sym != NULL)
				symbol = platform->xkb.state_key_get_one_sym(
					platform->xkb_state, xkb_key);
			text_length = platform->xkb.state_key_get_utf8(
				platform->xkb_state, xkb_key, text, sizeof(text));
			if ((text_length > 0) &&
			    ((size_t)text_length < sizeof(text)))
				codepoint = platform_wayland_first_codepoint(text,
					(size_t)text_length);
		}
	}
	if (pressed) {
		scancode = fb_hWaylandControlScancode(symbol, scancode);
		if ((scancode > 0) && (scancode < (int)FB_GFX3_INPUT_KEY_COUNT))
			platform->evdev_scancode[evdev_key] = (unsigned char)scancode;
	} else {
		if (platform->evdev_scancode[evdev_key] != 0)
			scancode = platform->evdev_scancode[evdev_key];
		platform->evdev_scancode[evdev_key] = 0;
	}
	if ((scancode <= 0) || (scancode >= (int)FB_GFX3_INPUT_KEY_COUNT)) {
		/* A virtual keyboard can assign printable XKB text to any evdev code. */
		if (pressed && codepoint) {
			platform_wayland_emit_text(platform, text);
			if ((platform->repeat_interval_ms != 0u) &&
			    platform_wayland_key_repeatable(evdev_key)) {
				platform->repeat_text_only = TRUE;
				platform->repeat_scancode = 0;
				platform->repeat_evdev_key = evdev_key;
				memcpy(platform->repeat_text, text, sizeof(text));
				platform->repeat_deadline_ms = platform_wayland_time_ms() +
					platform->repeat_delay_ms;
			}
		} else if (!pressed && platform->repeat_text_only &&
			   platform->repeat_evdev_key == evdev_key) {
			platform->repeat_text_only = FALSE;
			platform->repeat_deadline_ms = 0u;
		}
		return;
	}
	key_value = codepoint ? (int)codepoint :
		fb_hScancodeToExtendedKey(scancode);
	platform->key_down[scancode] = (unsigned char)(pressed != 0);
	fb_gfx3_input_platform_key(platform->input, type, scancode,
		((key_value >= 0) && (key_value <= 0xFF)) ? key_value : 0);
	if (pressed) {
		/* INKEY reserves values above 255 for extended keys. */
		if (codepoint > 0xFFu)
			fb_gfx3_input_platform_text(platform->input, codepoint);
		else
			fb_gfx3_input_platform_character(platform->input, key_value, 1u);
	}
	if (pressed && (platform->repeat_interval_ms != 0u) &&
	    platform_wayland_key_repeatable(evdev_key)) {
		platform->repeat_text_only = FALSE;
		platform->repeat_scancode = scancode;
		platform->repeat_key = (codepoint > 0xFFu) ? 0 : key_value;
		platform->repeat_unicode = (codepoint > 0xFFu) ? codepoint : 0u;
		platform->repeat_ascii =
			((key_value >= 0) && (key_value <= 0xFF)) ? key_value : 0;
		platform->repeat_deadline_ms = platform_wayland_time_ms() +
			platform->repeat_delay_ms;
	} else if (!pressed && !platform->repeat_text_only &&
		   (platform->repeat_scancode == scancode)) {
		platform->repeat_scancode = 0;
		platform->repeat_deadline_ms = 0u;
	}
}

static void platform_wayland_keyboard_enter(void *data,
	struct wl_proxy *keyboard, uint32_t serial, struct wl_proxy *surface,
	struct wl_array *keys)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)keyboard;
	(void)serial;
	(void)keys;
	platform->keyboard_focused = (surface == platform->surface);
	if (platform->input != NULL)
		fb_gfx3_input_platform_focus(platform->input,
			platform->keyboard_focused);
}

static void platform_wayland_keyboard_leave(void *data,
	struct wl_proxy *keyboard, uint32_t serial, struct wl_proxy *surface)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	unsigned int key;
	(void)keyboard;
	(void)serial;
	if (surface != platform->surface)
		return;
	platform->keyboard_focused = FALSE;
	platform->repeat_scancode = 0;
	platform->repeat_text_only = FALSE;
	platform->repeat_deadline_ms = 0u;
	for (key = 0; key < FB_GFX3_INPUT_KEY_COUNT; key++) {
		if (platform->key_down[key]) {
			platform->key_down[key] = 0;
			fb_gfx3_input_platform_key(platform->input,
				EVENT_KEY_RELEASE, (int)key, 0);
		}
	}
	memset(platform->evdev_scancode, 0, sizeof(platform->evdev_scancode));
	if (platform->input != NULL)
		fb_gfx3_input_platform_focus(platform->input, FALSE);
}

static void platform_wayland_keyboard_key(void *data,
	struct wl_proxy *keyboard, uint32_t serial, uint32_t time,
	uint32_t key, uint32_t state)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)keyboard;
	(void)serial;
	(void)time;
	if (!platform->keyboard_focused)
		return;
	platform_wayland_key_event(platform, key, state != 0u,
		(state != 0u) ? EVENT_KEY_PRESS : EVENT_KEY_RELEASE);
}

static void platform_wayland_keyboard_modifiers(void *data,
	struct wl_proxy *keyboard, uint32_t serial, uint32_t depressed,
	uint32_t latched, uint32_t locked, uint32_t group)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)keyboard;
	(void)serial;
	if ((platform->xkb_state != NULL) &&
	    (platform->xkb.state_update_mask != NULL))
		(void)platform->xkb.state_update_mask(platform->xkb_state,
			depressed, latched, locked, 0u, 0u, group);
}

static void platform_wayland_keyboard_repeat_info(void *data,
	struct wl_proxy *keyboard, int32_t rate, int32_t delay)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)keyboard;
	if ((rate <= 0) || (delay < 0)) {
		platform->repeat_interval_ms = 0u;
		platform->repeat_scancode = 0;
		platform->repeat_text_only = FALSE;
		platform->repeat_deadline_ms = 0u;
		return;
	}
	platform->repeat_delay_ms = (uint32_t)delay;
	platform->repeat_interval_ms = (uint32_t)(1000 / rate);
	if (platform->repeat_interval_ms == 0u)
		platform->repeat_interval_ms = 1u;
}

static const FB_GFX3_WAYLAND_KEYBOARD_LISTENER
	platform_wayland_keyboard_listener = {
	platform_wayland_keyboard_keymap_cb,
	platform_wayland_keyboard_enter,
	platform_wayland_keyboard_leave,
	platform_wayland_keyboard_key,
	platform_wayland_keyboard_modifiers,
	platform_wayland_keyboard_repeat_info
};

static void platform_wayland_mouse_position(FB_GFX3_PLATFORM_WAYLAND *platform,
	int32_t fixed_x, int32_t fixed_y)
{
	int x = fixed_x / 256;
	int y = fixed_y / 256;
	if (platform->view_width > 0u) {
		if (x < 0) x = 0;
		if ((uint32_t)x >= platform->view_width)
			x = (int)platform->view_width - 1;
	}
	if (platform->view_height > 0u) {
		if (y < 0) y = 0;
		if ((uint32_t)y >= platform->view_height)
			y = (int)platform->view_height - 1;
	}
	fb_gfx3_input_platform_mouse_move(platform->input, x, y);
}

static int platform_wayland_mouse_button(uint32_t native_button)
{
	switch (native_button) {
	case 0x110u: return BUTTON_LEFT;
	case 0x111u: return BUTTON_RIGHT;
	case 0x112u: return BUTTON_MIDDLE;
	default: return 0;
	}
}

static void platform_wayland_pointer_enter(void *data,
	struct wl_proxy *pointer, uint32_t serial, struct wl_proxy *surface,
	int32_t fixed_x, int32_t fixed_y)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)pointer;
	(void)serial;
	if ((platform->input == NULL) || (surface != platform->surface))
		return;
	platform->pointer_focused = TRUE;
	fb_gfx3_input_platform_mouse_enter(platform->input);
	platform_wayland_mouse_position(platform, fixed_x, fixed_y);
}

static void platform_wayland_pointer_leave(void *data,
	struct wl_proxy *pointer, uint32_t serial, struct wl_proxy *surface)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	static const int buttons[] = { BUTTON_LEFT, BUTTON_RIGHT, BUTTON_MIDDLE };
	size_t index;
	(void)pointer;
	(void)serial;
	if ((platform->input == NULL) || (surface != platform->surface))
		return;
	for (index = 0; index < sizeof(buttons) / sizeof(buttons[0]); index++) {
		if (platform->pointer_buttons & buttons[index])
			fb_gfx3_input_platform_mouse_button(platform->input,
				buttons[index], FALSE, FALSE);
	}
	platform->pointer_buttons = 0;
	platform->pointer_focused = FALSE;
	fb_gfx3_input_platform_mouse_exit(platform->input);
}

static void platform_wayland_pointer_motion(void *data,
	struct wl_proxy *pointer, uint32_t time, int32_t fixed_x,
	int32_t fixed_y)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)pointer;
	(void)time;
	if (platform->pointer_focused)
		platform_wayland_mouse_position(platform, fixed_x, fixed_y);
}

static void platform_wayland_pointer_button(void *data,
	struct wl_proxy *pointer, uint32_t serial, uint32_t time,
	uint32_t native_button, uint32_t state)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	int button = platform_wayland_mouse_button(native_button);
	int pressed = (state != 0u);
	int double_click = FALSE;
	(void)pointer;
	(void)serial;
	if ((platform->input == NULL) || !platform->pointer_focused || !button)
		return;
	if (pressed) {
		double_click = (platform->last_click_button == button) &&
			((uint32_t)(time - platform->last_click_time) < 250u);
		platform->last_click_button = button;
		platform->last_click_time = time;
		platform->pointer_buttons |= button;
	} else {
		platform->pointer_buttons &= ~button;
	}
	fb_gfx3_input_platform_mouse_button(platform->input, button,
		pressed, double_click);
}

static void platform_wayland_pointer_axis(void *data,
	struct wl_proxy *pointer, uint32_t time, uint32_t axis, int32_t value)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)pointer;
	(void)time;
	if ((platform->input != NULL) && (value != 0))
		fb_gfx3_input_platform_mouse_wheel(platform->input, axis == 1u,
			(value < 0) ? 1 : -1);
}

static const FB_GFX3_WAYLAND_POINTER_LISTENER
	platform_wayland_pointer_listener = {
	platform_wayland_pointer_enter,
	platform_wayland_pointer_leave,
	platform_wayland_pointer_motion,
	platform_wayland_pointer_button,
	platform_wayland_pointer_axis
};

static int platform_wayland_touch_find(FB_GFX3_PLATFORM_WAYLAND *platform,
	int id)
{
	uint32_t index;
	for (index = 0; index < platform->touch_count; index++) {
		if (platform->touch_contacts[index].id == id)
			return (int)index;
	}
	return -1;
}

static void platform_wayland_touch_publish(FB_GFX3_PLATFORM_WAYLAND *platform)
{
	int primary_index;
	if (platform->input == NULL)
		return;
	(void)fb_gfx3_input_platform_touch_replace(platform->input,
		platform->touch_contacts, platform->touch_count);
	if (!platform->have_primary_touch && (platform->touch_count > 0u)) {
		platform->primary_touch_id = platform->touch_contacts[0].id;
		platform->have_primary_touch = TRUE;
		fb_gfx3_input_platform_mouse_enter(platform->input);
		/* SCREENEVENT button payloads do not contain coordinates. Publish
		 * the first contact's position before its button edge. */
		fb_gfx3_input_platform_mouse_move(platform->input,
			platform->touch_contacts[0].x, platform->touch_contacts[0].y);
		fb_gfx3_input_platform_mouse_button(platform->input,
			BUTTON_LEFT, TRUE, FALSE);
	}
	primary_index = platform->have_primary_touch ?
		platform_wayland_touch_find(platform, platform->primary_touch_id) : -1;
	if (primary_index >= 0) {
		fb_gfx3_input_platform_mouse_move(platform->input,
			platform->touch_contacts[primary_index].x,
			platform->touch_contacts[primary_index].y);
	} else if (platform->have_primary_touch) {
		fb_gfx3_input_platform_mouse_button(platform->input,
			BUTTON_LEFT, FALSE, FALSE);
		platform->have_primary_touch = FALSE;
		if (platform->touch_count > 0u)
			platform_wayland_touch_publish(platform);
	}
}

static void platform_wayland_touch_down(void *data, struct wl_proxy *touch,
	uint32_t serial, uint32_t time, struct wl_proxy *surface, int32_t id,
	int32_t fixed_x, int32_t fixed_y)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	int index;
	(void)touch;
	(void)serial;
	(void)time;
	if ((surface != platform->surface) || (platform->input == NULL))
		return;
	index = platform_wayland_touch_find(platform, id);
	if ((index < 0) && (platform->touch_count < FB_GFX3_INPUT_TOUCH_MAX))
		index = (int)platform->touch_count++;
	if (index < 0)
		return;
	platform->touch_contacts[index].id = id;
	platform->touch_contacts[index].x = fixed_x / 256;
	platform->touch_contacts[index].y = fixed_y / 256;
	platform_wayland_touch_publish(platform);
}

static void platform_wayland_touch_up(void *data, struct wl_proxy *touch,
	uint32_t serial, uint32_t time, int32_t id)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	int index;
	(void)touch;
	(void)serial;
	(void)time;
	index = platform_wayland_touch_find(platform, id);
	if (index < 0)
		return;
	platform->touch_count--;
	if ((uint32_t)index < platform->touch_count)
		platform->touch_contacts[index] = platform->touch_contacts[platform->touch_count];
	platform_wayland_touch_publish(platform);
}

static void platform_wayland_touch_motion(void *data, struct wl_proxy *touch,
	uint32_t time, int32_t id, int32_t fixed_x, int32_t fixed_y)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	int index = platform_wayland_touch_find(platform, id);
	(void)touch;
	(void)time;
	if (index < 0)
		return;
	platform->touch_contacts[index].x = fixed_x / 256;
	platform->touch_contacts[index].y = fixed_y / 256;
	platform_wayland_touch_publish(platform);
}

static void platform_wayland_touch_frame(void *data, struct wl_proxy *touch)
{
	(void)data;
	(void)touch;
}

static void platform_wayland_touch_cancel(void *data, struct wl_proxy *touch)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)touch;
	platform->touch_count = 0u;
	if (platform->input == NULL)
		return;
	(void)fb_gfx3_input_platform_touch_replace(platform->input, NULL, 0u);
	if (platform->have_primary_touch) {
		fb_gfx3_input_platform_mouse_button(platform->input,
			BUTTON_LEFT, FALSE, FALSE);
		fb_gfx3_input_platform_mouse_exit(platform->input);
		platform->have_primary_touch = FALSE;
	}
}

static const FB_GFX3_WAYLAND_TOUCH_LISTENER
	platform_wayland_touch_listener = {
	platform_wayland_touch_down,
	platform_wayland_touch_up,
	platform_wayland_touch_motion,
	platform_wayland_touch_frame,
	platform_wayland_touch_cancel
};

static void platform_wayland_text_v3_enter(void *data,
	struct wl_proxy *text_input, struct wl_proxy *surface)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)text_input;
	if (surface != platform->surface)
		return;
	platform->text_input_entered = TRUE;
	if (platform->keyboard_requested) {
		/* Reset stale input-method state before restoring surface focus. */
		platform->keyboard_requested = FALSE;
		platform_wayland_apply_keyboard(platform);
		platform->keyboard_requested = TRUE;
		platform_wayland_apply_keyboard(platform);
	}
}

static void platform_wayland_text_v3_leave(void *data,
	struct wl_proxy *text_input, struct wl_proxy *surface)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)text_input;
	if (surface == platform->surface) {
		fb_hWaylandTextV3Reset(&platform->text_v3_pending);
		platform->text_input_entered = FALSE;
		platform->keyboard_visible = 0;
		if (platform->input != NULL)
			fb_gfx3_input_platform_keyboard_state(platform->input, TRUE, 0);
	}
}

static void platform_wayland_text_v3_preedit(void *data,
	struct wl_proxy *text_input, const char *text, int32_t begin, int32_t end)
{
	(void)data;
	(void)text_input;
	(void)text;
	(void)begin;
	(void)end;
}

static void platform_wayland_text_v3_commit(void *data,
	struct wl_proxy *text_input, const char *text)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)text_input;
	if (platform->text_input_entered && platform->keyboard_requested)
		fb_hWaylandTextV3SetCommit(&platform->text_v3_pending, text);
}

static void platform_wayland_text_v3_delete(void *data,
	struct wl_proxy *text_input, uint32_t before, uint32_t after)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)text_input;
	if (platform->text_input_entered && platform->keyboard_requested)
		fb_hWaylandTextV3SetDelete(&platform->text_v3_pending, before, after);
}

static void platform_wayland_text_v3_done(void *data,
	struct wl_proxy *text_input, uint32_t serial)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	uint32_t count = platform->text_v3_pending.before;
	(void)text_input;
	(void)serial;
	/*
	 * v3 applies deletion before insertion regardless of event order. The
	 * legacy input API has no surrounding text, so the existing before-cursor
	 * backspace fallback cannot exactly represent UTF-8 or after-cursor edits.
	 */
	if (platform->text_input_entered && platform->keyboard_requested) {
		if (count > FB_GFX3_INPUT_KEY_BUFFER_LENGTH - 1u)
			count = FB_GFX3_INPUT_KEY_BUFFER_LENGTH - 1u;
		while ((count > 0u) && (platform->input != NULL)) {
			fb_gfx3_input_platform_text(platform->input, '\b');
			count--;
		}
		platform_wayland_emit_text(platform, platform->text_v3_pending.text);
	}
	fb_hWaylandTextV3Reset(&platform->text_v3_pending);
}

static const FB_GFX3_WAYLAND_TEXT_INPUT_V3_LISTENER
	platform_wayland_text_v3_listener = {
	platform_wayland_text_v3_enter,
	platform_wayland_text_v3_leave,
	platform_wayland_text_v3_preedit,
	platform_wayland_text_v3_commit,
	platform_wayland_text_v3_delete,
	platform_wayland_text_v3_done
};

static void platform_wayland_text_v2_enter(void *data,
	struct wl_proxy *text_input, uint32_t serial, struct wl_proxy *surface)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)text_input;
	if (surface != platform->surface)
		return;
	platform->text_input_entered = TRUE;
	platform->text_input_serial = serial;
	if (platform->keyboard_requested)
		platform_wayland_apply_keyboard(platform);
}

static void platform_wayland_text_v2_leave(void *data,
	struct wl_proxy *text_input, uint32_t serial, struct wl_proxy *surface)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)text_input;
	(void)serial;
	if (surface == platform->surface) {
		platform->text_input_entered = FALSE;
		platform->keyboard_visible = 0;
		if (platform->input != NULL)
			fb_gfx3_input_platform_keyboard_state(platform->input, TRUE, 0);
	}
}

static void platform_wayland_text_v2_panel(void *data,
	struct wl_proxy *text_input, uint32_t state, int32_t x, int32_t y,
	int32_t width, int32_t height)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)text_input;
	(void)x;
	(void)y;
	(void)width;
	(void)height;
	platform->keyboard_visible = state ? -1 : 0;
	if (platform->input != NULL)
		fb_gfx3_input_platform_keyboard_state(platform->input, TRUE,
			platform->keyboard_visible);
}

static void platform_wayland_text_v2_preedit(void *data,
	struct wl_proxy *text_input, const char *text, const char *commit)
{
	/* The v2 fallback commit is not a commit_string event. This backend
	 * does not display preedit, so wait for the actual committed text. */
	(void)text_input;
	(void)text;
	(void)data;
	(void)commit;
}

static void platform_wayland_text_v2_preedit_styling(void *data,
	struct wl_proxy *text_input, uint32_t index, uint32_t length,
	uint32_t style)
{
	(void)data;
	(void)text_input;
	(void)index;
	(void)length;
	(void)style;
}

static void platform_wayland_text_v2_preedit_cursor(void *data,
	struct wl_proxy *text_input, int32_t index)
{
	(void)data;
	(void)text_input;
	(void)index;
}

static void platform_wayland_text_v2_commit(void *data,
	struct wl_proxy *text_input, const char *text)
{
	(void)text_input;
	platform_wayland_emit_text((FB_GFX3_PLATFORM_WAYLAND *)data, text);
}

static void platform_wayland_text_v2_cursor(void *data,
	struct wl_proxy *text_input, int32_t index, int32_t anchor)
{
	(void)data;
	(void)text_input;
	(void)index;
	(void)anchor;
}

static void platform_wayland_text_v2_delete(void *data,
	struct wl_proxy *text_input, uint32_t before, uint32_t after)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)text_input;
	(void)after;
	if (before > FB_GFX3_INPUT_KEY_BUFFER_LENGTH - 1u)
		before = FB_GFX3_INPUT_KEY_BUFFER_LENGTH - 1u;
	while ((before > 0u) && (platform->input != NULL)) {
		fb_gfx3_input_platform_text(platform->input, '\b');
		before--;
	}
}

static void platform_wayland_text_v2_modifiers(void *data,
	struct wl_proxy *text_input, struct wl_array *map)
{
	(void)data;
	(void)text_input;
	(void)map;
}

static void platform_wayland_text_v2_keysym(void *data,
	struct wl_proxy *text_input, uint32_t time, uint32_t sym,
	uint32_t state, uint32_t modifiers)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	int key = 0;
	(void)text_input;
	(void)time;
	(void)modifiers;
	if ((state == 0u) || (platform->input == NULL))
		return;
	if ((sym >= 0x20u) && (sym <= 0xFFu))
		key = (int)sym;
	else if (sym == 0xFF0Du)
		key = 13;
	else if (sym == 0xFF08u)
		key = 8;
	if (key > 0)
		fb_gfx3_input_platform_character(platform->input, key, 1u);
}

static void platform_wayland_text_v2_language(void *data,
	struct wl_proxy *text_input, const char *language)
{
	(void)data;
	(void)text_input;
	(void)language;
}

static void platform_wayland_text_v2_direction(void *data,
	struct wl_proxy *text_input, uint32_t direction)
{
	(void)data;
	(void)text_input;
	(void)direction;
}

static void platform_wayland_text_v2_surrounding(void *data,
	struct wl_proxy *text_input, int32_t before, int32_t after)
{
	(void)data;
	(void)text_input;
	(void)before;
	(void)after;
}

static void platform_wayland_text_v2_method_changed(void *data,
	struct wl_proxy *text_input, uint32_t serial, uint32_t flags)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)text_input;
	(void)flags;
	platform->text_input_serial = serial;
	if (platform->keyboard_requested)
		platform_wayland_apply_keyboard(platform);
}

static const FB_GFX3_WAYLAND_TEXT_INPUT_V2_LISTENER
	platform_wayland_text_v2_listener = {
	platform_wayland_text_v2_enter,
	platform_wayland_text_v2_leave,
	platform_wayland_text_v2_panel,
	platform_wayland_text_v2_preedit,
	platform_wayland_text_v2_preedit_styling,
	platform_wayland_text_v2_preedit_cursor,
	platform_wayland_text_v2_commit,
	platform_wayland_text_v2_cursor,
	platform_wayland_text_v2_delete,
	platform_wayland_text_v2_modifiers,
	platform_wayland_text_v2_keysym,
	platform_wayland_text_v2_language,
	platform_wayland_text_v2_direction,
	platform_wayland_text_v2_surrounding,
	platform_wayland_text_v2_method_changed
};

static void platform_wayland_seat_name(void *data,
	struct wl_proxy *seat, const char *name)
{
	(void)data;
	(void)seat;
	(void)name;
}

static void platform_wayland_proxy_release(FB_GFX3_PLATFORM_WAYLAND *platform,
	struct wl_proxy **proxy, uint32_t release_opcode,
	uint32_t release_since)
{
	if ((platform == NULL) || (proxy == NULL) || (*proxy == NULL) ||
	    (platform->api == NULL))
		return;
	if ((platform->api->proxy_get_version != NULL) &&
	    (platform->api->proxy_get_version(*proxy) >= release_since))
		platform->api->proxy_marshal(*proxy, release_opcode);
	platform->api->proxy_destroy(*proxy);
	*proxy = NULL;
}

static void platform_wayland_destroy_object(FB_GFX3_PLATFORM_WAYLAND *platform,
	struct wl_proxy **proxy, uint32_t destroy_opcode)
{
	if ((platform == NULL) || (proxy == NULL) || (*proxy == NULL) ||
	    (platform->api == NULL))
		return;
	platform->api->proxy_marshal(*proxy, destroy_opcode);
	platform->api->proxy_destroy(*proxy);
	*proxy = NULL;
}

static void platform_wayland_input_shutdown(
	FB_GFX3_PLATFORM_WAYLAND *platform)
{
	if ((platform == NULL) || (platform->api == NULL))
		return;
	platform_wayland_destroy_object(platform, &platform->text_input, 0u);
	platform_wayland_destroy_object(platform, &platform->text_input_v2, 0u);
	platform_wayland_destroy_object(platform,
		&platform->text_input_manager, 0u);
	platform_wayland_destroy_object(platform,
		&platform->text_input_manager_v2, 0u);
	platform_wayland_proxy_release(platform, &platform->touch,
		FB_GFX3_WL_TOUCH_RELEASE, 3u);
	platform_wayland_proxy_release(platform, &platform->keyboard,
		FB_GFX3_WL_KEYBOARD_RELEASE, 3u);
	platform_wayland_proxy_release(platform, &platform->pointer,
		FB_GFX3_WL_POINTER_RELEASE, 3u);
	platform_wayland_proxy_release(platform, &platform->seat,
		FB_GFX3_WL_SEAT_RELEASE, 5u);
	platform_wayland_xkb_unload(platform);
}

static void platform_wayland_seat_capabilities(void *data,
	struct wl_proxy *seat, uint32_t capabilities)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	uint32_t version;
	if ((platform == NULL) || (platform->api == NULL))
		return;
	version = platform->api->proxy_get_version(seat);
	if (version > 4u)
		version = 4u;
	if ((capabilities & FB_GFX3_WL_SEAT_CAPABILITY_POINTER) &&
	    (platform->pointer == NULL)) {
		platform->pointer = platform->api->proxy_marshal_constructor_versioned(
			seat, FB_GFX3_WL_SEAT_GET_POINTER, platform->pointer_interface,
			version, NULL);
		if ((platform->pointer == NULL) ||
		    (platform->api->proxy_add_listener(platform->pointer,
				(void (**)(void))&platform_wayland_pointer_listener,
				platform) != 0))
			platform_wayland_proxy_release(platform, &platform->pointer,
				FB_GFX3_WL_POINTER_RELEASE, 3u);
	} else if (!(capabilities & FB_GFX3_WL_SEAT_CAPABILITY_POINTER)) {
		platform_wayland_pointer_leave(platform, NULL, 0u, platform->surface);
		platform_wayland_proxy_release(platform, &platform->pointer,
			FB_GFX3_WL_POINTER_RELEASE, 3u);
	}
	if ((capabilities & FB_GFX3_WL_SEAT_CAPABILITY_KEYBOARD) &&
	    (platform->keyboard == NULL)) {
		platform->keyboard = platform->api->proxy_marshal_constructor_versioned(
			seat, FB_GFX3_WL_SEAT_GET_KEYBOARD, platform->keyboard_interface,
			version, NULL);
		if ((platform->keyboard == NULL) ||
		    (platform->api->proxy_add_listener(platform->keyboard,
				(void (**)(void))&platform_wayland_keyboard_listener,
				platform) != 0))
			platform_wayland_proxy_release(platform, &platform->keyboard,
				FB_GFX3_WL_KEYBOARD_RELEASE, 3u);
	} else if (!(capabilities & FB_GFX3_WL_SEAT_CAPABILITY_KEYBOARD)) {
		platform_wayland_keyboard_leave(platform, NULL, 0u, platform->surface);
		platform_wayland_proxy_release(platform, &platform->keyboard,
			FB_GFX3_WL_KEYBOARD_RELEASE, 3u);
	}
	if ((capabilities & FB_GFX3_WL_SEAT_CAPABILITY_TOUCH) &&
	    (platform->touch == NULL)) {
		platform->touch = platform->api->proxy_marshal_constructor_versioned(
			seat, FB_GFX3_WL_SEAT_GET_TOUCH, platform->touch_interface,
			version, NULL);
		if ((platform->touch == NULL) ||
		    (platform->api->proxy_add_listener(platform->touch,
				(void (**)(void))&platform_wayland_touch_listener,
				platform) != 0))
			platform_wayland_proxy_release(platform, &platform->touch,
				FB_GFX3_WL_TOUCH_RELEASE, 3u);
	} else if (!(capabilities & FB_GFX3_WL_SEAT_CAPABILITY_TOUCH)) {
		platform_wayland_touch_cancel(platform, NULL);
		platform_wayland_proxy_release(platform, &platform->touch,
			FB_GFX3_WL_TOUCH_RELEASE, 3u);
	}
}

static const FB_GFX3_WAYLAND_SEAT_LISTENER platform_wayland_seat_listener = {
	platform_wayland_seat_capabilities,
	platform_wayland_seat_name
};

static void platform_wayland_repeat_tick(FB_GFX3_PLATFORM_WAYLAND *platform)
{
	uint64_t now;
	unsigned int count = 0;
	if ((platform->input == NULL) ||
	    (!platform->repeat_text_only && platform->repeat_scancode <= 0) ||
	    (platform->repeat_interval_ms == 0u))
		return;
	now = platform_wayland_time_ms();
	while ((now >= platform->repeat_deadline_ms) && (count < 4u)) {
		if (platform->repeat_text_only) {
			platform_wayland_emit_text(platform, platform->repeat_text);
		} else {
			fb_gfx3_input_platform_key(platform->input, EVENT_KEY_REPEAT,
				platform->repeat_scancode, platform->repeat_ascii);
			fb_gfx3_input_platform_character(platform->input,
				platform->repeat_key, 1u);
			if (platform->repeat_unicode)
				fb_gfx3_input_platform_text(platform->input,
					platform->repeat_unicode);
		}
		platform->repeat_deadline_ms += platform->repeat_interval_ms;
		count++;
	}
}

static void platform_wayland_wm_ping(void *data, struct wl_proxy *wm_base,
	uint32_t serial)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	if ((platform == NULL) || (platform->api == NULL))
		return;
	platform->api->proxy_marshal(wm_base,
		FB_GFX3_XDG_WM_BASE_PONG, serial);
	(void)platform->api->display_flush(platform->display);
}

static void platform_wayland_surface_configure(void *data,
	struct wl_proxy *xdg_surface, uint32_t serial)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	if ((platform == NULL) || (platform->api == NULL))
		return;
	platform->api->proxy_marshal(xdg_surface,
		FB_GFX3_XDG_SURFACE_ACK_CONFIGURE, serial);
	platform->configured = TRUE;
	(void)platform->api->display_flush(platform->display);
}

static void platform_wayland_toplevel_configure(void *data,
	struct wl_proxy *toplevel, int32_t width, int32_t height,
	struct wl_array *states)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)toplevel;
	(void)states;
	if ((platform == NULL) || (width < 0) || (height < 0) ||
	    (width > 8192) || (height > 8192))
		return;
	if (width > 0)
		platform->view_width = (uint32_t)width;
	if (height > 0)
		platform->view_height = (uint32_t)height;
	if ((platform->egl_window != NULL) &&
	    (platform->egl.window_resize != NULL) &&
	    (platform->view_width > 0u) && (platform->view_height > 0u))
		platform->egl.window_resize(platform->egl_window,
			(int)platform->view_width, (int)platform->view_height, 0, 0);
	if ((platform->input != NULL) &&
	    (platform->flags & FB_GFX3_WINDOW_RESIZABLE) &&
	    (platform->view_width > 0u) && (platform->view_height > 0u))
		fb_gfx3_input_platform_resize(platform->input,
			platform->view_width, platform->view_height);
}

static void platform_wayland_toplevel_close(void *data,
	struct wl_proxy *toplevel)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)data;
	(void)toplevel;
	if (platform == NULL)
		return;
	platform->close_requested = TRUE;
	if (platform->input != NULL)
		fb_gfx3_input_platform_close(platform->input);
}

static void platform_wayland_toplevel_configure_bounds(void *data,
	struct wl_proxy *toplevel, int32_t width, int32_t height)
{
	(void)data;
	(void)toplevel;
	(void)width;
	(void)height;
}

static void platform_wayland_toplevel_capabilities(void *data,
	struct wl_proxy *toplevel, struct wl_array *capabilities)
{
	(void)data;
	(void)toplevel;
	(void)capabilities;
}

static void platform_wayland_publish_keyboard_state(
	FB_GFX3_PLATFORM_WAYLAND *platform)
{
	int visible = -2;

	/*
	 * A v3 input panel cannot be enabled for this surface until the
	 * compositor sends enter. Once enabled, v3 has no visibility event,
	 * so its actual state remains unknown. The v2 panel state event can
	 * report visibility independently and must remain unknown until then.
	 */
	if ((platform->text_input != NULL) &&
	    (!platform->text_input_entered || !platform->keyboard_requested))
		visible = 0;
	if (platform->input != NULL)
		fb_gfx3_input_platform_keyboard_state(platform->input,
			(platform->text_input != NULL) ||
			(platform->text_input_v2 != NULL), visible);
}

static void platform_wayland_bind_input_globals(
	FB_GFX3_PLATFORM_WAYLAND *platform)
{
	if ((platform->global_seat != 0u) && (platform->seat == NULL)) {
		if (platform_wayland_registry_bind(platform, platform->global_seat,
			"wl_seat", &platform->seat, 4u) == FB_GFX3_OK) {
			if (platform->api->proxy_add_listener(platform->seat,
				(void (**)(void))&platform_wayland_seat_listener,
				platform) != 0)
				platform_wayland_proxy_release(platform, &platform->seat,
					FB_GFX3_WL_SEAT_RELEASE, 5u);
		}
	}
	if ((platform->global_text_input_manager != 0u) &&
	    (platform->text_input_manager == NULL))
		(void)platform_wayland_registry_bind(platform,
			platform->global_text_input_manager,
			"zwp_text_input_manager_v3",
			&platform->text_input_manager, 1u);
	if ((platform->global_text_input_manager_v2 != 0u) &&
	    (platform->text_input_manager_v2 == NULL))
		(void)platform_wayland_registry_bind(platform,
			platform->global_text_input_manager_v2,
			"zwp_text_input_manager_v2",
			&platform->text_input_manager_v2, 1u);
}

static void platform_wayland_create_text_input(
	FB_GFX3_PLATFORM_WAYLAND *platform)
{
	if ((platform->seat == NULL) || (platform->surface == NULL))
		return;
	if (platform->text_input_manager != NULL) {
		platform->text_input = platform->api->proxy_marshal_constructor_versioned(
			platform->text_input_manager,
			FB_GFX3_WL_TEXT_INPUT_MANAGER_GET_TEXT_INPUT,
			&zwp_text_input_v3_interface, 1u, NULL, platform->seat);
		if ((platform->text_input != NULL) &&
		    (platform->api->proxy_add_listener(platform->text_input,
				(void (**)(void))&platform_wayland_text_v3_listener,
				platform) != 0))
			platform_wayland_destroy_object(platform,
				&platform->text_input, 0u);
	}
	if ((platform->text_input == NULL) &&
	    (platform->text_input_manager_v2 != NULL)) {
		platform->text_input_v2 =
			platform->api->proxy_marshal_constructor_versioned(
				platform->text_input_manager_v2,
				FB_GFX3_WL_TEXT_INPUT_V2_GET_TEXT_INPUT,
				&zwp_text_input_v2_interface, 1u, NULL, platform->seat);
		if ((platform->text_input_v2 != NULL) &&
		    (platform->api->proxy_add_listener(platform->text_input_v2,
				(void (**)(void))&platform_wayland_text_v2_listener,
				platform) != 0))
			platform_wayland_destroy_object(platform,
				&platform->text_input_v2, 0u);
	}
	platform_wayland_publish_keyboard_state(platform);
}

static void platform_wayland_apply_queued_input(
	FB_GFX3_PLATFORM_WAYLAND *platform)
{
	int visible;
	int x;
	int y;
	int width;
	int height;
	if (platform->input == NULL)
		return;
	if (fb_gfx3_input_take_soft_keyboard_request(platform->input,
		&visible) > 0) {
		platform->keyboard_requested = (visible != 0);
		platform_wayland_apply_keyboard(platform);
	}
	if (fb_gfx3_input_take_text_region(platform->input,
		&x, &y, &width, &height) > 0) {
		if (platform->region_x == x && platform->region_y == y &&
			platform->region_width == width &&
			platform->region_height == height)
			return;
		platform->region_x = x;
		platform->region_y = y;
		platform->region_width = width;
		platform->region_height = height;
		platform_wayland_update_text_region(platform);
	}
}

static int platform_wayland_load_egl_uncached(
	FB_GFX3_PLATFORM_WAYLAND *platform)
{
	void *symbols[13];
	static const char *const symbol_names[13] = {
		"eglGetProcAddress", "eglGetDisplay", "eglInitialize",
		"eglTerminate", "eglBindAPI", "eglChooseConfig",
		"eglCreateContext", "eglDestroyContext",
		"eglCreateWindowSurface", "eglDestroySurface", "eglMakeCurrent",
		"eglSwapBuffers", "eglSwapInterval"
	};
	void *function;
	size_t index;

	platform->egl.library = dlopen("libEGL.so.1", RTLD_NOW | RTLD_LOCAL);
	platform->egl.wayland_egl_library = dlopen("libwayland-egl.so.1",
		RTLD_NOW | RTLD_LOCAL);
	if ((platform->egl.library == NULL) ||
	    (platform->egl.wayland_egl_library == NULL))
		return FB_GFX3_UNSUPPORTED;
	for (index = 0; index < 13u; index++)
		symbols[index] = dlsym(platform->egl.library, symbol_names[index]);
	if (platform_wayland_copy_symbol(&platform->egl.get_proc_address,
		sizeof(platform->egl.get_proc_address), symbols[0]) != 0 ||
	    platform_wayland_copy_symbol(&platform->egl.get_display,
		sizeof(platform->egl.get_display), symbols[1]) != 0 ||
	    platform_wayland_copy_symbol(&platform->egl.initialize,
		sizeof(platform->egl.initialize), symbols[2]) != 0 ||
	    platform_wayland_copy_symbol(&platform->egl.terminate,
		sizeof(platform->egl.terminate), symbols[3]) != 0 ||
	    platform_wayland_copy_symbol(&platform->egl.bind_api,
		sizeof(platform->egl.bind_api), symbols[4]) != 0 ||
	    platform_wayland_copy_symbol(&platform->egl.choose_config,
		sizeof(platform->egl.choose_config), symbols[5]) != 0 ||
	    platform_wayland_copy_symbol(&platform->egl.create_context,
		sizeof(platform->egl.create_context), symbols[6]) != 0 ||
	    platform_wayland_copy_symbol(&platform->egl.destroy_context,
		sizeof(platform->egl.destroy_context), symbols[7]) != 0 ||
	    platform_wayland_copy_symbol(&platform->egl.create_window_surface,
		sizeof(platform->egl.create_window_surface), symbols[8]) != 0 ||
	    platform_wayland_copy_symbol(&platform->egl.destroy_surface,
		sizeof(platform->egl.destroy_surface), symbols[9]) != 0 ||
	    platform_wayland_copy_symbol(&platform->egl.make_current,
		sizeof(platform->egl.make_current), symbols[10]) != 0 ||
	    platform_wayland_copy_symbol(&platform->egl.swap_buffers,
		sizeof(platform->egl.swap_buffers), symbols[11]) != 0 ||
	    platform_wayland_copy_symbol(&platform->egl.swap_interval,
		sizeof(platform->egl.swap_interval), symbols[12]) != 0)
		return FB_GFX3_UNSUPPORTED;

	function = dlsym(platform->egl.library, "eglGetPlatformDisplay");
	if (function == NULL)
		function = platform->egl.get_proc_address("eglGetPlatformDisplayEXT");
	if (function != NULL && platform_wayland_copy_symbol(
		&platform->egl.get_platform_display,
		sizeof(platform->egl.get_platform_display), function) != 0)
		return FB_GFX3_UNSUPPORTED;
	function = dlsym(platform->egl.wayland_egl_library,
		"wl_egl_window_create");
	if (platform_wayland_copy_symbol(&platform->egl.window_create,
		sizeof(platform->egl.window_create), function) != 0)
		return FB_GFX3_UNSUPPORTED;
	function = dlsym(platform->egl.wayland_egl_library,
		"wl_egl_window_destroy");
	if (platform_wayland_copy_symbol(&platform->egl.window_destroy,
		sizeof(platform->egl.window_destroy), function) != 0)
		return FB_GFX3_UNSUPPORTED;
	function = dlsym(platform->egl.wayland_egl_library,
		"wl_egl_window_resize");
	if (platform_wayland_copy_symbol(&platform->egl.window_resize,
		sizeof(platform->egl.window_resize), function) != 0)
		return FB_GFX3_UNSUPPORTED;
	platform->egl.gles_library = dlopen("libGLESv2.so.2",
		RTLD_NOW | RTLD_LOCAL);
	platform->egl.gl_library = dlopen("libOpenGL.so.0",
		RTLD_NOW | RTLD_LOCAL);
	if (platform->egl.gl_library == NULL)
		platform->egl.gl_library = dlopen("libGL.so.1", RTLD_NOW | RTLD_LOCAL);
	return FB_GFX3_OK;
}

static int platform_wayland_load_egl(FB_GFX3_PLATFORM_WAYLAND *platform)
{
	int result;
	if (platform == NULL)
		return FB_GFX3_INVALID;
	if (pthread_mutex_lock(&fb_gfx3_wayland_egl_mutex) != 0)
		return FB_GFX3_FAILED;
	if (!fb_gfx3_wayland_egl_cache_ready) {
		fb_gfx3_wayland_egl_cache_result =
			platform_wayland_load_egl_uncached(platform);
		fb_gfx3_wayland_egl_cache = platform->egl;
		fb_gfx3_wayland_egl_cache_ready = TRUE;
	} else {
		platform->egl = fb_gfx3_wayland_egl_cache;
	}
	result = fb_gfx3_wayland_egl_cache_result;
	if (pthread_mutex_unlock(&fb_gfx3_wayland_egl_mutex) != 0)
		return FB_GFX3_FAILED;
	return result;
}

/*
	wl_proxy_add_listener retains each listener table pointer. These tables
	therefore need static storage because the proxies remain active after the
	connection setup function returns.
*/
static const FB_GFX3_WAYLAND_REGISTRY_LISTENER registry_listener = {
	platform_wayland_registry_global,
	platform_wayland_registry_global_remove
};

static const FB_GFX3_WAYLAND_WM_LISTENER wm_listener = {
	platform_wayland_wm_ping
};

static const FB_GFX3_WAYLAND_SURFACE_LISTENER surface_listener = {
	platform_wayland_surface_configure
};

static const FB_GFX3_WAYLAND_TOPLEVEL_LISTENER toplevel_listener = {
	platform_wayland_toplevel_configure,
	platform_wayland_toplevel_close,
	platform_wayland_toplevel_configure_bounds,
	platform_wayland_toplevel_capabilities
};

static int platform_wayland_load_symbols(FB_GFX3_PLATFORM_WAYLAND *platform)
{
	void *symbols[8];
	static const char *const symbol_names[8] = {
		"wl_registry_interface", "wl_compositor_interface",
		"wl_surface_interface", "wl_seat_interface",
		"wl_output_interface", "wl_pointer_interface",
		"wl_keyboard_interface", "wl_touch_interface"
	};

	if ((fb_hWaylandGetApi(&platform->api) != 0) ||
	    (fb_hWaylandInitProtocols() != 0) ||
	    (fb_hWaylandLoadSymbols(symbol_names, symbols, 8) != 0))
		return FB_GFX3_UNSUPPORTED;
	platform->registry_interface = symbols[0];
	platform->compositor_interface = symbols[1];
	platform->surface_interface = symbols[2];
	platform->seat_interface = symbols[3];
	platform->output_interface = symbols[4];
	platform->pointer_interface = symbols[5];
	platform->keyboard_interface = symbols[6];
	platform->touch_interface = symbols[7];
	return FB_GFX3_OK;
}

static int platform_wayland_connect(FB_GFX3_PLATFORM_WAYLAND *platform)
{
	if (platform->api == NULL)
		return FB_GFX3_UNSUPPORTED;
	if (platform->display == NULL)
		platform->display = platform->api->display_connect(NULL);
	if (platform->display == NULL)
		return FB_GFX3_UNSUPPORTED;
	platform->registry =
		platform->api->proxy_marshal_constructor_versioned(
			(struct wl_proxy *)platform->display, 1u,
			platform->registry_interface, 1u, NULL);
	if ((platform->registry == NULL) ||
	    (platform->api->proxy_add_listener(platform->registry,
		(void (**)(void))&registry_listener, platform) != 0) ||
	    (platform->api->display_roundtrip(platform->display) < 0))
		return FB_GFX3_FAILED;
	if ((platform->global_compositor == 0u) ||
	    (platform->global_wm_base == 0u))
		return FB_GFX3_UNSUPPORTED;
	return FB_GFX3_OK;
}

static void platform_wayland_request_toplevel_state(
	FB_GFX3_PLATFORM_WAYLAND *platform)
{
	if (!platform || !platform->api || !platform->api->proxy_marshal ||
	    !platform->toplevel)
		return;
	if (platform->flags & FB_GFX3_WINDOW_FULLSCREEN) {
		platform->api->proxy_marshal((struct wl_proxy *)platform->toplevel,
			FB_GFX3_XDG_TOPLEVEL_SET_FULLSCREEN, NULL);
	} else if ((platform->flags & FB_GFX3_WINDOW_RESIZABLE) &&
	    fb_hLinuxDeviceClass() == FB_GFX_DEVICE_CLASS_HANDHELD) {
		/* Let the compositor resize the usable area for rotation and the
		 * keyboard while keeping the mobile shell visible. */
		platform->api->proxy_marshal((struct wl_proxy *)platform->toplevel,
			FB_GFX3_XDG_TOPLEVEL_SET_MAXIMIZED);
	}
}

/*
 * A capability probe needs the display connection, but not a toplevel.
 * Creating and closing a temporary window can make Lomiri discard the
 * application's session before its real window is opened.
 */
static int platform_wayland_open_window(FB_GFX3_PLATFORM_WAYLAND *platform)
{
	char app_id[FB_WAYLAND_APP_ID_CAPACITY];

	platform_wayland_bind_input_globals(platform);
	if (platform_wayland_registry_bind(platform,
		platform->global_compositor, "wl_compositor",
		&platform->compositor, platform->compositor_version) != FB_GFX3_OK ||
	    platform_wayland_registry_bind(platform, platform->global_wm_base,
		"xdg_wm_base", &platform->wm_base,
		platform->wm_base_version) != FB_GFX3_OK)
		return FB_GFX3_UNSUPPORTED;
	if (platform->api->proxy_add_listener(platform->wm_base,
		(void (**)(void))&wm_listener, platform) != 0)
		return FB_GFX3_FAILED;
	platform->surface = platform->api->proxy_marshal_constructor_versioned(
		platform->compositor, FB_GFX3_WL_COMPOSITOR_CREATE_SURFACE,
		platform->surface_interface, 1, NULL);
	if (platform->surface == NULL)
		return FB_GFX3_FAILED;
	platform->xdg_surface = platform->api->proxy_marshal_constructor_versioned(
		platform->wm_base, FB_GFX3_XDG_WM_BASE_GET_SURFACE,
		&xdg_surface_interface, platform->wm_base_version,
		NULL, platform->surface);
	if ((platform->xdg_surface == NULL) ||
	    (platform->api->proxy_add_listener(platform->xdg_surface,
		(void (**)(void))&surface_listener, platform) != 0))
		return FB_GFX3_FAILED;
	platform->toplevel = platform->api->proxy_marshal_constructor_versioned(
		platform->xdg_surface, FB_GFX3_XDG_SURFACE_GET_TOPLEVEL,
		&xdg_toplevel_interface, platform->wm_base_version, NULL);
	if ((platform->toplevel == NULL) ||
	    (platform->api->proxy_add_listener(platform->toplevel,
		(void (**)(void))&toplevel_listener, platform) != 0))
		return FB_GFX3_FAILED;
	platform_wayland_create_text_input(platform);
	platform->api->proxy_marshal((struct wl_proxy *)platform->toplevel,
		FB_GFX3_XDG_TOPLEVEL_SET_APP_ID,
		fb_hWaylandAppId(app_id, sizeof(app_id), "freebasic-gfxlib3"));
	platform->api->proxy_marshal((struct wl_proxy *)platform->toplevel,
		FB_GFX3_XDG_TOPLEVEL_SET_TITLE, platform->title);
	platform_wayland_request_toplevel_state(platform);
	platform->api->proxy_marshal(platform->surface,
		FB_GFX3_WL_SURFACE_COMMIT);
	if (platform->api->display_roundtrip(platform->display) < 0 ||
	    !platform->configured)
		return FB_GFX3_FAILED;
	platform_wayland_apply_queued_input(platform);
	return FB_GFX3_OK;
}

static int platform_wayland_egl_display(FB_GFX3_PLATFORM_WAYLAND *platform)
{
	int major = 0;
	int minor = 0;
	if (platform->egl.get_platform_display != NULL)
		platform->egl_display = platform->egl.get_platform_display(
			FB_GFX3_EGL_PLATFORM_WAYLAND, platform->display, NULL);
	if (platform->egl_display == NULL)
		platform->egl_display = platform->egl.get_display(platform->display);
	if ((platform->egl_display == NULL) ||
	    !platform->egl.initialize(platform->egl_display, &major, &minor))
		return FB_GFX3_UNSUPPORTED;
	platform->owns_egl = TRUE;
	return FB_GFX3_OK;
}

static int platform_wayland_probe_opengl(void)
{
	FB_GFX3_PLATFORM_WAYLAND platform;
	int result;
	/*
	 * A short-lived Wayland connection can be registered as the application's
	 * session by Lomiri before the actual window connects. Check the required
	 * libraries here; window creation verifies the compositor and EGL display.
	 */
	memset(&platform, 0, sizeof(platform));
	result = platform_wayland_load_symbols(&platform);
	if (result == FB_GFX3_OK)
		result = platform_wayland_load_egl(&platform);
	return result;
}

static int platform_wayland_create_opengl(void **destination,
	const FB_GFX3_PLATFORM_OPENGL_CONFIG *config)
{
	FB_GFX3_PLATFORM_WAYLAND *platform;
	int result;
	int use_desktop_gl;
	int config_count = 0;
	int egl_attributes[21];
	int context_attributes[7];
	int attribute_index = 0;
	int context_index = 0;
	void *egl_configs[1] = { NULL };
	uint32_t width;
	uint32_t height;

	if ((destination == NULL) || (config == NULL) ||
	    (config->width < 2u) || (config->height < 2u) ||
	    (config->width > 8192u) || (config->height > 8192u) ||
	    ((uint64_t)config->width * config->height > 8388608u))
		return FB_GFX3_INVALID;
	*destination = NULL;
	platform = (FB_GFX3_PLATFORM_WAYLAND *)calloc(1, sizeof(*platform));
	if (platform == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	platform->input = (FB_GFX3_INPUT_STATE *)config->input;
	platform->flags = config->flags;
	platform->view_width = config->width;
	platform->view_height = config->height;
	if (config->title != NULL)
		strncpy(platform->title, config->title, sizeof(platform->title) - 1u);
	else
		strcpy(platform->title, "FreeBASIC gfxlib3");
	result = platform_wayland_load_egl(platform);
	if (result != FB_GFX3_OK)
		goto failed;
	result = platform_wayland_load_symbols(platform);
	if (result != FB_GFX3_OK)
		goto failed;
	if (fb_hWaylandDisplayBeginWindow(&platform->display) != 0) {
		result = FB_GFX3_FAILED;
		goto failed;
	}
	result = platform_wayland_connect(platform);
	if (result == FB_GFX3_OK)
		result = platform_wayland_open_window(platform);
	fb_hWaylandDisplayEndWindow();
	if (result != FB_GFX3_OK)
		goto failed;
	result = platform_wayland_egl_display(platform);
	if (result != FB_GFX3_OK)
		goto failed;
	use_desktop_gl = config->major_version >= 4u;
	if (!platform->egl.bind_api(use_desktop_gl ? FB_GFX3_EGL_OPENGL_API :
		FB_GFX3_EGL_OPENGL_ES_API)) {
		result = FB_GFX3_UNSUPPORTED;
		goto failed;
	}
	egl_attributes[attribute_index++] = FB_GFX3_EGL_SURFACE_TYPE;
	egl_attributes[attribute_index++] = FB_GFX3_EGL_WINDOW_BIT;
	egl_attributes[attribute_index++] = FB_GFX3_EGL_RENDERABLE_TYPE;
	egl_attributes[attribute_index++] = use_desktop_gl ?
		FB_GFX3_EGL_OPENGL_BIT : FB_GFX3_EGL_OPENGL_ES3_BIT;
	egl_attributes[attribute_index++] = FB_GFX3_EGL_RED_SIZE;
	egl_attributes[attribute_index++] = 8;
	egl_attributes[attribute_index++] = FB_GFX3_EGL_GREEN_SIZE;
	egl_attributes[attribute_index++] = 8;
	egl_attributes[attribute_index++] = FB_GFX3_EGL_BLUE_SIZE;
	egl_attributes[attribute_index++] = 8;
	egl_attributes[attribute_index++] = FB_GFX3_EGL_ALPHA_SIZE;
	egl_attributes[attribute_index++] = 8;
	egl_attributes[attribute_index++] = FB_GFX3_EGL_DEPTH_SIZE;
	egl_attributes[attribute_index++] = 0;
	egl_attributes[attribute_index++] = FB_GFX3_EGL_NONE;
	if (!platform->egl.choose_config(platform->egl_display, egl_attributes,
		egl_configs, 1, &config_count) || (config_count < 1) ||
		(egl_configs[0] == NULL)) {
		result = FB_GFX3_UNSUPPORTED;
		goto failed;
	}
	platform->egl_config = egl_configs[0];
	if (use_desktop_gl) {
		context_attributes[context_index++] =
			FB_GFX3_EGL_CONTEXT_CLIENT_VERSION;
		context_attributes[context_index++] = (int)config->major_version;
		context_attributes[context_index++] =
			FB_GFX3_EGL_CONTEXT_MINOR_VERSION;
		context_attributes[context_index++] = (int)config->minor_version;
		context_attributes[context_index++] =
			FB_GFX3_EGL_CONTEXT_PROFILE_MASK;
		context_attributes[context_index++] =
			FB_GFX3_EGL_CONTEXT_OPENGL_CORE_PROFILE;
		context_attributes[context_index++] = FB_GFX3_EGL_NONE;
	} else {
		context_attributes[context_index++] =
			FB_GFX3_EGL_CONTEXT_CLIENT_VERSION;
		context_attributes[context_index++] = (int)config->major_version;
		context_attributes[context_index++] = FB_GFX3_EGL_NONE;
	}
	platform->egl_context = platform->egl.create_context(
		platform->egl_display, platform->egl_config, NULL,
		context_attributes);
	if (platform->egl_context == NULL) {
		result = FB_GFX3_UNSUPPORTED;
		goto failed;
	}
	width = platform->view_width;
	height = platform->view_height;
	platform->egl_window = platform->egl.window_create(platform->surface,
		(int)width, (int)height);
	if (platform->egl_window == NULL) {
		result = FB_GFX3_FAILED;
		goto failed;
	}
	platform->egl_surface = platform->egl.create_window_surface(
		platform->egl_display, platform->egl_config,
		platform->egl_window, NULL);
	if (platform->egl_surface == NULL ||
	    !platform->egl.make_current(platform->egl_display,
		platform->egl_surface, platform->egl_surface,
		platform->egl_context)) {
		result = FB_GFX3_UNSUPPORTED;
		goto failed;
	}
	/*
	 * Let the compositor pace presentation. With interval zero, older Mesa
	 * Wayland software renderers can exhaust their small buffer pool when
	 * releases lag behind swaps, then dereference a missing back buffer.
	 * EGL may clamp this request to the chosen configuration's limits.
	 */
	platform->egl.swap_interval(platform->egl_display, 1);
	platform->shown = TRUE;
	if (platform->input != NULL)
		fb_gfx3_input_platform_window_info(platform->input,
			(uintptr_t)platform->surface, (uintptr_t)platform->display,
			0, 0, (int)width, (int)height);
	*destination = platform;
	return FB_GFX3_OK;

failed:
	if (platform != NULL) {
		if (platform->owns_egl && platform->egl.make_current != NULL)
			(void)platform->egl.make_current(platform->egl_display,
				NULL, NULL, NULL);
		if (platform->egl_context != NULL &&
		    platform->egl.destroy_context != NULL)
			platform->egl.destroy_context(platform->egl_display,
				platform->egl_context);
		if (platform->egl_surface != NULL &&
		    platform->egl.destroy_surface != NULL)
			platform->egl.destroy_surface(platform->egl_display,
				platform->egl_surface);
		if (platform->egl_window != NULL &&
		    platform->egl.window_destroy != NULL)
			platform->egl.window_destroy(platform->egl_window);
		if (platform->owns_egl && platform->egl.terminate != NULL)
			platform->egl.terminate(platform->egl_display);
		platform_wayland_input_shutdown(platform);
		if (platform->toplevel != NULL && platform->api != NULL) {
			platform->api->proxy_marshal(platform->toplevel,
				FB_GFX3_XDG_TOPLEVEL_DESTROY);
			platform->api->proxy_destroy(platform->toplevel);
		}
		if (platform->xdg_surface != NULL && platform->api != NULL) {
			platform->api->proxy_marshal(platform->xdg_surface,
				FB_GFX3_XDG_SURFACE_DESTROY);
			platform->api->proxy_destroy(platform->xdg_surface);
		}
		if (platform->surface != NULL && platform->api != NULL) {
			platform->api->proxy_marshal(platform->surface,
				FB_GFX3_WL_SURFACE_DESTROY);
			platform->api->proxy_destroy(platform->surface);
		}
		if (platform->wm_base != NULL && platform->api != NULL) {
			platform->api->proxy_marshal(platform->wm_base,
				FB_GFX3_XDG_WM_BASE_DESTROY);
			platform->api->proxy_destroy(platform->wm_base);
		}
		if (platform->compositor != NULL && platform->api != NULL)
			platform->api->proxy_destroy(platform->compositor);
		if (platform->registry != NULL && platform->api != NULL)
			platform->api->proxy_destroy(platform->registry);
		if (platform->display != NULL && platform->api != NULL)
			platform->api->display_disconnect(platform->display);
		free(platform);
	}
	return result;
}

static void platform_wayland_destroy(void *state)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)state;
	if (platform == NULL)
		return;
	fb_hLinuxMaliitCancelPending();
	if (platform->owns_egl && platform->egl.make_current != NULL)
		(void)platform->egl.make_current(platform->egl_display,
			NULL, NULL, NULL);
	if (platform->egl_context != NULL && platform->egl.destroy_context != NULL)
		platform->egl.destroy_context(platform->egl_display,
			platform->egl_context);
	if (platform->egl_surface != NULL && platform->egl.destroy_surface != NULL)
		platform->egl.destroy_surface(platform->egl_display,
			platform->egl_surface);
	if (platform->egl_window != NULL && platform->egl.window_destroy != NULL)
		platform->egl.window_destroy(platform->egl_window);
	if (platform->owns_egl && platform->egl.terminate != NULL)
		platform->egl.terminate(platform->egl_display);
	platform_wayland_input_shutdown(platform);
	if (platform->toplevel != NULL && platform->api != NULL) {
		platform->api->proxy_marshal(platform->toplevel,
			FB_GFX3_XDG_TOPLEVEL_DESTROY);
		platform->api->proxy_destroy(platform->toplevel);
	}
	if (platform->xdg_surface != NULL && platform->api != NULL) {
		platform->api->proxy_marshal(platform->xdg_surface,
			FB_GFX3_XDG_SURFACE_DESTROY);
		platform->api->proxy_destroy(platform->xdg_surface);
	}
	if (platform->surface != NULL && platform->api != NULL) {
		platform->api->proxy_marshal(platform->surface,
			FB_GFX3_WL_SURFACE_DESTROY);
		platform->api->proxy_destroy(platform->surface);
	}
	if (platform->wm_base != NULL && platform->api != NULL) {
		platform->api->proxy_marshal(platform->wm_base,
			FB_GFX3_XDG_WM_BASE_DESTROY);
		platform->api->proxy_destroy(platform->wm_base);
	}
	if (platform->compositor != NULL && platform->api != NULL)
		platform->api->proxy_destroy(platform->compositor);
	if (platform->registry != NULL && platform->api != NULL)
		platform->api->proxy_destroy(platform->registry);
	if (platform->display != NULL && platform->api != NULL)
		platform->api->display_disconnect(platform->display);
	free(platform);
}

static int platform_wayland_load_opengl_function(void *state,
	const char *name, void *destination, size_t destination_size)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)state;
	void *function;
	if ((platform == NULL) || (name == NULL) || (destination == NULL) ||
	    (destination_size != sizeof(function)))
		return FB_GFX3_INVALID;
	function = platform->egl.get_proc_address(name);
	if (function == NULL && platform->egl.gles_library != NULL)
		function = dlsym(platform->egl.gles_library, name);
	if (function == NULL && platform->egl.gl_library != NULL)
		function = dlsym(platform->egl.gl_library, name);
	if (function == NULL)
		return FB_GFX3_UNSUPPORTED;
	memcpy(destination, &function, sizeof(function));
	return FB_GFX3_OK;
}

static int platform_wayland_client_size(void *state, uint32_t *width,
	uint32_t *height)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)state;
	if ((platform == NULL) || (width == NULL) || (height == NULL))
		return FB_GFX3_INVALID;
	if ((platform->view_width < 2u) || (platform->view_height < 2u))
		return FB_GFX3_FAILED;
	*width = platform->view_width;
	*height = platform->view_height;
	return FB_GFX3_OK;
}

static int platform_wayland_desktop_info(ssize_t *width, ssize_t *height,
	ssize_t *depth, ssize_t *refresh)
{
	FB_WAYLAND_DISPLAY_INFO info;
	if (fb_hWaylandDisplayInfo(&info) != 0)
		return FB_GFX3_UNSUPPORTED;
	if (width != NULL)
		*width = info.width;
	if (height != NULL)
		*height = info.height;
	if (depth != NULL)
		*depth = info.depth;
	if (refresh != NULL)
		*refresh = info.refresh;
	return FB_GFX3_OK;
}

static int platform_wayland_swap_buffers(void *state)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)state;
	if ((platform == NULL) || (platform->egl_surface == NULL))
		return FB_GFX3_INVALID;
	return platform->egl.swap_buffers(platform->egl_display,
		platform->egl_surface) ? FB_GFX3_OK : FB_GFX3_FAILED;
}

static void platform_wayland_pump_events(void *state)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)state;
	struct pollfd poll_descriptor;
	int result;
	if ((platform == NULL) || (platform->api == NULL) ||
	    (platform->display == NULL))
		return;
	if (platform->api->display_dispatch_pending(platform->display) < 0)
		return;
	if ((platform->api->display_prepare_read == NULL) ||
	    (platform->api->display_read_events == NULL) ||
	    (platform->api->display_cancel_read == NULL) ||
	    (platform->api->display_get_fd == NULL)) {
		(void)platform->api->display_roundtrip(platform->display);
		platform_wayland_apply_queued_input(platform);
		platform_wayland_repeat_tick(platform);
		return;
	}
	while (platform->api->display_prepare_read(platform->display) != 0) {
		if (platform->api->display_dispatch_pending(platform->display) < 0)
			return;
	}
	poll_descriptor.fd = platform->api->display_get_fd(platform->display);
	poll_descriptor.events = POLLIN;
	poll_descriptor.revents = 0;
	result = poll(&poll_descriptor, 1, 0);
	if ((result > 0) && (poll_descriptor.revents & POLLIN))
		(void)platform->api->display_read_events(platform->display);
	else
		platform->api->display_cancel_read(platform->display);
	(void)platform->api->display_dispatch_pending(platform->display);
	(void)platform->api->display_flush(platform->display);
	platform_wayland_apply_queued_input(platform);
	platform_wayland_repeat_tick(platform);
}

static int platform_wayland_show_window(void *state)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)state;
	if (platform == NULL)
		return FB_GFX3_INVALID;
	platform->shown = TRUE;
	return FB_GFX3_OK;
}

static int platform_wayland_set_window_title(void *state, const char *title)
{
	FB_GFX3_PLATFORM_WAYLAND *platform =
		(FB_GFX3_PLATFORM_WAYLAND *)state;
	int result;
	if ((platform == NULL) || (title == NULL) ||
	    (platform->toplevel == NULL))
		return FB_GFX3_INVALID;
	platform->api->proxy_marshal(platform->toplevel,
		FB_GFX3_XDG_TOPLEVEL_SET_TITLE, title);
	/* A successful flush returns the number of bytes sent. On EAGAIN the
	 * queued request can be retried by the normal Wayland event loop. */
	result = platform->api->display_flush(platform->display);
	return ((result >= 0) || (errno == EAGAIN)) ? FB_GFX3_OK : FB_GFX3_FAILED;
}

static const FB_GFX3_PLATFORM_VTABLE __fb_gfx3_platform_wayland = {
	"Wayland/EGL",
	platform_wayland_probe_opengl,
	NULL,
	platform_wayland_create_opengl,
	NULL,
	platform_wayland_destroy,
	platform_wayland_load_opengl_function,
	platform_wayland_client_size,
	platform_wayland_desktop_info,
	platform_wayland_swap_buffers,
	platform_wayland_pump_events,
	platform_wayland_show_window,
	platform_wayland_set_window_title
};

const FB_GFX3_PLATFORM_VTABLE *fb_gfx3_platform_wayland(void)
{
	return &__fb_gfx3_platform_wayland;
}

int fb_gfx3_platform_wayland_should_default(void)
{
	const char *requested = getenv("FBGFX_GFX3_PLATFORM");
	const char *display = getenv("WAYLAND_DISPLAY");
	if ((requested != NULL) && (strcmp(requested, "wayland") == 0))
		return TRUE;
	if ((requested != NULL) &&
	    ((strcmp(requested, "x11") == 0) ||
	     (strcmp(requested, "drm") == 0)))
		return FALSE;
	return fb_hWaylandShouldUse() &&
		(display != NULL) && (display[0] != '\0');
}

/* end of linux/gfx3_platform_wayland.c */
