/*
 * Project: FreeBASIC gfxlib2 Linux Wayland support
 * File: gfx_driver_wayland.c
 * Purpose: Present the gfxlib2 software framebuffer in a Wayland toplevel.
 * Responsibilities: Own the xdg-shell or legacy wl_shell window,
 *     shared-memory buffer, native pointer, keyboard and touch input,
 *     and standard or legacy webOS text-input requests.
 * This file intentionally does not classify devices or implement widgets.
 */
#define _POSIX_C_SOURCE 200809L

#include "../../fb_gfx.h"
#include "fb_gfx_linux.h"
#include "fb_linux_device.h"
#include "fb_linux_maliit.h"
#include "fb_wayland_client.h"
#include "fb_wayland_display.h"
#include "fb_wayland_keymap.h"
#include "fb_wayland_text_v3.h"
#include "fb_wayland_webos.h"
#include "fb_wayland_window.h"
#include "../mobile_control.h"
#include "protocols/wayland-util.h"

#include <fcntl.h>
#include <dlfcn.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* ------------------------------------------------------------------------- */
/* Protocol constants and client-owned objects                               */
/* ------------------------------------------------------------------------- */

#define FB_WL_DISPLAY_GET_REGISTRY 1
#define FB_WL_REGISTRY_BIND 0
#define FB_WL_COMPOSITOR_CREATE_SURFACE 0
#define FB_WL_SHM_CREATE_POOL 0
#define FB_WL_SHM_POOL_CREATE_BUFFER 0
#define FB_WL_SHM_POOL_DESTROY 1
#define FB_WL_SURFACE_ATTACH 1
#define FB_WL_SURFACE_DAMAGE 2
#define FB_WL_SURFACE_FRAME 3
#define FB_WL_SURFACE_COMMIT 6
#define FB_WL_SURFACE_SET_BUFFER_SCALE 8
#define FB_WL_XDG_WM_BASE_GET_XDG_SURFACE 2
#define FB_WL_XDG_WM_BASE_PONG 3
#define FB_WL_XDG_SURFACE_GET_TOPLEVEL 1
#define FB_WL_XDG_SURFACE_ACK_CONFIGURE 4
#define FB_WL_XDG_TOPLEVEL_SET_TITLE 2
#define FB_WL_XDG_TOPLEVEL_SET_APP_ID 3
#define FB_WL_XDG_TOPLEVEL_SET_FULLSCREEN 11
#define FB_WL_XDG_TOPLEVEL_SET_MAXIMIZED 9
#define FB_WL_WL_SHELL_GET_SHELL_SURFACE 0
#define FB_WL_SHELL_SURFACE_PONG 0
#define FB_WL_SHELL_SURFACE_SET_TOPLEVEL 3
#define FB_WL_SHELL_SURFACE_SET_FULLSCREEN 5
#define FB_WL_SHELL_SURFACE_SET_MAXIMIZED 7
#define FB_WL_SHELL_SURFACE_SET_TITLE 8
#define FB_WL_SHELL_SURFACE_SET_CLASS 9
#define FB_WL_SHELL_FULLSCREEN_METHOD_DEFAULT 0
#define FB_WL_TEXT_INPUT_MANAGER_GET_TEXT_INPUT 1
#define FB_WL_TEXT_INPUT_MANAGER_V2_GET_TEXT_INPUT 1
/* text-input-v3 request numbers are zero-based in the protocol definition. */
#define FB_WL_TEXT_INPUT_ENABLE 1
#define FB_WL_TEXT_INPUT_DISABLE 2
#define FB_WL_TEXT_INPUT_SET_CURSOR_RECTANGLE 6
#define FB_WL_TEXT_INPUT_COMMIT 7
#define FB_WL_TEXT_INPUT_V2_ENABLE 1
#define FB_WL_TEXT_INPUT_V2_DISABLE 2
#define FB_WL_TEXT_INPUT_V2_SHOW_INPUT_PANEL 3
#define FB_WL_TEXT_INPUT_V2_HIDE_INPUT_PANEL 4
#define FB_WL_TEXT_INPUT_V2_SET_CURSOR_RECTANGLE 7
#define FB_WL_TEXT_INPUT_V2_UPDATE_STATE 9
#define FB_WL_TEXT_INPUT_V2_UPDATE_STATE_CHANGE 0
#define FB_WL_TEXT_INPUT_V2_UPDATE_STATE_ENTER 3
#define FB_WL_SEAT_GET_KEYBOARD 1
#define FB_WL_SEAT_GET_TOUCH 2
#define FB_WL_SEAT_GET_POINTER 0
#define FB_WL_KEYBOARD_RELEASE 0
#define FB_WL_POINTER_RELEASE 1
#define FB_WL_BUFFER_DESTROY 0
#define FB_WL_FORMAT_XRGB8888 1
#define FB_WL_MAX_BUFFER_BYTES (256U * 1024U * 1024U)
#define FB_WL_MAX_TOTAL_BUFFER_BYTES (512U * 1024U * 1024U)
/* Two current buffers plus space for buffers still owned during a resize. */
#define FB_WL_BUFFER_SLOTS 4
#define FB_WL_PRESENT_BUFFERS 2
#define FB_WL_MAX_TOUCH_POINTS 16
#define FB_WL_EVDEV_KEY_MAX 768u
/* Service a quiet window about 60 times per second, including buffer release. */
#define FB_WL_POLL_INTERVAL_NS 16000000L

struct wl_display;
struct wl_proxy;
/* Public libwayland array ABI used by wl_keyboard.enter's pressed-key list. */
struct wl_array {
	size_t size;
	size_t alloc;
	void *data;
};
struct wl_registry;
struct wl_compositor;
struct wl_surface;
struct wl_shm;
struct wl_shm_pool;
struct wl_buffer;
struct wl_callback;
struct wl_seat;
struct wl_pointer;
struct wl_keyboard;
struct wl_touch;
struct wl_surface;
struct wl_shell;
struct wl_shell_surface;
struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;
struct qt_windowmanager;
struct zwp_text_input_manager_v3;
struct zwp_text_input_v3;
struct zwp_text_input_manager_v2;
struct zwp_text_input_v2;

extern const struct wl_interface wl_registry_interface;
extern const struct wl_interface wl_compositor_interface;
extern const struct wl_interface wl_shm_interface;
extern const struct wl_interface wl_seat_interface;
extern const struct wl_interface wl_touch_interface;
extern const struct wl_interface wl_shm_pool_interface;
extern const struct wl_interface wl_buffer_interface;
extern const struct wl_interface wl_shell_interface;
extern const struct wl_interface wl_shell_surface_interface;
extern const struct wl_interface xdg_wm_base_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_toplevel_interface;
extern const struct wl_interface zwp_text_input_manager_v3_interface;
extern const struct wl_interface zwp_text_input_v3_interface;
extern const struct wl_interface zwp_text_input_manager_v2_interface;
extern const struct wl_interface zwp_text_input_v2_interface;

typedef struct FB_WL_REGISTRY_LISTENER {
	void (*global)(void *, struct wl_registry *, uint32_t, const char *, uint32_t);
	void (*global_remove)(void *, struct wl_registry *, uint32_t);
} FB_WL_REGISTRY_LISTENER;

typedef struct FB_WL_WM_BASE_LISTENER {
	void (*ping)(void *, struct xdg_wm_base *, uint32_t);
} FB_WL_WM_BASE_LISTENER;

typedef struct FB_WL_QT_WINDOWMANAGER_LISTENER {
	void (*hints)(void *, struct qt_windowmanager *, int32_t);
	void (*quit)(void *, struct qt_windowmanager *);
} FB_WL_QT_WINDOWMANAGER_LISTENER;

typedef struct FB_WL_SHELL_SURFACE_LISTENER {
	void (*ping)(void *, struct wl_shell_surface *, uint32_t);
	void (*configure)(void *, struct wl_shell_surface *, uint32_t,
		int32_t, int32_t);
	void (*popup_done)(void *, struct wl_shell_surface *);
} FB_WL_SHELL_SURFACE_LISTENER;

typedef struct FB_WL_XDG_SURFACE_LISTENER {
	void (*configure)(void *, struct xdg_surface *, uint32_t);
} FB_WL_XDG_SURFACE_LISTENER;

typedef struct FB_WL_TOPLEVEL_LISTENER {
	void (*configure)(void *, struct xdg_toplevel *, int32_t, int32_t,
		struct wl_array *);
	void (*close)(void *, struct xdg_toplevel *);
	void (*configure_bounds)(void *, struct xdg_toplevel *, int32_t, int32_t);
	void (*wm_capabilities)(void *, struct xdg_toplevel *, struct wl_array *);
} FB_WL_TOPLEVEL_LISTENER;

typedef struct FB_WL_BUFFER_LISTENER {
	void (*release)(void *, struct wl_buffer *);
} FB_WL_BUFFER_LISTENER;

typedef struct FB_WL_FRAME_LISTENER {
	void (*done)(void *, struct wl_callback *, uint32_t);
} FB_WL_FRAME_LISTENER;

typedef struct FB_WL_SEAT_LISTENER {
	void (*capabilities)(void *, struct wl_seat *, uint32_t);
	void (*name)(void *, struct wl_seat *, const char *);
} FB_WL_SEAT_LISTENER;

typedef struct FB_WL_POINTER_LISTENER {
	void (*enter)(void *, struct wl_pointer *, uint32_t,
		struct wl_surface *, int32_t, int32_t);
	void (*leave)(void *, struct wl_pointer *, uint32_t,
		struct wl_surface *);
	void (*motion)(void *, struct wl_pointer *, uint32_t, int32_t, int32_t);
	void (*button)(void *, struct wl_pointer *, uint32_t, uint32_t,
		uint32_t, uint32_t);
	void (*axis)(void *, struct wl_pointer *, uint32_t, uint32_t, int32_t);
	void (*frame)(void *, struct wl_pointer *);
	void (*axis_source)(void *, struct wl_pointer *, uint32_t);
	void (*axis_stop)(void *, struct wl_pointer *, uint32_t, uint32_t);
	void (*axis_discrete)(void *, struct wl_pointer *, uint32_t, int32_t);
} FB_WL_POINTER_LISTENER;

typedef struct FB_WL_KEYBOARD_LISTENER {
	void (*keymap)(void *, struct wl_keyboard *, uint32_t, int32_t, uint32_t);
	void (*enter)(void *, struct wl_keyboard *, uint32_t, struct wl_surface *,
		struct wl_array *);
	void (*leave)(void *, struct wl_keyboard *, uint32_t, struct wl_surface *);
	void (*key)(void *, struct wl_keyboard *, uint32_t, uint32_t, uint32_t,
		uint32_t);
	void (*modifiers)(void *, struct wl_keyboard *, uint32_t, uint32_t,
		uint32_t, uint32_t, uint32_t);
	void (*repeat_info)(void *, struct wl_keyboard *, int32_t, int32_t);
} FB_WL_KEYBOARD_LISTENER;

struct xkb_context;
struct xkb_keymap;
struct xkb_state;

typedef struct FB_WL_XKB_API {
	void *library;
	struct xkb_context *(*context_new)(int);
	void (*context_unref)(struct xkb_context *);
	struct xkb_keymap *(*keymap_new_from_string)(struct xkb_context *,
		const char *, int, int);
	void (*keymap_unref)(struct xkb_keymap *);
	struct xkb_state *(*state_new)(struct xkb_keymap *);
	void (*state_unref)(struct xkb_state *);
	uint32_t (*state_update_key)(struct xkb_state *, uint32_t, uint32_t);
	uint32_t (*state_update_mask)(struct xkb_state *, uint32_t, uint32_t,
		uint32_t, uint32_t, uint32_t, uint32_t);
	int (*state_key_get_utf8)(struct xkb_state *, uint32_t, char *, size_t);
	uint32_t (*state_key_get_one_sym)(struct xkb_state *, uint32_t);
} FB_WL_XKB_API;

typedef struct FB_WL_TOUCH_LISTENER {
	void (*down)(void *, struct wl_touch *, uint32_t, uint32_t,
		struct wl_surface *, int32_t, int32_t, int32_t);
	void (*up)(void *, struct wl_touch *, uint32_t, uint32_t, int32_t);
	void (*motion)(void *, struct wl_touch *, uint32_t, int32_t, int32_t, int32_t);
	void (*frame)(void *, struct wl_touch *);
	void (*cancel)(void *, struct wl_touch *);
	void (*shape)(void *, struct wl_touch *, int32_t, int32_t, int32_t);
	void (*orientation)(void *, struct wl_touch *, int32_t, int32_t);
} FB_WL_TOUCH_LISTENER;

typedef struct FB_WL_TEXT_INPUT_LISTENER {
	void (*enter)(void *, struct zwp_text_input_v3 *, struct wl_surface *);
	void (*leave)(void *, struct zwp_text_input_v3 *, struct wl_surface *);
	void (*preedit_string)(void *, struct zwp_text_input_v3 *, const char *, int32_t, int32_t);
	void (*commit_string)(void *, struct zwp_text_input_v3 *, const char *);
	void (*delete_surrounding_text)(void *, struct zwp_text_input_v3 *, uint32_t, uint32_t);
	void (*done)(void *, struct zwp_text_input_v3 *, uint32_t);
} FB_WL_TEXT_INPUT_LISTENER;

typedef struct FB_WL_TEXT_INPUT_V2_LISTENER {
	void (*enter)(void *, struct zwp_text_input_v2 *, uint32_t,
		struct wl_surface *);
	void (*leave)(void *, struct zwp_text_input_v2 *, uint32_t,
		struct wl_surface *);
	void (*input_panel_state)(void *, struct zwp_text_input_v2 *, uint32_t,
		int32_t, int32_t, int32_t, int32_t);
	void (*preedit_string)(void *, struct zwp_text_input_v2 *,
		const char *, const char *);
	void (*preedit_styling)(void *, struct zwp_text_input_v2 *,
		uint32_t, uint32_t, uint32_t);
	void (*preedit_cursor)(void *, struct zwp_text_input_v2 *, int32_t);
	void (*commit_string)(void *, struct zwp_text_input_v2 *, const char *);
	void (*cursor_position)(void *, struct zwp_text_input_v2 *, int32_t,
		int32_t);
	void (*delete_surrounding_text)(void *, struct zwp_text_input_v2 *,
		uint32_t, uint32_t);
	void (*modifiers_map)(void *, struct zwp_text_input_v2 *, struct wl_array *);
	void (*keysym)(void *, struct zwp_text_input_v2 *, uint32_t, uint32_t,
		uint32_t, uint32_t);
	void (*language)(void *, struct zwp_text_input_v2 *, const char *);
	void (*text_direction)(void *, struct zwp_text_input_v2 *, uint32_t);
	void (*configure_surrounding_text)(void *, struct zwp_text_input_v2 *,
		int32_t, int32_t);
	void (*input_method_changed)(void *, struct zwp_text_input_v2 *,
		uint32_t, uint32_t);
} FB_WL_TEXT_INPUT_V2_LISTENER;

typedef struct FB_WAYLAND_BUFFER {
	struct wl_buffer *buffer;
	void *pixels;
	char *dirty;
	size_t size;
	int width;
	int height;
	int busy;
	int retired;
} FB_WAYLAND_BUFFER;

typedef struct FB_WAYLAND_WINDOW {
	const FB_WAYLAND_API *api;
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_shm *shm;
	const struct wl_interface *surface_interface;
	const struct wl_interface *callback_interface;
	struct wl_seat *seat;
	struct wl_pointer *pointer;
	struct wl_keyboard *keyboard;
	struct wl_touch *touch;
	struct wl_surface *surface;
	struct wl_callback *frame_callback;
	const struct wl_interface *shell_interface;
	const struct wl_interface *shell_surface_interface;
	struct wl_shell *shell;
	struct wl_shell_surface *shell_surface;
	struct xdg_wm_base *wm_base;
	struct qt_windowmanager *qt_windowmanager;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *toplevel;
	struct zwp_text_input_manager_v3 *text_input_manager;
	struct zwp_text_input_v3 *text_input;
	struct zwp_text_input_manager_v2 *text_input_manager_v2;
	struct zwp_text_input_v2 *text_input_v2;
	FB_WAYLAND_WEBOS webos;
	FB_WAYLAND_BUFFER buffers[FB_WL_BUFFER_SLOTS];
	int width;
	int height;
	int stride;
	int configured;
	/* Role dimensions become a transaction only at xdg_surface.configure. */
	int toplevel_width;
	int toplevel_height;
	int toplevel_maximized;
	int maximized;
	int configure_width;
	int configure_height;
	uint32_t configure_serial;
	int configure_pending;
	int repaint_pending;
	int resize_present_pending;
	int text_input_entered;
	/* The focused editor's request survives a temporary surface focus loss. */
	int keyboard_requested;
	/* Unoccluded surface size, retained until the compositor restores it. */
	int keyboard_base_width;
	int keyboard_base_height;
	int keyboard_viewport_reserved;
	int keyboard_visible;
	/* text-input-v2 reports the panel overlap in surface-local coordinates. */
	int32_t panel_x;
	int32_t panel_y;
	int32_t panel_width;
	int32_t panel_height;
	uint32_t text_input_serial;
	FB_WAYLAND_TEXT_V3_PENDING text_v3_pending;
	int region_x;
	int region_y;
	int region_width;
	int region_height;
	/* The registered region may be cleared and restored within one GUI update. */
	int sent_region_valid;
	int sent_region_x;
	int sent_region_y;
	int sent_region_width;
	int sent_region_height;
	int active;
	int touch_active;
	int keyboard_focused;
	int pointer_focused;
	uint32_t repeat_key;
	int repeat_scancode;
	uint32_t repeat_delay_ms;
	uint32_t repeat_interval_ms;
	uint64_t repeat_deadline_ms;
	FB_WL_XKB_API xkb;
	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;
	unsigned char evdev_key_down[FB_WL_EVDEV_KEY_MAX];
	unsigned char evdev_scancode[FB_WL_EVDEV_KEY_MAX];
	int32_t touch_primary_id;
	struct {
		int active;
	int32_t id;
	int x;
	int y;
	} touch_points[FB_WL_MAX_TOUCH_POINTS];
	int mouse_x;
	int mouse_y;
	int mouse_buttons;
	int pointer_buttons;
	int mouse_wheel;
	int mouse_hwheel;
	int flags;
	BLITTER *blitter;
} FB_WAYLAND_WINDOW;

static FB_WAYLAND_WINDOW fb_wayland_window;

/*
 * The window thread and application calls share the Wayland connection,
 * framebuffer and input state. The driver mutex protects all three. It must
 * be recursive because an application can query input or change pages while
 * it holds SCREENLOCK. The window thread never takes FB_GRAPHICS_LOCK, so
 * shutdown can join it while the application owns that lock.
 */
static pthread_mutex_t fb_wayland_mutex;
static pthread_once_t fb_wayland_mutex_once = PTHREAD_ONCE_INIT;
static int fb_wayland_mutex_ready;
static pthread_t fb_wayland_thread;
static int fb_wayland_thread_started;
static int fb_wayland_thread_running;
static int fb_wayland_thread_failed;

static void fb_wayland_poll_events_locked(void);
static void fb_wayland_present(void);
static void *fb_wayland_window_thread(void *unused);
static int fb_wayland_set_keyboard_locked(int visible);
static void fb_wayland_post_text(const char *text);
static void fb_wayland_destroy_slot(FB_WAYLAND_BUFFER *slot);
static void fb_wayland_toplevel_close(void *data, struct xdg_toplevel *toplevel);

static void fb_wayland_init_mutex(void)
{
	pthread_mutexattr_t attributes;

	if (pthread_mutexattr_init(&attributes) != 0)
		return;
	if (pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE) == 0 &&
	    pthread_mutex_init(&fb_wayland_mutex, &attributes) == 0)
		fb_wayland_mutex_ready = 1;
	pthread_mutexattr_destroy(&attributes);
}

/* Mobile controls can be queried before the Wayland driver is initialized. */
static int fb_wayland_prepare_mutex(void)
{
	return pthread_once(&fb_wayland_mutex_once, fb_wayland_init_mutex) == 0 &&
		fb_wayland_mutex_ready;
}

/* ------------------------------------------------------------------------- */
/* Shared Wayland proxy helpers                                               */
/* ------------------------------------------------------------------------- */

static void *fb_wayland_bind(struct wl_registry *registry, uint32_t name,
	const struct wl_interface *interface, uint32_t version)
{
	return fb_wayland_window.api->proxy_marshal_constructor_versioned(
		(struct wl_proxy *)registry, FB_WL_REGISTRY_BIND, interface, version,
		name, interface->name, version, NULL);
}

static int fb_wayland_add_listener(void *proxy, const void *listener, void *data)
{
	return fb_wayland_window.api->proxy_add_listener((struct wl_proxy *)proxy,
		(void (**)(void))listener, data);
}

static void fb_wayland_destroy(void *proxy)
{
	if (proxy)
		fb_wayland_window.api->proxy_destroy((struct wl_proxy *)proxy);
}

static void fb_wayland_destroy_buffer(struct wl_buffer *buffer)
{
	if (!buffer)
		return;
	fb_wayland_window.api->proxy_marshal((struct wl_proxy *)buffer,
		FB_WL_BUFFER_DESTROY);
	fb_wayland_destroy(buffer);
}

static void fb_wayland_destroy_slot(FB_WAYLAND_BUFFER *slot)
{
	if (slot->buffer)
		fb_wayland_destroy_buffer(slot->buffer);
	if (slot->pixels && slot->size)
		munmap(slot->pixels, slot->size);
	free(slot->dirty);
	memset(slot, 0, sizeof(*slot));
}

/*
 * Glacier closes clients through Qt's optional window-manager extension,
 * even when their surface uses xdg-shell. Binding its version-one quit event
 * lets the application use the same orderly close path as xdg_toplevel.close.
 * Registry and event callbacks run under the existing driver mutex.
 *
 * The wire ABI has one request (open_url: uint, string), then the hints(int)
 * and quit() events in that order. There is no protocol destructor request.
 */
static const struct wl_message fb_wayland_qt_windowmanager_requests[] = {
	{ "open_url", "us", NULL }
};

static const struct wl_message fb_wayland_qt_windowmanager_events[] = {
	{ "hints", "i", NULL },
	{ "quit", "", NULL }
};

static const struct wl_interface fb_wayland_qt_windowmanager_interface = {
	"qt_windowmanager", 1,
	1, fb_wayland_qt_windowmanager_requests,
	2, fb_wayland_qt_windowmanager_events
};

static void fb_wayland_qt_windowmanager_hints(void *data,
	struct qt_windowmanager *manager, int32_t show_is_fullscreen)
{
	/* Explicit gfxlib window flags continue to own the initial window state. */
	(void)data;
	(void)manager;
	(void)show_is_fullscreen;
}

static void fb_wayland_qt_windowmanager_quit(void *data,
	struct qt_windowmanager *manager)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	if (window && window->active && manager && window->qt_windowmanager == manager)
		fb_wayland_toplevel_close(window, NULL);
}

static const FB_WL_QT_WINDOWMANAGER_LISTENER fb_wayland_qt_windowmanager_listener = {
	fb_wayland_qt_windowmanager_hints,
	fb_wayland_qt_windowmanager_quit
};

static void fb_wayland_qt_windowmanager_destroy(FB_WAYLAND_WINDOW *window)
{
	fb_wayland_destroy(window->qt_windowmanager);
	window->qt_windowmanager = NULL;
}

static void fb_wayland_qt_windowmanager_bind(FB_WAYLAND_WINDOW *window,
	struct wl_registry *registry, uint32_t name, uint32_t version)
{
	if (window->qt_windowmanager || version == 0)
		return;
	window->qt_windowmanager = fb_wayland_bind(registry, name,
		&fb_wayland_qt_windowmanager_interface, 1);
	if (window->qt_windowmanager && fb_wayland_add_listener(window->qt_windowmanager,
		&fb_wayland_qt_windowmanager_listener, window) < 0)
		fb_wayland_qt_windowmanager_destroy(window);
}

static void fb_wayland_registry_global(void *data, struct wl_registry *registry,
	uint32_t name, const char *interface_name, uint32_t version)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	if (!interface_name || version == 0)
		return;
	fb_hWaylandWebosGlobal(&window->webos, window->api,
		(struct wl_proxy *)registry, name, interface_name, version);
	if (!window->compositor && strcmp(interface_name, "wl_compositor") == 0) {
		const struct wl_interface *interface = NULL;
		void *symbols[1];
		const char *const names[] = { "wl_compositor_interface" };
		if (fb_hWaylandLoadSymbols(names, symbols, 1) == 0)
			interface = (const struct wl_interface *)symbols[0];
		if (interface)
			window->compositor = fb_wayland_bind(registry, name, interface,
				version < 4 ? version : 4);
	} else if (!window->shm && strcmp(interface_name, "wl_shm") == 0) {
		void *symbols[1];
		const char *const names[] = { "wl_shm_interface" };
		if (fb_hWaylandLoadSymbols(names, symbols, 1) == 0 && symbols[0])
			window->shm = fb_wayland_bind(registry, name,
				(const struct wl_interface *)symbols[0], 1);
	} else if (!window->seat && strcmp(interface_name, "wl_seat") == 0) {
		void *symbols[1];
		const char *const names[] = { "wl_seat_interface" };
		if (fb_hWaylandLoadSymbols(names, symbols, 1) == 0 && symbols[0]) {
			window->seat = fb_wayland_bind(registry, name,
				(const struct wl_interface *)symbols[0], version < 5 ? version : 5);
		}
	} else if (!window->wm_base && strcmp(interface_name, "xdg_wm_base") == 0) {
		window->wm_base = fb_wayland_bind(registry, name,
			&xdg_wm_base_interface, version < 6 ? version : 6);
	} else if (strcmp(interface_name, "qt_windowmanager") == 0) {
		fb_wayland_qt_windowmanager_bind(window, registry, name, version);
	} else if (!window->shell && window->shell_interface &&
		strcmp(interface_name, "wl_shell") == 0) {
		window->shell = fb_wayland_bind(registry, name,
			window->shell_interface, 1);
	} else if (!window->text_input_manager &&
		strcmp(interface_name, "zwp_text_input_manager_v3") == 0) {
		window->text_input_manager = fb_wayland_bind(registry, name,
			&zwp_text_input_manager_v3_interface, 1);
	} else if (!window->text_input_manager_v2 &&
		strcmp(interface_name, "zwp_text_input_manager_v2") == 0) {
		window->text_input_manager_v2 = fb_wayland_bind(registry, name,
			&zwp_text_input_manager_v2_interface, 1);
	}
}

/* ------------------------------------------------------------------------- */
/* Window, touch, and text-input protocol callbacks                          */
/* ------------------------------------------------------------------------- */

static void fb_wayland_registry_global_remove(void *data,
	struct wl_registry *registry, uint32_t name)
{
	(void)data;
	(void)registry;
	(void)name;
}

static const FB_WL_REGISTRY_LISTENER fb_wayland_registry_listener = {
	fb_wayland_registry_global,
	fb_wayland_registry_global_remove
};

static void fb_wayland_wm_ping(void *data, struct xdg_wm_base *wm_base,
	uint32_t serial)
{
	(void)data;
	fb_wayland_window.api->proxy_marshal((struct wl_proxy *)wm_base,
		FB_WL_XDG_WM_BASE_PONG, serial);
}

static const FB_WL_WM_BASE_LISTENER fb_wayland_wm_base_listener = {
	fb_wayland_wm_ping
};

static void fb_wayland_shell_surface_ping(void *data,
	struct wl_shell_surface *shell_surface, uint32_t serial)
{
	(void)data;
	fb_wayland_window.api->proxy_marshal((struct wl_proxy *)shell_surface,
		FB_WL_SHELL_SURFACE_PONG, serial);
}

static void fb_wayland_shell_surface_configure(void *data,
	struct wl_shell_surface *shell_surface, uint32_t edges,
	int32_t width, int32_t height)
{
	(void)data;
	(void)shell_surface;
	(void)edges;
	if ((width > 0) && (height > 0) && fb_gfx &&
		(fb_gfx->flags & DRIVER_RESIZABLE)) {
		fb_hRequestResize(width, height);
	}
	fb_wayland_window.configured = 1;
	fb_wayland_window.repaint_pending = 1;
}

static void fb_wayland_shell_surface_popup_done(void *data,
	struct wl_shell_surface *shell_surface)
{
	(void)data;
	(void)shell_surface;
}

static const FB_WL_SHELL_SURFACE_LISTENER fb_wayland_shell_surface_listener = {
	fb_wayland_shell_surface_ping,
	fb_wayland_shell_surface_configure,
	fb_wayland_shell_surface_popup_done
};

/* The driver lock covers configure dispatch, framebuffer resize and present.
 * Acknowledging a resize makes the next surface commit apply that configure.
 * Keep old buffers on screen until both framebuffer and native buffer sizes
 * match it; otherwise KWin can place an old-sized maximized surface outside
 * the new work area. Zero dimensions leave that axis to the client. */
static void fb_wayland_observe_keyboard_viewport(FB_WAYLAND_WINDOW *window)
{
	/* Only a maximized, resizable v3 surface delegates its work area to the
	 * compositor. A floating user resize or fullscreen overlay is not evidence
	 * of keyboard reservation. Observe only acknowledged buffer transactions. */
	if (!window->maximized || !window->text_input || !fb_gfx ||
		!(fb_gfx->flags & DRIVER_RESIZABLE) ||
		(fb_gfx->flags & DRIVER_FULLSCREEN)) {
		window->keyboard_viewport_reserved = 0;
		return;
	}
	if (window->width == window->keyboard_base_width &&
		(window->keyboard_requested || window->height >= window->keyboard_base_height))
		window->keyboard_viewport_reserved = window->height > 0 &&
			window->height < window->keyboard_base_height;
	/* Keep a known reservation through rotation while the request is active. */
}

static int fb_wayland_ack_configure(FB_WAYLAND_WINDOW *window)
{
	if (!window->configure_pending)
		return 1;
	if ((window->configure_width > 0 &&
		(window->width != window->configure_width ||
		 (fb_gfx && fb_gfx->w != window->configure_width))) ||
		(window->configure_height > 0 &&
		(window->height != window->configure_height ||
		 (fb_gfx && fb_gfx->h != window->configure_height))))
		return 0;
	fb_wayland_observe_keyboard_viewport(window);
	window->api->proxy_marshal((struct wl_proxy *)window->xdg_surface,
		FB_WL_XDG_SURFACE_ACK_CONFIGURE, window->configure_serial);
	window->configure_pending = 0;
	return 1;
}

static void fb_wayland_xdg_configure(void *data, struct xdg_surface *surface,
	uint32_t serial)
{
	FB_WAYLAND_WINDOW *window = &fb_wayland_window;
	(void)data;
	(void)surface;
	window->configure_width = window->configure_height = 0;
	window->maximized = window->toplevel_maximized;
	if (fb_gfx && (fb_gfx->flags & DRIVER_RESIZABLE)) {
		window->configure_width = window->toplevel_width;
		window->configure_height = window->toplevel_height;
		if (window->configure_width > 0 || window->configure_height > 0)
			fb_hRequestResize(
				window->configure_width > 0 ? window->configure_width : fb_gfx->w,
				window->configure_height > 0 ? window->configure_height : fb_gfx->h);
	}
	/* A newer configure supersedes any unacknowledged serial. */
	window->configure_serial = serial;
	window->configure_pending = 1;
	window->configured = 1;
	window->repaint_pending = 1;
	(void)fb_wayland_ack_configure(window);
}

static const FB_WL_XDG_SURFACE_LISTENER fb_wayland_xdg_surface_listener = {
	fb_wayland_xdg_configure
};

static void fb_wayland_toplevel_configure(void *data,
	struct xdg_toplevel *toplevel, int32_t width, int32_t height,
	struct wl_array *states)
{
	(void)data;
	(void)toplevel;
	/* xdg_toplevel state 1 is maximized. Copy each word because foreign array
	 * storage need not be aligned for a C uint32_t access. */
	fb_wayland_window.toplevel_maximized = 0;
	if (states && states->data && states->size % sizeof(uint32_t) == 0) {
		for (size_t offset = 0; offset < states->size; offset += sizeof(uint32_t)) {
			uint32_t state;
			memcpy(&state, (const char *)states->data + offset, sizeof(state));
			if (state == 1)
				fb_wayland_window.toplevel_maximized = 1;
		}
	}
	fb_wayland_window.toplevel_width = width > 0 ? width : 0;
	fb_wayland_window.toplevel_height = height > 0 ? height : 0;
}

static void fb_wayland_toplevel_close(void *data, struct xdg_toplevel *toplevel)
{
	EVENT event;
	(void)data;
	(void)toplevel;
	memset(&event, 0, sizeof(event));
	event.type = EVENT_WINDOW_CLOSE;
	fb_hPostEvent(&event);
}

static void fb_wayland_toplevel_configure_bounds(void *data,
	struct xdg_toplevel *toplevel, int32_t width, int32_t height)
{
	(void)data;
	(void)toplevel;
	(void)width;
	(void)height;
}

static void fb_wayland_toplevel_capabilities(void *data,
	struct xdg_toplevel *toplevel, struct wl_array *capabilities)
{
	(void)data;
	(void)toplevel;
	(void)capabilities;
}

static const FB_WL_TOPLEVEL_LISTENER fb_wayland_toplevel_listener = {
	fb_wayland_toplevel_configure,
	fb_wayland_toplevel_close,
	fb_wayland_toplevel_configure_bounds,
	fb_wayland_toplevel_capabilities
};

static void fb_wayland_buffer_release(void *data, struct wl_buffer *buffer)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	int i;
	for (i = 0; i < FB_WL_BUFFER_SLOTS; i++) {
		if (buffer == window->buffers[i].buffer) {
			window->buffers[i].busy = 0;
			if (window->buffers[i].retired)
				fb_wayland_destroy_slot(&window->buffers[i]);
			return;
		}
	}
}

static const FB_WL_BUFFER_LISTENER fb_wayland_buffer_listener = {
	fb_wayland_buffer_release
};

/*
 * A frame callback is a compositor invitation to submit the next update.
 * It does not release the attached buffer; wl_buffer.release does that.
 */
static void fb_wayland_frame_done(void *data, struct wl_callback *callback,
	uint32_t time)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	(void)time;
	if (window->frame_callback != callback)
		return;
	window->frame_callback = NULL;
	fb_wayland_destroy(callback);
}

static const FB_WL_FRAME_LISTENER fb_wayland_frame_listener = {
	fb_wayland_frame_done
};

static void fb_wayland_post_mouse_button(int type, int button)
{
	EVENT event;
	memset(&event, 0, sizeof(event));
	event.type = type;
	/* EVENT stores mouse coordinates and the button in the same union.
	 * A touch sends a move before its button edge so clients retain the
	 * contact position when consuming the two separate event payloads. */
	if (type == EVENT_MOUSE_MOVE) {
		event.x = fb_wayland_window.mouse_x;
		event.y = fb_wayland_window.mouse_y;
	} else {
		event.button = button;
	}
	fb_hPostEvent(&event);
}

static void fb_wayland_post_mouse(int type)
{
	fb_wayland_post_mouse_button(type, BUTTON_LEFT);
}

static int fb_wayland_touch_find(int32_t id)
{
	int i;
	for (i = 0; i < FB_WL_MAX_TOUCH_POINTS; i++) {
		if (fb_wayland_window.touch_points[i].active &&
			fb_wayland_window.touch_points[i].id == id)
			return i;
	}
	return -1;
}

static int fb_wayland_touch_allocate(void)
{
	int i;
	for (i = 0; i < FB_WL_MAX_TOUCH_POINTS; i++) {
		if (!fb_wayland_window.touch_points[i].active)
			return i;
	}
	return -1;
}

static int fb_wayland_touch_count(void)
{
	int i;
	int count = 0;
	for (i = 0; i < FB_WL_MAX_TOUCH_POINTS; i++) {
		if (fb_wayland_window.touch_points[i].active)
			count++;
	}
	return count;
}

static void fb_wayland_touch_down(void *data, struct wl_touch *touch,
	uint32_t serial, uint32_t time, struct wl_surface *surface, int32_t id,
	int32_t x, int32_t y)
{
	FB_WAYLAND_WINDOW *window = &fb_wayland_window;
	int slot;
	(void)data;
	(void)touch;
	(void)serial;
	(void)time;
	if (surface != fb_wayland_window.surface)
		return;
	if (fb_wayland_touch_find(id) >= 0)
		return;
	slot = fb_wayland_touch_allocate();
	if (slot < 0)
		return;
	window->touch_points[slot].active = 1;
	window->touch_points[slot].id = id;
	window->touch_points[slot].x = x / 256;
	window->touch_points[slot].y = y / 256;
	if (!window->touch_active) {
		window->touch_active = 1;
		window->touch_primary_id = id;
		window->mouse_x = window->touch_points[slot].x;
		window->mouse_y = window->touch_points[slot].y;
		window->mouse_buttons = BUTTON_LEFT;
		fb_wayland_post_mouse(EVENT_MOUSE_MOVE);
		fb_wayland_post_mouse(EVENT_MOUSE_BUTTON_PRESS);
	}
}

static void fb_wayland_touch_up(void *data, struct wl_touch *touch,
	uint32_t serial, uint32_t time, int32_t id)
{
	FB_WAYLAND_WINDOW *window = &fb_wayland_window;
	int slot;
	(void)data;
	(void)touch;
	(void)serial;
	(void)time;
	slot = fb_wayland_touch_find(id);
	if (slot < 0)
		return;
	memset(&window->touch_points[slot], 0, sizeof(window->touch_points[slot]));
	if (window->touch_active && window->touch_primary_id == id) {
		fb_wayland_post_mouse(EVENT_MOUSE_BUTTON_RELEASE);
		window->touch_active = 0;
		window->mouse_buttons = 0;
	}
}

static void fb_wayland_touch_motion(void *data, struct wl_touch *touch,
	uint32_t time, int32_t id, int32_t x, int32_t y)
{
	FB_WAYLAND_WINDOW *window = &fb_wayland_window;
	int slot;
	(void)data;
	(void)touch;
	(void)time;
	slot = fb_wayland_touch_find(id);
	if (slot < 0)
		return;
	window->touch_points[slot].x = x / 256;
	window->touch_points[slot].y = y / 256;
	if (!window->touch_active || window->touch_primary_id != id)
		return;
	window->mouse_x = window->touch_points[slot].x;
	window->mouse_y = window->touch_points[slot].y;
	fb_wayland_post_mouse(EVENT_MOUSE_MOVE);
}

static void fb_wayland_touch_frame(void *data, struct wl_touch *touch)
{
	(void)data;
	(void)touch;
}

static void fb_wayland_touch_cancel(void *data, struct wl_touch *touch)
{
	FB_WAYLAND_WINDOW *window = &fb_wayland_window;
	(void)data;
	(void)touch;
	if (window->touch_active)
		fb_wayland_post_mouse(EVENT_MOUSE_BUTTON_RELEASE);
	memset(window->touch_points, 0, sizeof(window->touch_points));
	window->touch_active = 0;
	window->mouse_buttons = 0;
}

static void fb_wayland_touch_shape(void *data, struct wl_touch *touch,
	int32_t id, int32_t major, int32_t minor)
{
	(void)data;
	(void)touch;
	(void)id;
	(void)major;
	(void)minor;
}

static void fb_wayland_touch_orientation(void *data, struct wl_touch *touch,
	int32_t id, int32_t orientation)
{
	(void)data;
	(void)touch;
	(void)id;
	(void)orientation;
}

static const FB_WL_TOUCH_LISTENER fb_wayland_touch_listener = {
	fb_wayland_touch_down,
	fb_wayland_touch_up,
	fb_wayland_touch_motion,
	fb_wayland_touch_frame,
	fb_wayland_touch_cancel,
	fb_wayland_touch_shape,
	fb_wayland_touch_orientation
};

static void fb_wayland_keyboard_release_all(FB_WAYLAND_WINDOW *window)
{
	int scancode;
	uint32_t key;
	window->repeat_key = FB_WL_EVDEV_KEY_MAX;
	for (key = 0; key < FB_WL_EVDEV_KEY_MAX; key++) {
		if (window->evdev_key_down[key]) {
			if (window->xkb_state)
				(void)window->xkb.state_update_key(window->xkb_state,
					key + 8u, 0u);
			window->evdev_key_down[key] = 0;
		}
		window->evdev_scancode[key] = 0;
	}
	if (!fb_gfx)
		return;
	for (scancode = 1; scancode < 128; scancode++) {
		if (fb_gfx->key[scancode]) {
			EVENT event;
			memset(&event, 0, sizeof(event));
			event.type = EVENT_KEY_RELEASE;
			event.scancode = scancode;
			fb_gfx->key[scancode] = FALSE;
			fb_hPostEvent(&event);
		}
	}
}

/* Wayland keyboard key values use the Linux evdev key-code assignments. */
static int fb_wayland_evdev_scancode(uint32_t key)
{
	if ((key >= 2u) && (key <= 11u))
		return (int)(SC_1 + (key - 2u));
	if ((key >= 16u) && (key <= 25u))
		return (int)(SC_Q + (key - 16u));
	if ((key >= 30u) && (key <= 38u))
		return (int)(SC_A + (key - 30u));
	if ((key >= 44u) && (key <= 50u))
		return (int)(SC_Z + (key - 44u));
	if ((key >= 59u) && (key <= 68u))
		return (int)(SC_F1 + (key - 59u));
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

static void fb_wayland_xkb_clear(FB_WAYLAND_WINDOW *window)
{
	if (window->xkb_state && window->xkb.state_unref)
		window->xkb.state_unref(window->xkb_state);
	if (window->xkb_keymap && window->xkb.keymap_unref)
		window->xkb.keymap_unref(window->xkb_keymap);
	if (window->xkb_context && window->xkb.context_unref)
		window->xkb.context_unref(window->xkb_context);
	window->xkb_state = NULL;
	window->xkb_keymap = NULL;
	window->xkb_context = NULL;
}

static void fb_wayland_xkb_unload(FB_WAYLAND_WINDOW *window)
{
	fb_wayland_xkb_clear(window);
	if (window->xkb.library)
		dlclose(window->xkb.library);
	memset(&window->xkb, 0, sizeof(window->xkb));
}

static int fb_wayland_xkb_load(FB_WAYLAND_WINDOW *window)
{
	if (window->xkb.library)
		return 0;
	window->xkb.library = dlopen("libxkbcommon.so.0", RTLD_NOW | RTLD_LOCAL);
	if (!window->xkb.library)
		return -1;
	window->xkb.context_new = dlsym(window->xkb.library, "xkb_context_new");
	window->xkb.context_unref = dlsym(window->xkb.library, "xkb_context_unref");
	window->xkb.keymap_new_from_string = dlsym(window->xkb.library,
		"xkb_keymap_new_from_string");
	window->xkb.keymap_unref = dlsym(window->xkb.library, "xkb_keymap_unref");
	window->xkb.state_new = dlsym(window->xkb.library, "xkb_state_new");
	window->xkb.state_unref = dlsym(window->xkb.library, "xkb_state_unref");
	window->xkb.state_update_key = dlsym(window->xkb.library,
		"xkb_state_update_key");
	window->xkb.state_update_mask = dlsym(window->xkb.library,
		"xkb_state_update_mask");
	window->xkb.state_key_get_utf8 = dlsym(window->xkb.library,
		"xkb_state_key_get_utf8");
	window->xkb.state_key_get_one_sym = dlsym(window->xkb.library,
		"xkb_state_key_get_one_sym");
	if (!window->xkb.context_new || !window->xkb.context_unref ||
		!window->xkb.keymap_new_from_string || !window->xkb.keymap_unref ||
		!window->xkb.state_new || !window->xkb.state_unref ||
		!window->xkb.state_update_key || !window->xkb.state_key_get_utf8) {
		dlclose(window->xkb.library);
		memset(&window->xkb, 0, sizeof(window->xkb));
		return -1;
	}
	return 0;
}

static int fb_wayland_key_repeatable(uint32_t key)
{
	/* Linux evdev codes for modifiers, lock keys, and system keys do not repeat. */
	switch (key) {
		case 29u: case 42u: case 54u: case 56u: case 58u:
		case 97u: case 100u: case 125u: case 126u:
			return 0;
		default:
			return 1;
	}
}

static void fb_wayland_start_key_repeat(FB_WAYLAND_WINDOW *window,
	uint32_t key, int scancode)
{
	struct timespec now;

	if (!window->repeat_interval_ms || !fb_wayland_key_repeatable(key) ||
		clock_gettime(CLOCK_MONOTONIC, &now) != 0)
		return;
	window->repeat_key = key;
	window->repeat_scancode = scancode;
	window->repeat_deadline_ms = (uint64_t)now.tv_sec * 1000u +
		(uint64_t)now.tv_nsec / 1000000u + window->repeat_delay_ms;
}

static uint32_t fb_wayland_first_codepoint(const char *text)
{
	const unsigned char *p = (const unsigned char *)text;
	if (!p || !p[0])
		return 0;
	if (p[0] < 0x80)
		return p[0];
	if ((p[0] >= 0xC2) && (p[0] <= 0xDF) && ((p[1] & 0xC0) == 0x80))
		return ((uint32_t)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
	if ((p[0] >= 0xE0) && (p[0] <= 0xEF) &&
		((p[1] & 0xC0) == 0x80) && ((p[2] & 0xC0) == 0x80) &&
		!((p[0] == 0xE0) && (p[1] < 0xA0)) &&
		!((p[0] == 0xED) && (p[1] >= 0xA0)))
		return ((uint32_t)(p[0] & 0x0F) << 12) |
			((uint32_t)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
	if ((p[0] >= 0xF0) && (p[0] <= 0xF4) &&
		((p[1] & 0xC0) == 0x80) && ((p[2] & 0xC0) == 0x80) &&
		((p[3] & 0xC0) == 0x80) &&
		!((p[0] == 0xF0) && (p[1] < 0x90)) &&
		!((p[0] == 0xF4) && (p[1] >= 0x90)))
		return ((uint32_t)(p[0] & 7) << 18) |
			((uint32_t)(p[1] & 0x3F) << 12) |
			((uint32_t)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
	return 0;
}

static void fb_wayland_keyboard_keymap(void *data, struct wl_keyboard *keyboard,
	uint32_t format, int32_t fd, uint32_t size)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	char *keymap_text;
	size_t allocation_size;
	void *mapping;
	(void)keyboard;
	if ((fd < 0) || (format != 1u) || (size == 0) ||
		(size > 16u * 1024u * 1024u)) {
		if (fd >= 0)
			close(fd);
		return;
	}
	if (fb_wayland_xkb_load(window) != 0) {
		close(fd);
		return;
	}
	mapping = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (mapping == MAP_FAILED)
		return;
	allocation_size = (size_t)size;
	if (allocation_size == SIZE_MAX) {
		munmap(mapping, size);
		return;
	}
	keymap_text = (char *)malloc(allocation_size + 1);
	if (!keymap_text) {
		munmap(mapping, size);
		return;
	}
	memcpy(keymap_text, mapping, allocation_size);
	keymap_text[allocation_size] = '\0';
	munmap(mapping, size);
	fb_wayland_xkb_clear(window);
	window->xkb_context = window->xkb.context_new(0);
	if (window->xkb_context)
		window->xkb_keymap = window->xkb.keymap_new_from_string(
			window->xkb_context, keymap_text, 1, 0);
	free(keymap_text);
	if (window->xkb_keymap)
		window->xkb_state = window->xkb.state_new(window->xkb_keymap);
}


/*
 * Wayland keyboard enter and leave callbacks carry this surface's input-focus
 * transitions. Forward them through gfxlib2's window events so applications
 * can observe when input focus moves between surfaces.
 */
static void fb_wayland_set_keyboard_focus(FB_WAYLAND_WINDOW *window, int focused)
{
	EVENT event;
	focused = (focused != 0);
	if (window->keyboard_focused == focused)
		return;
	window->keyboard_focused = focused;
	if (!fb_gfx)
		return;
	memset(&event, 0, sizeof(event));
	event.type = focused ? EVENT_WINDOW_GOT_FOCUS : EVENT_WINDOW_LOST_FOCUS;
	fb_hPostEvent(&event);
}

static void fb_wayland_keyboard_enter(void *data, struct wl_keyboard *keyboard,
	uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	const uint32_t *key;
	(void)keyboard;
	(void)serial;
	fb_wayland_keyboard_release_all(window);
	fb_wayland_set_keyboard_focus(window, surface == window->surface);
	if (window->keyboard_focused && window->webos.text_ready && window->keyboard_requested)
		(void)fb_wayland_set_keyboard_locked(1);
	if (!window->keyboard_focused || !fb_gfx || !keys || !keys->data ||
		(keys->size > 128u * sizeof(uint32_t)) ||
		(keys->size % sizeof(uint32_t)) != 0)
		return;
	for (key = (const uint32_t *)keys->data;
		(const unsigned char *)key < (const unsigned char *)keys->data + keys->size;
		key++) {
		int scancode = fb_wayland_evdev_scancode(*key);
		if ((scancode > 0) && (scancode < 128))
			fb_gfx->key[scancode] = TRUE;
		if ((*key < FB_WL_EVDEV_KEY_MAX) && window->xkb_state) {
			(void)window->xkb.state_update_key(window->xkb_state,
				*key + 8u, 1u);
			window->evdev_key_down[*key] = 1;
		}
	}
}

static void fb_wayland_keyboard_leave(void *data, struct wl_keyboard *keyboard,
	uint32_t serial, struct wl_surface *surface)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	(void)keyboard;
	(void)serial;
	if (surface == window->surface) {
		if (window->webos.text_ready)
			fb_hWaylandWebosBlur(&window->webos);
		fb_wayland_keyboard_release_all(window);
		fb_wayland_set_keyboard_focus(window, 0);
	}
}

static void fb_wayland_keyboard_key(void *data, struct wl_keyboard *keyboard,
	uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	EVENT event;
	char text[16];
	uint32_t xkb_key;
	uint32_t codepoint = 0;
	uint32_t symbol = 0;
	int scancode;
	int key_value;
	int key_down = (state != 0u);
	(void)keyboard;
	(void)serial;
	(void)time;
	if (!window->keyboard_focused || !fb_gfx)
		return;
	if (key >= FB_WL_EVDEV_KEY_MAX)
		return;
	scancode = fb_wayland_evdev_scancode(key);
	xkb_key = key + 8u;
	if (window->xkb_state) {
		(void)window->xkb.state_update_key(window->xkb_state, xkb_key,
			key_down ? 1u : 0u);
		if (key_down) {
			int length;
			if (window->xkb.state_key_get_one_sym)
				symbol = window->xkb.state_key_get_one_sym(window->xkb_state,
					xkb_key);
			memset(text, 0, sizeof(text));
			length = window->xkb.state_key_get_utf8(window->xkb_state,
				xkb_key, text, sizeof(text));
			if (length > 0 && (size_t)length < sizeof(text))
				codepoint = fb_wayland_first_codepoint(text);
		}
	}
	window->evdev_key_down[key] = key_down ? 1 : 0;
	if (key_down) {
		scancode = fb_hWaylandControlScancode(symbol, scancode);
		if ((scancode > 0) && (scancode < 128))
			window->evdev_scancode[key] = (unsigned char)scancode;
	} else {
		if (window->evdev_scancode[key])
			scancode = window->evdev_scancode[key];
		window->evdev_scancode[key] = 0;
	}
	if ((scancode <= 0) || (scancode >= 128)) {
		/* A virtual keyboard can assign printable XKB text to any evdev code. */
		if (key_down && codepoint) {
			fb_wayland_post_text(text);
			fb_wayland_start_key_repeat(window, key, 0);
		} else if (!key_down && window->repeat_key == key) {
			window->repeat_key = FB_WL_EVDEV_KEY_MAX;
		}
		return;
	}
	/* INKEY reserves values above 255 for extended keys. */
	key_value = codepoint ? ((codepoint <= 0xFFu) ? (int)codepoint : 0) :
		fb_hScancodeToExtendedKey(scancode);
	if (key_down) {
		if (key_value)
			fb_hPostKey(key_value);
		fb_gfx->key[scancode] = TRUE;
		fb_wayland_start_key_repeat(window, key, scancode);
	} else {
		fb_gfx->key[scancode] = FALSE;
		if (window->repeat_key == key)
			window->repeat_key = FB_WL_EVDEV_KEY_MAX;
	}
	memset(&event, 0, sizeof(event));
	event.type = key_down ? EVENT_KEY_PRESS : EVENT_KEY_RELEASE;
	event.scancode = scancode;
	if (codepoint)
		event.ascii = (int)codepoint;
	else if (key_value >= 0 && key_value <= 0xFF)
		event.ascii = key_value;
	fb_hPostEvent(&event);
}

static void fb_wayland_keyboard_modifiers(void *data,
	struct wl_keyboard *keyboard, uint32_t serial, uint32_t depressed,
	uint32_t latched, uint32_t locked, uint32_t group)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	(void)keyboard;
	(void)serial;
	if (window->xkb_state && window->xkb.state_update_mask)
		(void)window->xkb.state_update_mask(window->xkb_state, depressed,
			latched, locked, 0u, 0u, group);
}

static void fb_wayland_keyboard_repeat_info(void *data,
	struct wl_keyboard *keyboard, int32_t rate, int32_t delay)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	(void)keyboard;
	if (rate <= 0 || delay < 0) {
		window->repeat_key = FB_WL_EVDEV_KEY_MAX;
		window->repeat_interval_ms = 0;
		return;
	}
	window->repeat_delay_ms = (uint32_t)delay;
	window->repeat_interval_ms = (uint32_t)(1000 / rate);
	if (!window->repeat_interval_ms)
		window->repeat_interval_ms = 1;
}

static const FB_WL_KEYBOARD_LISTENER fb_wayland_keyboard_listener = {
	fb_wayland_keyboard_keymap,
	fb_wayland_keyboard_enter,
	fb_wayland_keyboard_leave,
	fb_wayland_keyboard_key,
	fb_wayland_keyboard_modifiers,
	fb_wayland_keyboard_repeat_info
};

static void fb_wayland_pointer_set_position(FB_WAYLAND_WINDOW *window,
	int32_t fixed_x, int32_t fixed_y)
{
	int x = fixed_x / 256;
	int y = fixed_y / 256;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (window->width > 0 && x >= window->width)
		x = window->width - 1;
	if (window->height > 0 && y >= window->height)
		y = window->height - 1;
	window->mouse_x = x;
	window->mouse_y = y;
}

static void fb_wayland_post_pointer_motion(FB_WAYLAND_WINDOW *window,
	int old_x, int old_y)
{
	EVENT event;
	memset(&event, 0, sizeof(event));
	event.type = EVENT_MOUSE_MOVE;
	event.x = window->mouse_x;
	event.y = window->mouse_y;
	event.dx = event.x - old_x;
	event.dy = event.y - old_y;
	if (fb_gfx && fb_gfx->scanline_size > 1) {
		event.y /= fb_gfx->scanline_size;
		event.dy /= fb_gfx->scanline_size;
	}
	fb_hPostEvent(&event);
}

static int fb_wayland_map_pointer_button(uint32_t evdev_button)
{
	/* Standard Linux input-event-codes.h values for the three mouse buttons. */
	switch (evdev_button) {
		case 0x110u: return BUTTON_LEFT;
		case 0x111u: return BUTTON_RIGHT;
		case 0x112u: return BUTTON_MIDDLE;
		default: return 0;
	}
}

static void fb_wayland_pointer_enter(void *data, struct wl_pointer *pointer,
	uint32_t serial, struct wl_surface *surface, int32_t fixed_x,
	int32_t fixed_y)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	EVENT event;
	(void)pointer;
	(void)serial;
	if (surface != window->surface)
		return;
	window->pointer_focused = 1;
	fb_wayland_pointer_set_position(window, fixed_x, fixed_y);
	memset(&event, 0, sizeof(event));
	event.type = EVENT_MOUSE_ENTER;
	event.x = window->mouse_x;
	event.y = window->mouse_y;
	fb_hPostEvent(&event);
	/* Wayland can send a button immediately after enter, without motion.
	 * Publish the entry position before that button reaches event consumers. */
	fb_wayland_post_pointer_motion(window, window->mouse_x, window->mouse_y);
}

static void fb_wayland_pointer_leave(void *data, struct wl_pointer *pointer,
	uint32_t serial, struct wl_surface *surface)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	static const int buttons[] = { BUTTON_LEFT, BUTTON_RIGHT, BUTTON_MIDDLE };
	EVENT event;
	int i;
	(void)pointer;
	(void)serial;
	if (surface != window->surface)
		return;
	for (i = 0; i < (int)(sizeof(buttons) / sizeof(buttons[0])); i++) {
		if (window->pointer_buttons & buttons[i]) {
			window->pointer_buttons &= ~buttons[i];
			fb_wayland_post_mouse_button(EVENT_MOUSE_BUTTON_RELEASE,
				buttons[i]);
		}
	}
	window->pointer_focused = 0;
	memset(&event, 0, sizeof(event));
	event.type = EVENT_MOUSE_EXIT;
	event.x = window->mouse_x;
	event.y = window->mouse_y;
	fb_hPostEvent(&event);
}

static void fb_wayland_pointer_motion(void *data, struct wl_pointer *pointer,
	uint32_t time, int32_t fixed_x, int32_t fixed_y)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	int old_x;
	int old_y;
	(void)pointer;
	(void)time;
	if (!window->pointer_focused)
		return;
	old_x = window->mouse_x;
	old_y = window->mouse_y;
	fb_wayland_pointer_set_position(window, fixed_x, fixed_y);
	fb_wayland_post_pointer_motion(window, old_x, old_y);
}

static void fb_wayland_pointer_button(void *data,
	struct wl_pointer *pointer, uint32_t serial, uint32_t time,
	uint32_t evdev_button, uint32_t state)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	int button = fb_wayland_map_pointer_button(evdev_button);
	int pressed = (state != 0u);
	(void)pointer;
	(void)serial;
	(void)time;
	if (!window->pointer_focused || !button)
		return;
	if (pressed) {
		if (window->pointer_buttons & button)
			return;
		window->pointer_buttons |= button;
		fb_wayland_post_mouse_button(EVENT_MOUSE_BUTTON_PRESS, button);
	} else {
		if (!(window->pointer_buttons & button))
			return;
		window->pointer_buttons &= ~button;
		fb_wayland_post_mouse_button(EVENT_MOUSE_BUTTON_RELEASE, button);
	}
}

static void fb_wayland_pointer_axis(void *data, struct wl_pointer *pointer,
	uint32_t time, uint32_t axis, int32_t value)
{
	/* Discrete wheel steps arrive separately in wl_pointer v5. */
	(void)data;
	(void)pointer;
	(void)time;
	(void)axis;
	(void)value;
}

static void fb_wayland_pointer_frame(void *data, struct wl_pointer *pointer)
{
	(void)data;
	(void)pointer;
}

static void fb_wayland_pointer_axis_source(void *data,
	struct wl_pointer *pointer, uint32_t source)
{
	(void)data;
	(void)pointer;
	(void)source;
}

static void fb_wayland_pointer_axis_stop(void *data,
	struct wl_pointer *pointer, uint32_t time, uint32_t axis)
{
	(void)data;
	(void)pointer;
	(void)time;
	(void)axis;
}

static int fb_wayland_saturating_add(int value, int64_t delta)
{
	int64_t result = (int64_t)value + delta;
	if (result > INT_MAX)
		return INT_MAX;
	if (result < INT_MIN)
		return INT_MIN;
	return (int)result;
}

static void fb_wayland_pointer_axis_discrete(void *data,
	struct wl_pointer *pointer, uint32_t axis, int32_t discrete)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	EVENT event;
	(void)pointer;
	if (!window->pointer_focused || discrete == 0)
		return;
	memset(&event, 0, sizeof(event));
	if (axis == 0u) {
		window->mouse_wheel = fb_wayland_saturating_add(
			window->mouse_wheel, -(int64_t)discrete);
		event.type = EVENT_MOUSE_WHEEL;
		event.z = window->mouse_wheel;
	} else if (axis == 1u) {
		window->mouse_hwheel = fb_wayland_saturating_add(
			window->mouse_hwheel, discrete);
		event.type = EVENT_MOUSE_HWHEEL;
		event.w = window->mouse_hwheel;
	} else {
		return;
	}
	event.x = window->mouse_x;
	event.y = window->mouse_y;
	fb_hPostEvent(&event);
}

static const FB_WL_POINTER_LISTENER fb_wayland_pointer_listener = {
	fb_wayland_pointer_enter,
	fb_wayland_pointer_leave,
	fb_wayland_pointer_motion,
	fb_wayland_pointer_button,
	fb_wayland_pointer_axis,
	fb_wayland_pointer_frame,
	fb_wayland_pointer_axis_source,
	fb_wayland_pointer_axis_stop,
	fb_wayland_pointer_axis_discrete
};

static void fb_wayland_seat_capabilities(void *data, struct wl_seat *seat,
	uint32_t capabilities)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	(void)seat;
	if ((capabilities & 1U) && !window->pointer) {
		void *symbols[1];
		uint32_t pointer_version;
		const char *const names[] = { "wl_pointer_interface" };
		if (fb_hWaylandLoadSymbols(names, symbols, 1) == 0 && symbols[0]) {
			/* wl_pointer v5 adds discrete wheel steps used by BASIC mouse events. */
			pointer_version = window->api->proxy_get_version(
				(struct wl_proxy *)window->seat);
			if (pointer_version > 5u)
				pointer_version = 5u;
			window->pointer = (struct wl_pointer *)window->api->
				proxy_marshal_constructor_versioned((struct wl_proxy *)window->seat,
					FB_WL_SEAT_GET_POINTER,
					(const struct wl_interface *)symbols[0],
					pointer_version, NULL);
			if (window->pointer &&
				fb_wayland_add_listener(window->pointer,
					&fb_wayland_pointer_listener, window) != 0) {
				window->api->proxy_destroy((struct wl_proxy *)window->pointer);
				window->pointer = NULL;
			}
		}
	} else if (!(capabilities & 1U) && window->pointer) {
		if (window->pointer_focused)
			fb_wayland_pointer_leave(window, window->pointer, 0,
				window->surface);
		if (window->api->proxy_get_version((struct wl_proxy *)window->pointer) >= 3)
			window->api->proxy_marshal((struct wl_proxy *)window->pointer,
				FB_WL_POINTER_RELEASE);
		window->api->proxy_destroy((struct wl_proxy *)window->pointer);
		window->pointer = NULL;
	}
	if ((capabilities & 2U) && !window->keyboard) {
		void *symbols[1];
		uint32_t keyboard_version;
		const char *const names[] = { "wl_keyboard_interface" };
		if (fb_hWaylandLoadSymbols(names, symbols, 1) == 0 && symbols[0]) {
			/* wl_keyboard v4 adds compositor-configured repeat_info. */
			keyboard_version = window->api->proxy_get_version(
				(struct wl_proxy *)window->seat);
			if (keyboard_version > 4u)
				keyboard_version = 4u;
			window->keyboard = (struct wl_keyboard *)window->api->
				proxy_marshal_constructor_versioned((struct wl_proxy *)window->seat,
					FB_WL_SEAT_GET_KEYBOARD,
					(const struct wl_interface *)symbols[0],
					keyboard_version, NULL);
			if (window->keyboard &&
				fb_wayland_add_listener(window->keyboard,
					&fb_wayland_keyboard_listener, window) != 0) {
				window->api->proxy_destroy((struct wl_proxy *)window->keyboard);
				window->keyboard = NULL;
			}
		}
	} else if (!(capabilities & 2U) && window->keyboard) {
		fb_wayland_keyboard_release_all(window);
		fb_wayland_set_keyboard_focus(window, 0);
		if (window->api->proxy_get_version((struct wl_proxy *)window->keyboard) >= 3)
			window->api->proxy_marshal((struct wl_proxy *)window->keyboard,
				FB_WL_KEYBOARD_RELEASE);
		window->api->proxy_destroy((struct wl_proxy *)window->keyboard);
		window->keyboard = NULL;
		fb_wayland_xkb_clear(window);
	}
	if ((capabilities & 4U) && !window->touch) {
		void *symbols[1];
		const char *const names[] = { "wl_touch_interface" };
		if (fb_hWaylandLoadSymbols(names, symbols, 1) == 0 && symbols[0]) {
			window->touch = (struct wl_touch *)window->api->
				proxy_marshal_constructor_versioned((struct wl_proxy *)window->seat,
					FB_WL_SEAT_GET_TOUCH,
					(const struct wl_interface *)symbols[0], 1, NULL);
			if (window->touch)
				(void)fb_wayland_add_listener(window->touch,
					&fb_wayland_touch_listener, window);
		}
	}
}

static void fb_wayland_seat_name(void *data, struct wl_seat *seat,
	const char *name)
{
	(void)data;
	(void)seat;
	(void)name;
}

static const FB_WL_SEAT_LISTENER fb_wayland_seat_listener = {
	fb_wayland_seat_capabilities,
	fb_wayland_seat_name
};

static void fb_wayland_text_enter(void *data, struct zwp_text_input_v3 *input,
	struct wl_surface *surface)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	(void)input;
	if (surface == window->surface) {
		window->text_input_entered = 1;
		if (window->keyboard_requested) {
			/*
			 * Some compositors retain stale input-method state after a
			 * surface switch. Commit a disable before restoring focus so
			 * the keyboard receives a fresh activation.
			 */
			(void)fb_wayland_set_keyboard_locked(0);
			fb_wayland_set_keyboard_locked(1);
		}
	}
}

static void fb_wayland_text_leave(void *data, struct zwp_text_input_v3 *input,
	struct wl_surface *surface)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	(void)input;
	if (surface == window->surface) {
		fb_hWaylandTextV3Reset(&window->text_v3_pending);
		window->text_input_entered = 0;
		window->sent_region_valid = 0;
	}
}

static void fb_wayland_text_preedit(void *data, struct zwp_text_input_v3 *input,
	const char *text, int32_t cursor_begin, int32_t cursor_end)
{
	(void)data;
	(void)input;
	(void)text;
	(void)cursor_begin;
	(void)cursor_end;
}

static void fb_wayland_post_text(const char *text)
{
	const unsigned char *p = (const unsigned char *)text;
	int posted = 0;
	if (!p)
		return;
	while (*p && posted < 255) {
		uint32_t value;
		size_t length;
		if (*p < 0x80) {
			value = *p;
			length = 1;
		} else if ((*p >= 0xC2) && (*p <= 0xDF) &&
			((p[1] & 0xC0) == 0x80)) {
			value = ((uint32_t)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
			length = 2;
		} else if ((*p >= 0xE0) && (*p <= 0xEF) &&
			((p[1] & 0xC0) == 0x80) && ((p[2] & 0xC0) == 0x80) &&
			!((*p == 0xE0) && (p[1] < 0xA0)) &&
			!((*p == 0xED) && (p[1] >= 0xA0))) {
			value = ((uint32_t)(p[0] & 0x0F) << 12) |
				((uint32_t)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
			length = 3;
		} else if ((*p >= 0xF0) && (*p <= 0xF4) &&
			((p[1] & 0xC0) == 0x80) && ((p[2] & 0xC0) == 0x80) &&
			((p[3] & 0xC0) == 0x80) &&
			!((*p == 0xF0) && (p[1] < 0x90)) &&
			!((*p == 0xF4) && (p[1] >= 0x90))) {
			value = ((uint32_t)(p[0] & 7) << 18) |
				((uint32_t)(p[1] & 0x3F) << 12) |
				((uint32_t)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
			length = 4;
		} else {
			p++;
			continue;
		}
		if ((value > 0) && (value <= 0x10FFFF) &&
			!(value >= 0xD800 && value <= 0xDFFF)) {
			EVENT event;
			int legacy_key = (value == '\n') ? '\r' : (int)value;
			/*
			 * Text-input commits are not wl_keyboard key events. Publish
			 * byte-sized characters to INKEY as well as the event queue;
			 * larger Unicode values are not legacy extended key codes.
			 * Protocol callbacks run with the driver lock held.
			 */
			if (legacy_key <= 0xFF)
				fb_hPostKey(legacy_key);
			memset(&event, 0, sizeof(event));
			event.type = EVENT_KEY_PRESS;
			event.ascii = legacy_key;
			if (legacy_key == '\r')
				event.scancode = SC_ENTER;
			else if (legacy_key == '\b')
				event.scancode = SC_BACKSPACE;
			fb_hPostEvent(&event);
			event.type = EVENT_KEY_RELEASE;
			fb_hPostEvent(&event);
			posted++;
		}
		p += length;
	}
}

static void fb_wayland_text_commit(void *data, struct zwp_text_input_v3 *input,
	const char *text)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	(void)input;
	if (window->text_input_entered && window->keyboard_requested)
		fb_hWaylandTextV3SetCommit(&window->text_v3_pending, text);
}

static void fb_wayland_post_delete(uint32_t before)
{
	uint32_t count = before;
	EVENT event;
	if (count > 64)
		count = 64;
	memset(&event, 0, sizeof(event));
	event.type = EVENT_KEY_PRESS;
	event.scancode = SC_BACKSPACE;
	while (count--) {
		fb_hPostKey('\b');
		fb_hPostEvent(&event);
		event.type = EVENT_KEY_RELEASE;
		fb_hPostEvent(&event);
		event.type = EVENT_KEY_PRESS;
	}
}

static void fb_wayland_text_delete(void *data, struct zwp_text_input_v3 *input,
	uint32_t before, uint32_t after)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	(void)input;
	if (window->text_input_entered && window->keyboard_requested)
		fb_hWaylandTextV3SetDelete(&window->text_v3_pending, before, after);
}

static void fb_wayland_text_done(void *data, struct zwp_text_input_v3 *input,
	uint32_t serial)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	(void)input;
	(void)serial;
	/*
	 * v3 applies deletion before insertion regardless of event order. The
	 * legacy input API has no surrounding text, so the existing before-cursor
	 * backspace fallback cannot exactly represent UTF-8 or after-cursor edits.
	 */
	if (window->text_input_entered && window->keyboard_requested) {
		fb_wayland_post_delete(window->text_v3_pending.before);
		fb_wayland_post_text(window->text_v3_pending.text);
	}
	fb_hWaylandTextV3Reset(&window->text_v3_pending);
}

static const FB_WL_TEXT_INPUT_LISTENER fb_wayland_text_input_listener = {
	fb_wayland_text_enter,
	fb_wayland_text_leave,
	fb_wayland_text_preedit,
	fb_wayland_text_commit,
	fb_wayland_text_delete,
	fb_wayland_text_done
};

static void fb_wayland_text_v2_enter(void *data, struct zwp_text_input_v2 *input,
	uint32_t serial, struct wl_surface *surface)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	(void)input;
	if (surface != window->surface)
		return;
	window->text_input_entered = 1;
	window->text_input_serial = serial;
	window->keyboard_visible = -2;
	if (window->keyboard_requested)
		(void)fb_wayland_set_keyboard_locked(1);
}

static void fb_wayland_text_v2_leave(void *data, struct zwp_text_input_v2 *input,
	uint32_t serial, struct wl_surface *surface)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	(void)input;
	(void)serial;
	if (surface == window->surface) {
		window->text_input_entered = 0;
		window->sent_region_valid = 0;
		window->keyboard_visible = 0;
	}
}

static void fb_wayland_text_v2_panel_state(void *data,
	struct zwp_text_input_v2 *input, uint32_t state, int32_t x, int32_t y,
	int32_t width, int32_t height)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	(void)input;
	window->keyboard_visible = state == 1 ? -1 : 0;
	/* Keep the compositor's geometry, including negative origins. Clip it
	 * against the current drawable when queried, after queued resizes apply.
	 * Callbacks and public queries run under fb_wayland_mutex. */
	window->panel_x = x;
	window->panel_y = y;
	window->panel_width = window->keyboard_visible && width > 0 ? width : 0;
	window->panel_height = window->keyboard_visible && height > 0 ? height : 0;
}

static void fb_wayland_text_v2_preedit(void *data,
	struct zwp_text_input_v2 *input, const char *text, const char *commit)
{
	/* The v2 fallback commit is not a commit_string event. This backend
	 * does not display preedit, so wait for the actual committed text. */
	(void)data;
	(void)input;
	(void)text;
	(void)commit;
}

static void fb_wayland_text_v2_preedit_styling(void *data,
	struct zwp_text_input_v2 *input, uint32_t index, uint32_t length,
	uint32_t style)
{
	(void)data;
	(void)input;
	(void)index;
	(void)length;
	(void)style;
}

static void fb_wayland_text_v2_preedit_cursor(void *data,
	struct zwp_text_input_v2 *input, int32_t index)
{
	(void)data;
	(void)input;
	(void)index;
}

static void fb_wayland_text_v2_commit(void *data,
	struct zwp_text_input_v2 *input, const char *text)
{
	(void)data;
	(void)input;
	fb_wayland_post_text(text);
}

static void fb_wayland_text_v2_cursor_position(void *data,
	struct zwp_text_input_v2 *input, int32_t index, int32_t anchor)
{
	(void)data;
	(void)input;
	(void)index;
	(void)anchor;
}

static void fb_wayland_text_v2_delete(void *data,
	struct zwp_text_input_v2 *input, uint32_t before, uint32_t after)
{
	(void)data;
	(void)input;
	(void)after;
	fb_wayland_post_delete(before);
}

static void fb_wayland_text_v2_modifiers_map(void *data,
	struct zwp_text_input_v2 *input, struct wl_array *map)
{
	(void)data;
	(void)input;
	(void)map;
}

static void fb_wayland_text_v2_keysym(void *data,
	struct zwp_text_input_v2 *input, uint32_t time, uint32_t sym,
	uint32_t state, uint32_t modifiers)
{
	(void)data;
	(void)input;
	(void)time;
	(void)sym;
	(void)state;
	(void)modifiers;
}

/* webOS uses the same graphics input queues as the standard text protocols. */
static void fb_wayland_webos_text(void *data, const char *text)
{
	(void)data;
	fb_wayland_post_text(text);
}

static void fb_wayland_webos_delete(void *data, uint32_t count)
{
	(void)data;
	fb_wayland_post_delete(count);
}

static void fb_wayland_webos_key(void *data, uint32_t symbol, uint32_t state)
{
	EVENT event;
	int scancode, key;
	(void)data;
	if (!state)
		return;
	/* Printable keys arrive through commit_string, not a second keysym path. */
	switch (symbol) {
	/* LuneOS Maliit sends Qt key values; other implementations use XKB. */
	case 0x1000003: case 0xff08: scancode = SC_BACKSPACE; key = '\b'; break;
	case 0x1000001: case 0xff09: scancode = SC_TAB; key = '\t'; break;
	case 0x1000004: case 0x1000005: case 0xff0d: case 0xff8d:
		scancode = SC_ENTER; key = '\r'; break;
	case 0x1000000: case 0xff1b: scancode = SC_ESCAPE; key = 27; break;
	case 0x1000010: case 0xff50: scancode = SC_HOME; key = (SC_HOME << 8) | 0xff; break;
	case 0x1000012: case 0xff51: scancode = SC_LEFT; key = (SC_LEFT << 8) | 0xff; break;
	case 0x1000013: case 0xff52: scancode = SC_UP; key = (SC_UP << 8) | 0xff; break;
	case 0x1000014: case 0xff53: scancode = SC_RIGHT; key = (SC_RIGHT << 8) | 0xff; break;
	case 0x1000015: case 0xff54: scancode = SC_DOWN; key = (SC_DOWN << 8) | 0xff; break;
	case 0x1000011: case 0xff57: scancode = SC_END; key = (SC_END << 8) | 0xff; break;
	case 0x1000007: case 0xffff: scancode = SC_DELETE; key = (SC_DELETE << 8) | 0xff; break;
	default: return;
	}
	fb_hPostKey(key);
	memset(&event, 0, sizeof(event));
	event.type = EVENT_KEY_PRESS;
	event.scancode = scancode;
	event.ascii = key <= 0xff ? key : 0;
	fb_hPostEvent(&event);
	event.type = EVENT_KEY_RELEASE;
	fb_hPostEvent(&event);
}

static void fb_wayland_webos_resize(void *data, int width, int height)
{
	fb_wayland_shell_surface_configure(data, NULL, 0, width, height);
}

static void fb_wayland_webos_close(void *data)
{
	fb_wayland_toplevel_close(data, NULL);
}

static const FB_WAYLAND_WEBOS_CALLBACKS fb_wayland_webos_callbacks = {
	fb_wayland_webos_text, fb_wayland_webos_key, fb_wayland_webos_delete,
	fb_wayland_webos_resize, fb_wayland_webos_close
};

static void fb_wayland_text_v2_language(void *data,
	struct zwp_text_input_v2 *input, const char *language)
{
	(void)data;
	(void)input;
	(void)language;
}

static void fb_wayland_text_v2_direction(void *data,
	struct zwp_text_input_v2 *input, uint32_t direction)
{
	(void)data;
	(void)input;
	(void)direction;
}

static void fb_wayland_text_v2_configure_surrounding(void *data,
	struct zwp_text_input_v2 *input, int32_t before_cursor,
	int32_t after_cursor)
{
	(void)data;
	(void)input;
	(void)before_cursor;
	(void)after_cursor;
}

static void fb_wayland_text_v2_input_method_changed(void *data,
	struct zwp_text_input_v2 *input, uint32_t serial, uint32_t flags)
{
	FB_WAYLAND_WINDOW *window = (FB_WAYLAND_WINDOW *)data;
	(void)input;
	(void)flags;
	window->text_input_serial = serial;
	window->keyboard_visible = -2;
	if (window->keyboard_requested)
		(void)fb_wayland_set_keyboard_locked(1);
}

static const FB_WL_TEXT_INPUT_V2_LISTENER fb_wayland_text_input_v2_listener = {
	fb_wayland_text_v2_enter,
	fb_wayland_text_v2_leave,
	fb_wayland_text_v2_panel_state,
	fb_wayland_text_v2_preedit,
	fb_wayland_text_v2_preedit_styling,
	fb_wayland_text_v2_preedit_cursor,
	fb_wayland_text_v2_commit,
	fb_wayland_text_v2_cursor_position,
	fb_wayland_text_v2_delete,
	fb_wayland_text_v2_modifiers_map,
	fb_wayland_text_v2_keysym,
	fb_wayland_text_v2_language,
	fb_wayland_text_v2_direction,
	fb_wayland_text_v2_configure_surrounding,
	fb_wayland_text_v2_input_method_changed
};

static int fb_wayland_create_buffer(FB_WAYLAND_BUFFER *slot, int width, int height)
{
	char name[64];
	char *dirty;
	void *pixels;
	struct wl_buffer *buffer;
	struct wl_shm_pool *pool;
	const struct wl_interface *pool_interface;
	const struct wl_interface *buffer_interface;
	void *symbols[2];
	const char *const names[] = { "wl_shm_pool_interface", "wl_buffer_interface" };
	uint64_t bytes;
	uint64_t total_bytes = 0;
	int fd;
	int i;
	int name_length;
	static unsigned int sequence;
	if (!slot || slot->buffer || slot->pixels || slot->dirty ||
		(width <= 0) || (height <= 0) ||
		(uint64_t)width * (uint64_t)height > FB_WL_MAX_BUFFER_BYTES / 4U)
		return -1;
	bytes = (uint64_t)width * (uint64_t)height * 4U;
	if (bytes > FB_WL_MAX_BUFFER_BYTES || bytes > (uint64_t)SIZE_MAX)
		return -1;
	for (i = 0; i < FB_WL_BUFFER_SLOTS; i++)
		total_bytes += fb_wayland_window.buffers[i].size;
	if (total_bytes + bytes > FB_WL_MAX_TOTAL_BUFFER_BYTES)
		return -1;
	if (fb_hWaylandLoadSymbols(names, symbols, 2) != 0 ||
		!symbols[0] || !symbols[1])
		return -1;
	dirty = (char *)malloc((size_t)height);
	if (!dirty)
		return -1;
	memset(dirty, TRUE, (size_t)height);
	pool_interface = (const struct wl_interface *)symbols[0];
	buffer_interface = (const struct wl_interface *)symbols[1];
	sequence++;
	name_length = snprintf(name, sizeof(name), "/fbgfx-wayland-%ld-%u",
		(long)getpid(), sequence);
	if (name_length < 0 || (size_t)name_length >= sizeof(name))
		goto fail_dirty;
	fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
	if (fd < 0)
		goto fail_dirty;
	(void)shm_unlink(name);
	if (ftruncate(fd, (off_t)bytes) != 0) {
		close(fd);
		goto fail_dirty;
	}
	pixels = mmap(NULL, (size_t)bytes,
		PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (pixels == MAP_FAILED) {
		close(fd);
		goto fail_dirty;
	}
	/* Constructor arguments follow the protocol signature, including new-id slots. */
	pool = (struct wl_shm_pool *)fb_wayland_window.api->
		proxy_marshal_constructor_versioned((struct wl_proxy *)fb_wayland_window.shm,
		FB_WL_SHM_CREATE_POOL, pool_interface, 1, NULL, fd, (int32_t)bytes);
	close(fd);
	if (!pool)
		goto fail_map;
	buffer = (struct wl_buffer *)fb_wayland_window.api->
		proxy_marshal_constructor_versioned((struct wl_proxy *)pool,
		FB_WL_SHM_POOL_CREATE_BUFFER, buffer_interface, 1,
		NULL, 0, width, height, width * 4, FB_WL_FORMAT_XRGB8888);
	fb_wayland_window.api->proxy_marshal((struct wl_proxy *)pool,
		FB_WL_SHM_POOL_DESTROY);
	fb_wayland_destroy(pool);
	if (!buffer)
		goto fail_map;
	if (fb_wayland_add_listener(buffer,
		&fb_wayland_buffer_listener, &fb_wayland_window) != 0)
		goto fail_buffer;
	slot->buffer = buffer;
	slot->pixels = pixels;
	slot->dirty = dirty;
	slot->size = (size_t)bytes;
	slot->width = width;
	slot->height = height;
	return 0;

fail_buffer:
	fb_wayland_destroy_buffer(buffer);
fail_map:
	munmap(pixels, (size_t)bytes);
fail_dirty:
	free(dirty);
	return -1;
}

static void fb_wayland_request_toplevel_state(FB_WAYLAND_WINDOW *window,
	int flags)
{
	if (!window || !window->api || !window->api->proxy_marshal)
		return;
	if (window->toplevel && (flags & DRIVER_FULLSCREEN)) {
		window->api->proxy_marshal((struct wl_proxy *)window->toplevel,
			FB_WL_XDG_TOPLEVEL_SET_FULLSCREEN, NULL);
	} else if (window->toplevel && (flags & DRIVER_RESIZABLE) &&
	    fb_hLinuxDeviceClass() == FB_GFX_DEVICE_CLASS_HANDHELD) {
		/* Maximized windows follow the compositor's usable area, including
		 * rotation and the keyboard, without hiding the mobile shell. */
		window->api->proxy_marshal((struct wl_proxy *)window->toplevel,
			FB_WL_XDG_TOPLEVEL_SET_MAXIMIZED);
	} else if (window->shell_surface && (flags & DRIVER_FULLSCREEN)) {
		window->api->proxy_marshal(
			(struct wl_proxy *)window->shell_surface,
			FB_WL_SHELL_SURFACE_SET_FULLSCREEN,
			(uint32_t)FB_WL_SHELL_FULLSCREEN_METHOD_DEFAULT,
			(uint32_t)0, (struct wl_proxy *)NULL);
	} else if (window->shell_surface && (flags & DRIVER_RESIZABLE) &&
	    fb_hLinuxDeviceClass() == FB_GFX_DEVICE_CLASS_HANDHELD) {
		window->api->proxy_marshal(
			(struct wl_proxy *)window->shell_surface,
			FB_WL_SHELL_SURFACE_SET_MAXIMIZED, (struct wl_proxy *)NULL);
	} else if (window->shell_surface) {
		window->api->proxy_marshal(
			(struct wl_proxy *)window->shell_surface,
			FB_WL_SHELL_SURFACE_SET_TOPLEVEL);
	}
}

static int fb_wayland_driver_init(char *title, int width, int height,
	int depth, int refresh, int flags)
{
	const struct wl_interface *registry_interface;
	void *symbols[3];
	void *shell_symbols[2] = { NULL, NULL };
	const char *const names[] = {
		"wl_registry_interface", "wl_surface_interface", "wl_callback_interface"
	};
	const char *const shell_names[] = {
		"wl_shell_interface", "wl_shell_surface_interface"
	};
	char app_id[FB_WAYLAND_APP_ID_CAPACITY];
	int probe_locked = 0;
	int i;
	(void)refresh;
	if ((flags & DRIVER_OPENGL) || width <= 0 || height <= 0)
		return -1;
	if (!fb_hWaylandShouldUse())
		return -1;
	if (!fb_wayland_prepare_mutex())
		return -1;
	pthread_mutex_lock(&fb_wayland_mutex);
	memset(&fb_wayland_window, 0, sizeof(fb_wayland_window));
	fb_wayland_thread_failed = 0;
	fb_wayland_window.flags = flags;
	fb_wayland_window.keyboard_visible = -2;
	if (fb_gfx)
		fb_gfx->flags |= DRIVER_RESIZABLE;
	if (fb_hWaylandGetApi(&fb_wayland_window.api) != 0 ||
		fb_hWaylandInitProtocols() != 0 ||
		fb_hWaylandLoadSymbols(names, symbols, 3) != 0 ||
		!symbols[0] || !symbols[1] || !symbols[2])
		goto fail;
	if (!fb_wayland_window.api->proxy_marshal ||
		!fb_wayland_window.api->proxy_marshal_constructor_versioned ||
		!fb_wayland_window.api->proxy_get_version ||
		!fb_wayland_window.api->proxy_add_listener ||
		!fb_wayland_window.api->proxy_destroy ||
		!fb_wayland_window.api->display_roundtrip ||
		!fb_wayland_window.api->display_flush)
		goto fail;
	registry_interface = (const struct wl_interface *)symbols[0];
	fb_wayland_window.surface_interface =
		(const struct wl_interface *)symbols[1];
	fb_wayland_window.callback_interface =
		(const struct wl_interface *)symbols[2];
	if (fb_hWaylandLoadSymbols(shell_names, shell_symbols, 2) == 0 &&
		shell_symbols[0] && shell_symbols[1]) {
		fb_wayland_window.shell_interface =
			(const struct wl_interface *)shell_symbols[0];
		fb_wayland_window.shell_surface_interface =
			(const struct wl_interface *)shell_symbols[1];
	}
	if (fb_hWaylandDisplayBeginWindow(&fb_wayland_window.display) != 0)
		goto fail;
	probe_locked = 1;
	if (!fb_wayland_window.display)
		fb_wayland_window.display = fb_wayland_window.api->display_connect(NULL);
	if (!fb_wayland_window.display)
		goto fail;
	fb_wayland_window.registry = (struct wl_registry *)
		fb_wayland_window.api->proxy_marshal_constructor_versioned(
			(struct wl_proxy *)fb_wayland_window.display,
			FB_WL_DISPLAY_GET_REGISTRY, registry_interface, 1, NULL);
	if (!fb_wayland_window.registry ||
		fb_wayland_add_listener(fb_wayland_window.registry,
			&fb_wayland_registry_listener, &fb_wayland_window) != 0 ||
		fb_wayland_window.api->display_roundtrip(fb_wayland_window.display) < 0)
		goto fail;
	if (!fb_wayland_window.compositor || !fb_wayland_window.shm ||
		(!fb_wayland_window.wm_base && !fb_wayland_window.shell))
		goto fail;
	/* A Wayland compositor can provide display output without an input seat.
	   Keep graphics usable in that case and leave pointer, touch, and keyboard
	   requests unavailable until the compositor provides those capabilities. */
	if (fb_wayland_window.seat &&
		fb_wayland_add_listener(fb_wayland_window.seat,
			&fb_wayland_seat_listener, &fb_wayland_window) != 0)
		goto fail;
	if (fb_wayland_window.wm_base &&
		fb_wayland_add_listener(fb_wayland_window.wm_base,
			&fb_wayland_wm_base_listener, &fb_wayland_window) != 0)
		goto fail;
	fb_wayland_window.surface = (struct wl_surface *)
		fb_wayland_window.api->proxy_marshal_constructor_versioned(
			(struct wl_proxy *)fb_wayland_window.compositor,
			FB_WL_COMPOSITOR_CREATE_SURFACE,
			fb_wayland_window.surface_interface,
			fb_wayland_window.api->proxy_get_version(
				(struct wl_proxy *)fb_wayland_window.compositor), NULL);
	if (!fb_wayland_window.surface)
		goto fail;
	if (fb_wayland_window.wm_base) {
		fb_wayland_window.xdg_surface = (struct xdg_surface *)
			fb_wayland_window.api->proxy_marshal_constructor_versioned(
				(struct wl_proxy *)fb_wayland_window.wm_base,
				FB_WL_XDG_WM_BASE_GET_XDG_SURFACE, &xdg_surface_interface, 1,
				NULL, (struct wl_proxy *)fb_wayland_window.surface);
		if (!fb_wayland_window.xdg_surface ||
			fb_wayland_add_listener(fb_wayland_window.xdg_surface,
				&fb_wayland_xdg_surface_listener, &fb_wayland_window) != 0)
			goto fail;
		fb_wayland_window.toplevel = (struct xdg_toplevel *)
			fb_wayland_window.api->proxy_marshal_constructor_versioned(
				(struct wl_proxy *)fb_wayland_window.xdg_surface,
				FB_WL_XDG_SURFACE_GET_TOPLEVEL, &xdg_toplevel_interface, 1, NULL);
		if (!fb_wayland_window.toplevel ||
			fb_wayland_add_listener(fb_wayland_window.toplevel,
				&fb_wayland_toplevel_listener, &fb_wayland_window) != 0)
			goto fail;
		fb_wayland_window.api->proxy_marshal(
			(struct wl_proxy *)fb_wayland_window.toplevel,
			FB_WL_XDG_TOPLEVEL_SET_TITLE, title ? title : "FreeBASIC");
		fb_wayland_window.api->proxy_marshal(
			(struct wl_proxy *)fb_wayland_window.toplevel,
			FB_WL_XDG_TOPLEVEL_SET_APP_ID,
			fb_hWaylandAppId(app_id, sizeof(app_id), "freebasic"));
		fb_wayland_request_toplevel_state(&fb_wayland_window, flags);
	} else {
		fb_wayland_window.shell_surface = (struct wl_shell_surface *)
			fb_wayland_window.api->proxy_marshal_constructor_versioned(
				(struct wl_proxy *)fb_wayland_window.shell,
				FB_WL_WL_SHELL_GET_SHELL_SURFACE,
				fb_wayland_window.shell_surface_interface, 1, NULL,
				(struct wl_proxy *)fb_wayland_window.surface);
		if (!fb_wayland_window.shell_surface ||
			fb_wayland_add_listener(fb_wayland_window.shell_surface,
				&fb_wayland_shell_surface_listener,
				&fb_wayland_window) != 0)
			goto fail;
		fb_wayland_window.api->proxy_marshal(
			(struct wl_proxy *)fb_wayland_window.shell_surface,
			FB_WL_SHELL_SURFACE_SET_TITLE, title ? title : "FreeBASIC");
		fb_wayland_window.api->proxy_marshal(
			(struct wl_proxy *)fb_wayland_window.shell_surface,
			FB_WL_SHELL_SURFACE_SET_CLASS,
			fb_hWaylandAppId(app_id, sizeof(app_id), "freebasic"));
		fb_wayland_request_toplevel_state(&fb_wayland_window, flags);
		/* wl_shell has no initial configure handshake like xdg-shell. */
		fb_wayland_window.configured = 1;
	}
	/* wl_shm XRGB8888 uses the same red and blue bit positions as gfxlib. */
	fb_hWaylandWebosOpen(&fb_wayland_window.webos,
		(struct wl_proxy *)fb_wayland_window.seat,
		(struct wl_proxy *)fb_wayland_window.surface,
		fb_hWaylandAppId(app_id, sizeof(app_id), "freebasic"),
		&fb_wayland_webos_callbacks, &fb_wayland_window,
		!fb_wayland_window.text_input_manager && !fb_wayland_window.text_input_manager_v2);
	fb_wayland_window.blitter = fb_hGetBlitter(32, FALSE);
	if (!fb_wayland_window.blitter ||
	    fb_wayland_create_buffer(&fb_wayland_window.buffers[0], width, height) != 0)
		goto fail;
	fb_wayland_window.width = width;
	fb_wayland_window.height = height;
	fb_wayland_window.stride = width * 4;
	fb_wayland_window.api->proxy_marshal((struct wl_proxy *)fb_wayland_window.surface,
		FB_WL_SURFACE_SET_BUFFER_SCALE, 1);
	fb_wayland_window.api->proxy_marshal((struct wl_proxy *)fb_wayland_window.surface,
		FB_WL_SURFACE_COMMIT);
	if (fb_wayland_window.api->display_roundtrip(fb_wayland_window.display) < 0)
		goto fail;
	fb_wayland_window.active = 1;
	/* Prefer v2 when advertised: its input_panel_state includes real overlap
	 * bounds. KWin can change from resizing to overlaying during rotation;
	 * v3 alone cannot describe that change. Keep v3 for compositors without v2. */
	if (fb_wayland_window.seat && fb_wayland_window.text_input_manager_v2) {
		fb_wayland_window.text_input_v2 = (struct zwp_text_input_v2 *)
			fb_wayland_window.api->proxy_marshal_constructor_versioned(
				(struct wl_proxy *)fb_wayland_window.text_input_manager_v2,
				FB_WL_TEXT_INPUT_MANAGER_V2_GET_TEXT_INPUT,
				&zwp_text_input_v2_interface, 1, NULL,
				(struct wl_proxy *)fb_wayland_window.seat);
		if (fb_wayland_window.text_input_v2 &&
			fb_wayland_add_listener(fb_wayland_window.text_input_v2,
				&fb_wayland_text_input_v2_listener,
				&fb_wayland_window) != 0)
			goto fail;
	} else if (fb_wayland_window.seat && fb_wayland_window.text_input_manager) {
		fb_wayland_window.text_input = (struct zwp_text_input_v3 *)
			fb_wayland_window.api->proxy_marshal_constructor_versioned(
				(struct wl_proxy *)fb_wayland_window.text_input_manager,
				FB_WL_TEXT_INPUT_MANAGER_GET_TEXT_INPUT,
				&zwp_text_input_v3_interface, 1, NULL,
				(struct wl_proxy *)fb_wayland_window.seat);
		if (fb_wayland_window.text_input)
			(void)fb_wayland_add_listener(fb_wayland_window.text_input,
				&fb_wayland_text_input_listener, &fb_wayland_window);
	}
	if (fb_wayland_window.api->display_roundtrip(fb_wayland_window.display) < 0)
		goto fail;
	(void)depth;
	fb_hWaylandDisplayEndWindow();
	pthread_mutex_unlock(&fb_wayland_mutex);
	return 0;

fail:
	fb_wayland_window.active = 0;
	fb_hWaylandWebosDestroy(&fb_wayland_window.webos);
	for (i = 0; i < FB_WL_BUFFER_SLOTS; i++)
		fb_wayland_destroy_slot(&fb_wayland_window.buffers[i]);
	fb_wayland_xkb_unload(&fb_wayland_window);
	if (fb_wayland_window.display)
		fb_wayland_window.api->display_disconnect(fb_wayland_window.display);
	memset(&fb_wayland_window, 0, sizeof(fb_wayland_window));
	if (probe_locked)
		fb_hWaylandDisplayEndWindow();
	pthread_mutex_unlock(&fb_wayland_mutex);
	return -1;
}

/* ------------------------------------------------------------------------- */
/* GFXDRIVER lifecycle and presentation callbacks                            */
/* ------------------------------------------------------------------------- */

static void fb_wayland_driver_exit(void)
{
	FB_WAYLAND_WINDOW *window = &fb_wayland_window;
	int join_thread;
	int i;

	fb_hLinuxMaliitCancelPending();
	if (!fb_wayland_prepare_mutex())
		return;
	pthread_mutex_lock(&fb_wayland_mutex);
	join_thread = fb_wayland_thread_started;
	fb_wayland_thread_running = 0;
	window->active = 0;
	pthread_mutex_unlock(&fb_wayland_mutex);
	if (join_thread)
		pthread_join(fb_wayland_thread, NULL);

	pthread_mutex_lock(&fb_wayland_mutex);
	fb_wayland_keyboard_release_all(window);
	if (window->pointer && window->pointer_focused)
		fb_wayland_pointer_leave(window, window->pointer, 0, window->surface);
	if (window->touch_active)
		fb_wayland_touch_cancel(window, window->touch);
	fb_wayland_xkb_unload(window);
	if (window->frame_callback)
		fb_wayland_destroy(window->frame_callback);
	fb_hWaylandWebosDestroy(&window->webos);
	fb_wayland_qt_windowmanager_destroy(window);
	for (i = 0; i < FB_WL_BUFFER_SLOTS; i++)
		fb_wayland_destroy_slot(&window->buffers[i]);
	if (window->display && window->api)
		window->api->display_disconnect(window->display);
	memset(window, 0, sizeof(*window));
	fb_wayland_thread_started = 0;
	fb_wayland_thread_failed = 0;
	pthread_mutex_unlock(&fb_wayland_mutex);
}

static void fb_wayland_driver_lock(void)
{
	pthread_mutex_lock(&fb_wayland_mutex);
}

static void fb_wayland_driver_unlock(void)
{
	/* Preserve drawing when a system cannot create the presentation thread. */
	if (fb_wayland_thread_failed && fb_wayland_window.active) {
		fb_wayland_window.repaint_pending = 1;
		fb_wayland_poll_events_locked();
	}
	pthread_mutex_unlock(&fb_wayland_mutex);
}

static int fb_wayland_has_dirty_lines(void)
{
	int y;

	if (!fb_gfx || !fb_gfx->dirty || fb_gfx->h <= 0)
		return 0;
	for (y = 0; y < fb_gfx->h; y++) {
		if (fb_gfx->dirty[y])
			return 1;
	}
	return 0;
}

/* Each reusable buffer must receive every row changed since it was last used. */
static void fb_wayland_record_dirty_lines(void)
{
	int active_buffers = 0;
	int i, y;

	for (i = 0; i < FB_WL_BUFFER_SLOTS; i++) {
		FB_WAYLAND_BUFFER *slot = &fb_wayland_window.buffers[i];
		if (slot->buffer && !slot->retired && slot->dirty &&
		    slot->height >= fb_gfx->h)
			active_buffers++;
	}
	if (!active_buffers)
		return;
	for (y = 0; y < fb_gfx->h; y++) {
		if (!fb_gfx->dirty[y])
			continue;
		for (i = 0; i < FB_WL_BUFFER_SLOTS; i++) {
			FB_WAYLAND_BUFFER *slot = &fb_wayland_window.buffers[i];
			if (slot->buffer && !slot->retired && slot->dirty &&
			    slot->height > y)
				slot->dirty[y] = TRUE;
		}
		fb_gfx->dirty[y] = FALSE;
	}
}

static void fb_wayland_present(void)
{
	FB_WAYLAND_BUFFER *slot = NULL;
	FB_WAYLAND_BUFFER *empty = NULL;
	struct wl_callback *frame_callback;
	char *saved_dirty;
	int active_buffers = 0;
	int i;

	/* Nested input polling must not present a partly drawn SCREENLOCK frame. */
	if (!fb_wayland_window.active || !fb_wayland_window.configured ||
		!fb_wayland_window.repaint_pending ||
		(fb_wayland_window.frame_callback &&
		 !fb_wayland_window.resize_present_pending) ||
		!fb_gfx || fb_gfx->lock_count > 0 || !fb_gfx->framebuffer ||
		!fb_gfx->dirty || fb_gfx->h <= 0)
		return;
	if (!fb_wayland_ack_configure(&fb_wayland_window))
		return;
	fb_wayland_record_dirty_lines();
	for (i = 0; i < FB_WL_BUFFER_SLOTS; i++) {
		FB_WAYLAND_BUFFER *candidate = &fb_wayland_window.buffers[i];
		if (!candidate->buffer) {
			if (!empty)
				empty = candidate;
		} else if (!candidate->retired) {
			active_buffers++;
			if (!candidate->busy && !slot)
				slot = candidate;
		}
	}
	if (!slot && empty && active_buffers < FB_WL_PRESENT_BUFFERS &&
	    fb_wayland_create_buffer(empty, fb_wayland_window.width,
		fb_wayland_window.height) == 0)
		slot = empty;
	if (!slot || !slot->pixels || !slot->dirty || slot->height < fb_gfx->h)
		return;

	/*
	 * A compositor may withhold a frame callback until it can display a
	 * correctly sized buffer. Let a resize replace the old buffer while
	 * retaining the outstanding callback, so neither side waits on the other.
	 */
	if (!fb_wayland_window.frame_callback) {
		frame_callback = (struct wl_callback *)fb_wayland_window.api->
			proxy_marshal_constructor_versioned(
				(struct wl_proxy *)fb_wayland_window.surface,
				FB_WL_SURFACE_FRAME, fb_wayland_window.callback_interface, 1, NULL);
		if (!frame_callback)
			return;
		if (fb_wayland_add_listener(frame_callback, &fb_wayland_frame_listener,
			&fb_wayland_window) != 0) {
			fb_wayland_destroy(frame_callback);
			return;
		}
		fb_wayland_window.frame_callback = frame_callback;
	}

	/* The driver lock keeps drawing away while the blitter uses this mask. */
	saved_dirty = fb_gfx->dirty;
	fb_gfx->dirty = slot->dirty;
	fb_wayland_window.blitter((unsigned char *)slot->pixels,
		fb_wayland_window.stride);
	fb_gfx->dirty = saved_dirty;
	memset(slot->dirty, FALSE, (size_t)fb_gfx->h);
	fb_wayland_window.api->proxy_marshal((struct wl_proxy *)fb_wayland_window.surface,
		FB_WL_SURFACE_ATTACH, slot->buffer, 0, 0);
	fb_wayland_window.api->proxy_marshal((struct wl_proxy *)fb_wayland_window.surface,
		FB_WL_SURFACE_DAMAGE, 0, 0, fb_wayland_window.width,
		fb_wayland_window.height);
	fb_wayland_window.api->proxy_marshal((struct wl_proxy *)fb_wayland_window.surface,
		FB_WL_SURFACE_COMMIT);
	(void)fb_wayland_window.api->display_flush(fb_wayland_window.display);
	slot->busy = 1;
	fb_wayland_window.repaint_pending = 0;
	fb_wayland_window.resize_present_pending = 0;
}

static void fb_wayland_driver_update(void)
{
	pthread_mutex_lock(&fb_wayland_mutex);
	fb_wayland_window.repaint_pending = 1;
	fb_wayland_present();
	pthread_mutex_unlock(&fb_wayland_mutex);
}

static void fb_wayland_keyboard_repeat(void)
{
	FB_WAYLAND_WINDOW *window = &fb_wayland_window;
	struct timespec now;
	uint64_t now_ms;
	char text[16];
	uint32_t codepoint = 0;
	int key_value;
	EVENT event;
	if (!window->keyboard_focused || !fb_gfx ||
		(window->repeat_key >= FB_WL_EVDEV_KEY_MAX) ||
		!window->repeat_interval_ms ||
		!window->evdev_key_down[window->repeat_key])
		return;
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
		return;
	now_ms = (uint64_t)now.tv_sec * 1000u +
		(uint64_t)now.tv_nsec / 1000000u;
	if (now_ms < window->repeat_deadline_ms)
		return;
	window->repeat_deadline_ms = now_ms + window->repeat_interval_ms;
	if (window->xkb_state) {
		memset(text, 0, sizeof(text));
		if (window->xkb.state_key_get_utf8(window->xkb_state,
			window->repeat_key + 8u, text, sizeof(text)) > 0)
			codepoint = fb_wayland_first_codepoint(text);
	}
	if (window->repeat_scancode == 0) {
		if (codepoint)
			fb_wayland_post_text(text);
		return;
	}
	key_value = codepoint ? ((codepoint <= 0xFFu) ? (int)codepoint : 0) :
		fb_hScancodeToExtendedKey(window->repeat_scancode);
	if (key_value)
		fb_hPostKey(key_value);
	memset(&event, 0, sizeof(event));
	event.type = EVENT_KEY_REPEAT;
	event.scancode = window->repeat_scancode;
	if (codepoint)
		event.ascii = (int)codepoint;
	else if (key_value >= 0 && key_value <= 0xFF)
		event.ascii = key_value;
	fb_hPostEvent(&event);
}

/* The caller holds fb_wayland_mutex, including during protocol callbacks. */
static void fb_wayland_poll_events_locked(void)
{
	struct pollfd descriptor;
	int result;
	if (!fb_wayland_window.active ||
		!fb_wayland_window.api->display_prepare_read ||
		!fb_wayland_window.api->display_read_events ||
		!fb_wayland_window.api->display_cancel_read ||
		!fb_wayland_window.api->display_get_fd ||
		!fb_wayland_window.api->display_dispatch_pending)
		return;
	while (fb_wayland_window.api->display_prepare_read(fb_wayland_window.display) != 0) {
		if (fb_wayland_window.api->display_dispatch_pending(
			fb_wayland_window.display) < 0)
			return;
	}
	(void)fb_wayland_window.api->display_flush(fb_wayland_window.display);
	descriptor.fd = fb_wayland_window.api->display_get_fd(fb_wayland_window.display);
	descriptor.events = POLLIN;
	descriptor.revents = 0;
	result = poll(&descriptor, 1, 0);
	if (result > 0 && (descriptor.revents & POLLIN))
		(void)fb_wayland_window.api->display_read_events(fb_wayland_window.display);
	else
		fb_wayland_window.api->display_cancel_read(fb_wayland_window.display);
	(void)fb_wayland_window.api->display_dispatch_pending(fb_wayland_window.display);
	fb_wayland_keyboard_repeat();
	fb_wayland_present();
}

static void *fb_wayland_window_thread(void *unused)
{
	const struct timespec interval = { 0, FB_WL_POLL_INTERVAL_NS };
	(void)unused;

	for (;;) {
		pthread_mutex_lock(&fb_wayland_mutex);
		if (!fb_wayland_thread_running) {
			pthread_mutex_unlock(&fb_wayland_mutex);
			break;
		}
		if (fb_wayland_has_dirty_lines())
			fb_wayland_window.repaint_pending = 1;
		fb_wayland_poll_events_locked();
		pthread_mutex_unlock(&fb_wayland_mutex);
		(void)nanosleep(&interval, NULL);
	}
	return NULL;
}

static void fb_wayland_driver_poll_events(void)
{
	pthread_mutex_lock(&fb_wayland_mutex);
	/* The first poll occurs after the generic layer has initialized its pages. */
	if (fb_wayland_window.active && !fb_wayland_thread_started &&
	    !fb_wayland_thread_failed) {
		fb_wayland_thread_running = 1;
		if (pthread_create(&fb_wayland_thread, NULL,
		    fb_wayland_window_thread, NULL) == 0) {
			fb_wayland_thread_started = 1;
		} else {
			fb_wayland_thread_running = 0;
			fb_wayland_thread_failed = 1;
		}
	}
	/* The presentation thread owns event dispatch once it has started. */
	if (!fb_wayland_thread_started) {
		if (fb_wayland_has_dirty_lines())
			fb_wayland_window.repaint_pending = 1;
		fb_wayland_poll_events_locked();
	}
	pthread_mutex_unlock(&fb_wayland_mutex);
}

static int fb_wayland_driver_resize(int width, int height)
{
	FB_WAYLAND_BUFFER *replacement = NULL;
	int i;

	if (!fb_wayland_window.active)
		return -1;
	for (i = 0; i < FB_WL_BUFFER_SLOTS; i++) {
		if (!fb_wayland_window.buffers[i].buffer) {
			replacement = &fb_wayland_window.buffers[i];
			break;
		}
	}
	/* Allocate first so an unsuccessful resize leaves the old mode usable. */
	if (!replacement || fb_wayland_create_buffer(replacement, width, height) != 0)
		return -1;
	for (i = 0; i < FB_WL_BUFFER_SLOTS; i++) {
		FB_WAYLAND_BUFFER *slot = &fb_wayland_window.buffers[i];
		if (slot == replacement || !slot->buffer || slot->retired)
			continue;
		if (slot->busy)
			slot->retired = 1;
		else
			fb_wayland_destroy_slot(slot);
	}
	fb_wayland_window.width = width;
	fb_wayland_window.height = height;
	fb_wayland_window.stride = width * 4;
	fb_wayland_window.repaint_pending = 1;
	fb_wayland_window.resize_present_pending = 1;
	return 0;
}

static int fb_wayland_driver_get_mouse(int *x, int *y, int *z,
	int *buttons, int *clip)
{
	*x = fb_wayland_window.mouse_x;
	*y = fb_wayland_window.mouse_y;
	*z = fb_wayland_window.mouse_wheel;
	*buttons = fb_wayland_window.mouse_buttons |
		fb_wayland_window.pointer_buttons;
	*clip = 0;
	return 0;
}

static int fb_wayland_driver_get_touch_count(void)
{
	return fb_wayland_touch_count();
}

static int fb_wayland_driver_get_touch(int index, int *x, int *y, int *id)
{
	int i;
	int seen = 0;
	if (index < 0)
		return -1;
	for (i = 0; i < FB_WL_MAX_TOUCH_POINTS; i++) {
		if (!fb_wayland_window.touch_points[i].active)
			continue;
		if (seen == index) {
			if (x)
				*x = fb_wayland_window.touch_points[i].x;
			if (y)
				*y = fb_wayland_window.touch_points[i].y;
			if (id)
				*id = fb_wayland_window.touch_points[i].id;
			return 0;
		}
		seen++;
	}
	return -1;
}

static void fb_wayland_driver_set_window_title(char *title)
{
	FB_WAYLAND_WINDOW *window = &fb_wayland_window;

	if (!fb_wayland_prepare_mutex())
		return;
	pthread_mutex_lock(&fb_wayland_mutex);
	/*
	 * Mobile launchers can match running windows by title. WINDOWTITLE may
	 * change it after the surface was created, so update the native role too.
	 * The mutex keeps this request away from event dispatch and role teardown.
	 */
	if (window->active && window->display && window->api &&
	    window->api->proxy_marshal && window->api->display_flush) {
		if (window->toplevel) {
			window->api->proxy_marshal((struct wl_proxy *)window->toplevel,
				FB_WL_XDG_TOPLEVEL_SET_TITLE, title ? title : "");
		} else if (window->shell_surface) {
			window->api->proxy_marshal((struct wl_proxy *)window->shell_surface,
				FB_WL_SHELL_SURFACE_SET_TITLE, title ? title : "");
		} else {
			pthread_mutex_unlock(&fb_wayland_mutex);
			return;
		}
		/* Temporary socket backpressure leaves the request for the poll flush. */
		(void)window->api->display_flush(window->display);
	}
	pthread_mutex_unlock(&fb_wayland_mutex);
}

const GFXDRIVER fb_gfxDriverWayland = {
	"Wayland",
	fb_wayland_driver_init,
	fb_wayland_driver_exit,
	fb_wayland_driver_lock,
	fb_wayland_driver_unlock,
	NULL,
	NULL,
	fb_wayland_driver_get_mouse,
	fb_wayland_driver_get_touch_count,
	fb_wayland_driver_get_touch,
	NULL,
	fb_wayland_driver_set_window_title,
	NULL,
	NULL,
	NULL,
	fb_wayland_driver_poll_events,
	fb_wayland_driver_update,
	fb_wayland_driver_resize
};

static void fb_wayland_remember_sent_region(FB_WAYLAND_WINDOW *window)
{
	window->sent_region_x = window->region_x;
	window->sent_region_y = window->region_y;
	window->sent_region_width = window->region_width;
	window->sent_region_height = window->region_height;
	window->sent_region_valid = 1;
}

static int fb_wayland_sent_region_matches(const FB_WAYLAND_WINDOW *window)
{
	return window->sent_region_valid &&
		window->sent_region_x == window->region_x &&
		window->sent_region_y == window->region_y &&
		window->sent_region_width == window->region_width &&
		window->sent_region_height == window->region_height;
}

static int fb_wayland_set_keyboard_locked(int visible)
{
	FB_WAYLAND_WINDOW *window = &fb_wayland_window;
	if (!window->active)
		return -1;
	if (visible && !window->keyboard_requested && !window->keyboard_viewport_reserved) {
		window->keyboard_base_width = window->width;
		window->keyboard_base_height = window->height;
	}
	/* Changing editors can hide then show before the shell restores its work
	 * area. Retain the acknowledged geometry across that request gap. Queries
	 * remain inactive while hidden; a full native configure retires the area. */
	window->keyboard_requested = visible != 0;
	if (!visible)
		window->sent_region_valid = 0;
	if (window->webos.text_ready) {
		/* A card can construct its editor before the compositor focuses it. */
		if (visible && !window->keyboard_focused)
			return 0;
		(void)fb_hWaylandWebosRegion(&window->webos, window->region_x,
			window->region_y, window->region_width, window->region_height);
		if (fb_hWaylandWebosKeyboard(&window->webos, visible) != 0)
			return -1;
		(void)window->api->display_flush(window->display);
		return 0;
	}
	/* Let the caller use the session OSK when this surface has no text focus. */
	if (window->text_input) {
		if (!window->text_input_entered)
			return -1;
		/* Enabling or disabling v3 invalidates its uncommitted edit. */
		fb_hWaylandTextV3Reset(&window->text_v3_pending);
		if (visible) {
			window->api->proxy_marshal((struct wl_proxy *)window->text_input,
				FB_WL_TEXT_INPUT_ENABLE);
			window->api->proxy_marshal((struct wl_proxy *)window->text_input,
				FB_WL_TEXT_INPUT_SET_CURSOR_RECTANGLE, window->region_x,
				window->region_y, window->region_width, window->region_height);
			fb_wayland_remember_sent_region(window);
			window->keyboard_visible = -2;
		} else {
			window->api->proxy_marshal((struct wl_proxy *)window->text_input,
				FB_WL_TEXT_INPUT_DISABLE);
			window->keyboard_visible = 0;
		}
		window->api->proxy_marshal((struct wl_proxy *)window->text_input,
			FB_WL_TEXT_INPUT_COMMIT);
		(void)window->api->display_flush(window->display);
		return 0;
	}
	if (window->text_input_v2) {
		if (!window->text_input_entered)
			return -1;
		if (visible) {
			window->api->proxy_marshal((struct wl_proxy *)window->text_input_v2,
				FB_WL_TEXT_INPUT_V2_ENABLE,
				(struct wl_proxy *)window->surface);
			window->api->proxy_marshal((struct wl_proxy *)window->text_input_v2,
				FB_WL_TEXT_INPUT_V2_SET_CURSOR_RECTANGLE,
				window->region_x, window->region_y, window->region_width,
				window->region_height);
			fb_wayland_remember_sent_region(window);
			window->api->proxy_marshal((struct wl_proxy *)window->text_input_v2,
				FB_WL_TEXT_INPUT_V2_UPDATE_STATE,
				window->text_input_serial,
				FB_WL_TEXT_INPUT_V2_UPDATE_STATE_ENTER);
			window->api->proxy_marshal((struct wl_proxy *)window->text_input_v2,
				FB_WL_TEXT_INPUT_V2_SHOW_INPUT_PANEL);
			window->keyboard_visible = -2;
		} else {
			window->api->proxy_marshal((struct wl_proxy *)window->text_input_v2,
				FB_WL_TEXT_INPUT_V2_DISABLE,
				(struct wl_proxy *)window->surface);
			window->api->proxy_marshal((struct wl_proxy *)window->text_input_v2,
				FB_WL_TEXT_INPUT_V2_UPDATE_STATE,
				window->text_input_serial,
				FB_WL_TEXT_INPUT_V2_UPDATE_STATE_CHANGE);
			window->api->proxy_marshal((struct wl_proxy *)window->text_input_v2,
				FB_WL_TEXT_INPUT_V2_HIDE_INPUT_PANEL);
			window->keyboard_visible = 0;
		}
		(void)window->api->display_flush(window->display);
		return 0;
	}
	return -1;
}

static int fb_wayland_set_text_input_region_locked(int x, int y,
	int width, int height)
{
	FB_WAYLAND_WINDOW *window = &fb_wayland_window;
	if (!window->active || (!window->text_input && !window->text_input_v2 && !window->webos.text_ready))
		return -1;
	if (window->webos.text_ready) {
		int result = fb_hWaylandWebosRegion(&window->webos, x, y, width, height);
		if (result == 0) {
			window->region_x = x; window->region_y = y;
			window->region_width = width; window->region_height = height;
			(void)window->api->display_flush(window->display);
		}
		return result;
	}
	if ((width < 0) || (height < 0) || x < 0 || y < 0 ||
		x > window->width || y > window->height ||
		width > window->width - x || height > window->height - y)
		return -1;
	window->region_x = x;
	window->region_y = y;
	window->region_width = width;
	window->region_height = height;
	if (window->keyboard_requested && window->text_input_entered) {
		/* CLEAR followed by the same registration changes no compositor state. */
		if (fb_wayland_sent_region_matches(window))
			return 0;
		/*
		 * Enabling text-input-v3 resets its state. A rectangle change only
		 * needs a state update, so keep an active composition intact.
		 */
		if (window->text_input) {
			window->api->proxy_marshal((struct wl_proxy *)window->text_input,
				FB_WL_TEXT_INPUT_SET_CURSOR_RECTANGLE, x, y, width, height);
			window->api->proxy_marshal((struct wl_proxy *)window->text_input,
				FB_WL_TEXT_INPUT_COMMIT);
		} else {
			window->api->proxy_marshal((struct wl_proxy *)window->text_input_v2,
				FB_WL_TEXT_INPUT_V2_SET_CURSOR_RECTANGLE, x, y, width, height);
			window->api->proxy_marshal((struct wl_proxy *)window->text_input_v2,
				FB_WL_TEXT_INPUT_V2_UPDATE_STATE, window->text_input_serial,
				FB_WL_TEXT_INPUT_V2_UPDATE_STATE_CHANGE);
		}
		fb_wayland_remember_sent_region(window);
		(void)window->api->display_flush(window->display);
	}
	return 0;
}

static int fb_wayland_clear_text_input_regions_locked(void)
{
	if (!fb_wayland_window.active)
		return -1;
	fb_wayland_window.region_x = 0;
	fb_wayland_window.region_y = 0;
	fb_wayland_window.region_width = 1;
	fb_wayland_window.region_height = 1;
	return 0;
}

static int fb_wayland_get_keyboard_state_locked(int *visible)
{
	if (visible && fb_wayland_window.active && fb_wayland_window.webos.text_ready) {
		*visible = fb_wayland_window.webos.visible;
		return 1;
	}
	if (!visible || !fb_wayland_window.active ||
		(!fb_wayland_window.text_input && !fb_wayland_window.text_input_v2))
		return 0;
	if (fb_wayland_window.text_input_v2)
		*visible = fb_wayland_window.keyboard_visible;
	else
		*visible = (fb_wayland_window.text_input_entered &&
			fb_wayland_window.keyboard_requested) ? -2 : 0;
	return 1;
}

int fb_hWaylandSetKeyboard(int visible)
{
	int result;
	if (!fb_wayland_prepare_mutex())
		return -1;
	pthread_mutex_lock(&fb_wayland_mutex);
	result = fb_wayland_set_keyboard_locked(visible);
	pthread_mutex_unlock(&fb_wayland_mutex);
	return result;
}

int fb_hWaylandSetTextInputRegion(int x, int y, int width, int height)
{
	int result;
	if (!fb_wayland_prepare_mutex())
		return -1;
	pthread_mutex_lock(&fb_wayland_mutex);
	result = fb_wayland_set_text_input_region_locked(x, y, width, height);
	pthread_mutex_unlock(&fb_wayland_mutex);
	return result;
}

int fb_hWaylandClearTextInputRegions(void)
{
	int result;
	if (!fb_wayland_prepare_mutex())
		return -1;
	pthread_mutex_lock(&fb_wayland_mutex);
	result = fb_wayland_clear_text_input_regions_locked();
	pthread_mutex_unlock(&fb_wayland_mutex);
	return result;
}

int fb_hWaylandGetKeyboardState(int *visible)
{
	int result;
	if (!fb_wayland_prepare_mutex())
		return 0;
	pthread_mutex_lock(&fb_wayland_mutex);
	result = fb_wayland_get_keyboard_state_locked(visible);
	pthread_mutex_unlock(&fb_wayland_mutex);
	return result;
}

int fb_hWaylandGetKeyboardOcclusion(int *top, int *height)
{
	FB_WAYLAND_WINDOW *window = &fb_wayland_window;
	FB_WAYLAND_WEBOS *webos = &fb_wayland_window.webos;
	int result = 0;
	if (!top || !height || !fb_wayland_prepare_mutex())
		return 0;
	pthread_mutex_lock(&fb_wayland_mutex);
	if (window->active && fb_gfx && window->text_input_v2 &&
		window->keyboard_visible == -1 && window->panel_width > 0 &&
		window->panel_height > 0 && window->panel_x < fb_gfx->w &&
		(int64_t)window->panel_x + window->panel_width > 0 &&
		window->panel_y < fb_gfx->h) {
		int panel_top = window->panel_y > 0 ? window->panel_y : 0;
		int64_t bottom = (int64_t)window->panel_y + window->panel_height;
		if (bottom > fb_gfx->h)
			bottom = fb_gfx->h;
		if (bottom > panel_top) {
			*top = panel_top;
			*height = (int)(bottom - panel_top);
			result = 1;
		}
	} else if (window->active && fb_gfx && webos->visible == -1 &&
		webos->panel_height > 0 && webos->panel_top < fb_gfx->h) {
		int panel_top = webos->panel_top > 0 ? webos->panel_top : 0;
		int64_t bottom = (int64_t)webos->panel_top + webos->panel_height;
		if (bottom > fb_gfx->h)
			bottom = fb_gfx->h;
		if (bottom > panel_top) {
			*top = panel_top;
			*height = (int)(bottom - panel_top);
			result = 1;
		}
	}
	if (!result && window->active && fb_gfx && fb_gfx->h > 0 &&
		window->text_input && window->text_input_entered &&
		window->keyboard_requested && window->keyboard_viewport_reserved) {
		/* Empty overlap at the drawable end means the acknowledged maximized
		 * viewport already excludes the keyboard. v3 cannot confirm visibility;
		 * callers must also obtain a known-visible observation from the OSK. */
		*top = fb_gfx->h;
		*height = 0;
		result = 1;
	}
	/* Direct Maliit is the fallback on shells without a text-input protocol.
	 * Its observed area is preferable to a guessed whole-window reservation.
	 * Native text-input and webOS panel observations keep their priority. */
	if (!result && window->active && fb_gfx &&
		!window->text_input && !window->text_input_v2)
		result = fb_hLinuxMaliitGetKeyboardOcclusion(fb_gfx->w,
			fb_gfx->h, top, height);
	pthread_mutex_unlock(&fb_wayland_mutex);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Native handle and OpenGL compatibility                                     */
/* ------------------------------------------------------------------------- */

#ifdef DISABLE_X11
ssize_t fb_hGetWindowHandle(void)
{
	ssize_t result = 0;
	if (!fb_wayland_prepare_mutex())
		return 0;
	pthread_mutex_lock(&fb_wayland_mutex);
	result = (ssize_t)(intptr_t)fb_wayland_window.surface;
	pthread_mutex_unlock(&fb_wayland_mutex);
	return result;
}

ssize_t fb_hGetDisplayHandle(void)
{
	ssize_t result = 0;
	if (!fb_wayland_prepare_mutex())
		return 0;
	pthread_mutex_lock(&fb_wayland_mutex);
	result = (ssize_t)(intptr_t)fb_wayland_window.display;
	pthread_mutex_unlock(&fb_wayland_mutex);
	return result;
}

#ifndef DISABLE_OPENGL
/* The SHM presentation backend owns no EGL/OpenGL context or resolver. */
void *fb_hGL_GetProcAddress(const char *proc)
{
	(void)proc;
	return NULL;
}
#endif
#endif

/* end of gfx_driver_wayland.c */
