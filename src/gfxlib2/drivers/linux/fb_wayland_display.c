/*
 * Project: FreeBASIC gfxlib2 Linux Wayland support
 * File: fb_wayland_display.c
 * Purpose: Read the current output geometry before a graphics mode exists.
 * Responsibilities: Load the optional client ABI, bind one output and seat,
 *     then report logical geometry, scale, density, rotation and capabilities.
 * This file intentionally does not create a window or choose application UI.
 */
#define _POSIX_C_SOURCE 200809L

#include "fb_wayland_display.h"
#include "fb_wayland_client.h"

#include <errno.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Core Wayland protocol opcodes and enum values used by the small probe. */
#define FB_WL_DISPLAY_GET_REGISTRY 1
#define FB_WL_REGISTRY_BIND 0
#define FB_WL_OUTPUT_RELEASE 0
#define FB_WL_SEAT_RELEASE 3
#define FB_XDG_OUTPUT_MANAGER_DESTROY 0
#define FB_XDG_OUTPUT_MANAGER_GET_OUTPUT 1
#define FB_XDG_OUTPUT_DESTROY 0
#define FB_WL_OUTPUT_MODE_CURRENT 1
#define FB_WL_SEAT_CAPABILITY_TOUCH 4
#define FB_WL_OUTPUT_TRANSFORM_90 1
#define FB_WL_OUTPUT_TRANSFORM_180 2
#define FB_WL_OUTPUT_TRANSFORM_270 3
#define FB_WL_OUTPUT_TRANSFORM_FLIPPED_90 5
#define FB_WL_OUTPUT_TRANSFORM_FLIPPED_180 6
#define FB_WL_OUTPUT_TRANSFORM_FLIPPED_270 7

struct wl_display;
struct wl_proxy;
struct wl_registry;
struct wl_output;
struct wl_seat;
struct zxdg_output_manager_v1;
struct zxdg_output_v1;

/*
 * xdg-output supplies logical geometry when a compositor uses fractional
 * scaling. The core wl_output scale is only an integer buffer scale. Version
 * 1 is sufficient for logical size and its atomic done event, and keeps this
 * optional probe independent of generated headers and link-time libraries.
 */
static const struct wl_interface fb_xdg_output_interface;
static const struct wl_interface *fb_xdg_output_request_types[] = {
	&fb_xdg_output_interface, NULL
};
static const struct wl_interface *fb_xdg_output_no_types[] = { NULL, NULL };
static const struct wl_message fb_xdg_output_manager_requests[] = {
	{ "destroy", "", fb_xdg_output_no_types },
	{ "get_xdg_output", "no", fb_xdg_output_request_types }
};
static const struct wl_message fb_xdg_output_requests[] = {
	{ "destroy", "", fb_xdg_output_no_types }
};
static const struct wl_message fb_xdg_output_events[] = {
	{ "logical_position", "ii", fb_xdg_output_no_types },
	{ "logical_size", "ii", fb_xdg_output_no_types },
	{ "done", "", fb_xdg_output_no_types }
};
static const struct wl_interface fb_xdg_output_manager_interface = {
	"zxdg_output_manager_v1", 1, 2, fb_xdg_output_manager_requests, 0, NULL
};
static const struct wl_interface fb_xdg_output_interface = {
	"zxdg_output_v1", 1, 1, fb_xdg_output_requests, 3, fb_xdg_output_events
};

typedef struct FB_WAYLAND_OUTPUT_LISTENER {
	void (*geometry)(void *, struct wl_output *, int, int, int, int, int,
		const char *, const char *, int);
	void (*mode)(void *, struct wl_output *, uint32_t, int, int, int);
	void (*done)(void *, struct wl_output *);
	void (*scale)(void *, struct wl_output *, int32_t);
	void (*name)(void *, struct wl_output *, const char *);
	void (*description)(void *, struct wl_output *, const char *);
} FB_WAYLAND_OUTPUT_LISTENER;

typedef struct FB_WAYLAND_SEAT_LISTENER {
	void (*capabilities)(void *, struct wl_seat *, uint32_t);
	void (*name)(void *, struct wl_seat *, const char *);
} FB_WAYLAND_SEAT_LISTENER;

typedef struct FB_WAYLAND_XDG_OUTPUT_LISTENER {
	void (*logical_position)(void *, struct zxdg_output_v1 *, int32_t, int32_t);
	void (*logical_size)(void *, struct zxdg_output_v1 *, int32_t, int32_t);
	void (*done)(void *, struct zxdg_output_v1 *);
} FB_WAYLAND_XDG_OUTPUT_LISTENER;

typedef struct FB_WAYLAND_REGISTRY_LISTENER {
	void (*global)(void *, struct wl_registry *, uint32_t, const char *, uint32_t);
	void (*global_remove)(void *, struct wl_registry *, uint32_t);
} FB_WAYLAND_REGISTRY_LISTENER;

#define FB_WAYLAND_MAX_DPI 768
#define FB_WAYLAND_MIN_DPI 72
#define FB_WAYLAND_OUTPUT_VERSION 4
#define FB_WAYLAND_CACHE_NS 250000000L

typedef struct FB_WAYLAND_PROBE {
	struct wl_output *output;
	struct wl_seat *seat;
	struct zxdg_output_manager_v1 *xdg_manager;
	struct zxdg_output_v1 *xdg_output;
	uint32_t output_name;
	uint32_t seat_name;
	uint32_t xdg_manager_name;
	int mode_width;
	int mode_height;
	int refresh_millihz;
	int physical_width_mm;
	int physical_height_mm;
	int transform;
	int scale;
	int logical_width;
	int logical_height;
	int pending_logical_width;
	int pending_logical_height;
	int have_logical_size;
	int pending_logical_size;
	int touch_capable;
	int have_mode;
	int text_input_available;
	int error;
} FB_WAYLAND_PROBE;

static const FB_WAYLAND_API *fb_wayland_api;
static const struct wl_interface *fb_wayland_registry_interface;
static const struct wl_interface *fb_wayland_output_interface;
static const struct wl_interface *fb_wayland_seat_interface;
static pthread_mutex_t fb_wayland_probe_mutex = PTHREAD_MUTEX_INITIALIZER;
/* The independent probe connection is reused and protected by this mutex. */
static FB_WAYLAND_PROBE fb_wayland_probe_state;
static struct wl_display *fb_wayland_probe_display;
static struct wl_registry *fb_wayland_probe_registry;
static int fb_wayland_probe_cleanup_registered;
static FB_WAYLAND_DISPLAY_INFO fb_wayland_cached_info;
static struct timespec fb_wayland_cached_at;
static int fb_wayland_cache_valid;

/*
    Keep the probe independent of libwayland development headers and link-time
    symbols. These exported ABI declarations are limited to the stable core
    client objects needed to read one output; all requests still pass through
    libwayland's protocol marshaller.
*/
static int fb_wayland_load_api(void)
{
	static const char *const interfaces[] = {
		"wl_registry_interface", "wl_output_interface", "wl_seat_interface"
	};
	void *interface_symbols[3];

	if ((fb_hWaylandGetApi(&fb_wayland_api) != 0) ||
		(fb_hWaylandLoadSymbols(interfaces, interface_symbols, 3) != 0) ||
		(interface_symbols[0] == NULL) || (interface_symbols[1] == NULL) ||
		(interface_symbols[2] == NULL))
		return -1;
	fb_wayland_registry_interface =
		(const struct wl_interface *)interface_symbols[0];
	fb_wayland_output_interface =
		(const struct wl_interface *)interface_symbols[1];
	fb_xdg_output_request_types[1] = fb_wayland_output_interface;
	fb_wayland_seat_interface =
		(const struct wl_interface *)interface_symbols[2];
	return fb_hWaylandInitProtocols();
}

static struct wl_registry *fb_wayland_get_registry(struct wl_display *display)
{
	struct wl_proxy *proxy = fb_wayland_api->proxy_marshal_constructor_versioned(
		(struct wl_proxy *)display, FB_WL_DISPLAY_GET_REGISTRY,
		fb_wayland_registry_interface,
		fb_wayland_api->proxy_get_version((struct wl_proxy *)display), NULL);
	return (struct wl_registry *)proxy;
}

static void *fb_wayland_registry_bind(struct wl_registry *registry,
	uint32_t name, const struct wl_interface *interface, uint32_t version)
{
	return fb_wayland_api->proxy_marshal_constructor_versioned(
		(struct wl_proxy *)registry, FB_WL_REGISTRY_BIND, interface, version,
		name, interface->name, version, NULL);
}

static int fb_wayland_add_listener(void *proxy, const void *listener, void *data)
{
	return fb_wayland_api->proxy_add_listener((struct wl_proxy *)proxy,
		(void (**)(void))listener, data);
}

static void fb_wayland_destroy_proxy(void *proxy)
{
	if (proxy)
		fb_wayland_api->proxy_destroy((struct wl_proxy *)proxy);
}

/* A broken connection can only discard proxies locally, without requests. */
static void fb_wayland_destroy_xdg_output(FB_WAYLAND_PROBE *probe, int send_requests)
{
	if (send_requests && fb_wayland_api->proxy_marshal != NULL) {
		if (probe->xdg_output)
			fb_wayland_api->proxy_marshal((struct wl_proxy *)probe->xdg_output,
				FB_XDG_OUTPUT_DESTROY);
		if (probe->xdg_manager)
			fb_wayland_api->proxy_marshal((struct wl_proxy *)probe->xdg_manager,
				FB_XDG_OUTPUT_MANAGER_DESTROY);
	}
	fb_wayland_destroy_proxy(probe->xdg_output);
	fb_wayland_destroy_proxy(probe->xdg_manager);
	probe->xdg_output = NULL;
	probe->xdg_manager = NULL;
	probe->xdg_manager_name = 0;
	probe->have_logical_size = 0;
	probe->pending_logical_size = 0;
}

static void fb_wayland_output_geometry(void *data, struct wl_output *output,
	int x, int y, int physical_width, int physical_height, int subpixel,
	const char *make, const char *model, int transform)
{
	FB_WAYLAND_PROBE *probe = (FB_WAYLAND_PROBE *)data;
	(void)output;
	(void)x;
	(void)y;
	(void)subpixel;
	(void)make;
	(void)model;
	probe->physical_width_mm = physical_width;
	probe->physical_height_mm = physical_height;
	probe->transform = transform;
}

static void fb_wayland_output_mode(void *data, struct wl_output *output,
	uint32_t flags, int width, int height, int refresh)
{
	FB_WAYLAND_PROBE *probe = (FB_WAYLAND_PROBE *)data;
	(void)output;
	if ((flags & FB_WL_OUTPUT_MODE_CURRENT) == 0)
		return;
	probe->mode_width = width;
	probe->mode_height = height;
	probe->refresh_millihz = refresh;
	probe->have_mode = 1;
}

static void fb_wayland_output_done(void *data, struct wl_output *output)
{
	(void)data;
	(void)output;
}

static void fb_wayland_output_scale(void *data, struct wl_output *output,
	int32_t factor)
{
	FB_WAYLAND_PROBE *probe = (FB_WAYLAND_PROBE *)data;
	(void)output;
	if (factor > 0)
		probe->scale = factor;
}

static void fb_wayland_output_name(void *data, struct wl_output *output,
	const char *name)
{
	(void)data;
	(void)output;
	(void)name;
}

static void fb_wayland_output_description(void *data, struct wl_output *output,
	const char *description)
{
	(void)data;
	(void)output;
	(void)description;
}

static const FB_WAYLAND_OUTPUT_LISTENER fb_wayland_output_listener = {
	fb_wayland_output_geometry,
	fb_wayland_output_mode,
	fb_wayland_output_done,
	fb_wayland_output_scale,
	fb_wayland_output_name,
	fb_wayland_output_description
};

static void fb_wayland_xdg_output_position(void *data,
	struct zxdg_output_v1 *output, int32_t x, int32_t y)
{
	(void)data;
	(void)output;
	(void)x;
	(void)y;
}

static void fb_wayland_xdg_output_size(void *data,
	struct zxdg_output_v1 *output, int32_t width, int32_t height)
{
	FB_WAYLAND_PROBE *probe = (FB_WAYLAND_PROBE *)data;
	(void)output;
	probe->pending_logical_size = width > 0 && height > 0;
	if (probe->pending_logical_size) {
		probe->pending_logical_width = width;
		probe->pending_logical_height = height;
	}
}

/* Apply a complete xdg-output update, not an intermediate geometry. */
static void fb_wayland_xdg_output_done(void *data, struct zxdg_output_v1 *output)
{
	FB_WAYLAND_PROBE *probe = (FB_WAYLAND_PROBE *)data;
	(void)output;
	if (probe->pending_logical_size) {
		probe->logical_width = probe->pending_logical_width;
		probe->logical_height = probe->pending_logical_height;
		probe->have_logical_size = 1;
		probe->pending_logical_size = 0;
	}
}

static const FB_WAYLAND_XDG_OUTPUT_LISTENER fb_wayland_xdg_output_listener = {
	fb_wayland_xdg_output_position,
	fb_wayland_xdg_output_size,
	fb_wayland_xdg_output_done
};

/* The manager and output globals may be advertised in either order. */
static void fb_wayland_bind_xdg_output(FB_WAYLAND_PROBE *probe)
{
	if (!probe->xdg_manager || !probe->output || probe->xdg_output)
		return;
	probe->xdg_output = (struct zxdg_output_v1 *)
		fb_wayland_api->proxy_marshal_constructor_versioned(
			(struct wl_proxy *)probe->xdg_manager, FB_XDG_OUTPUT_MANAGER_GET_OUTPUT,
			&fb_xdg_output_interface, 1, NULL, probe->output);
	if (!probe->xdg_output || fb_wayland_add_listener(probe->xdg_output,
		&fb_wayland_xdg_output_listener, probe) != 0)
		probe->error = 1;
}

static void fb_wayland_seat_capabilities(void *data, struct wl_seat *seat,
	uint32_t capabilities)
{
	FB_WAYLAND_PROBE *probe = (FB_WAYLAND_PROBE *)data;
	(void)seat;
	probe->touch_capable =
		(capabilities & FB_WL_SEAT_CAPABILITY_TOUCH) != 0;
}

static void fb_wayland_seat_name(void *data, struct wl_seat *seat,
	const char *name)
{
	(void)data;
	(void)seat;
	(void)name;
}

static const FB_WAYLAND_SEAT_LISTENER fb_wayland_seat_listener = {
	fb_wayland_seat_capabilities,
	fb_wayland_seat_name
};

static void fb_wayland_registry_global(void *data, struct wl_registry *registry,
	uint32_t name, const char *interface, uint32_t version)
{
	FB_WAYLAND_PROBE *probe = (FB_WAYLAND_PROBE *)data;
	if ((probe->output == NULL) &&
		(strcmp(interface, fb_wayland_output_interface->name) == 0)) {
		uint32_t output_version = version;
		if (output_version > FB_WAYLAND_OUTPUT_VERSION)
			output_version = FB_WAYLAND_OUTPUT_VERSION;
		probe->output_name = name;
		probe->output = (struct wl_output *)fb_wayland_registry_bind(registry,
			name, fb_wayland_output_interface, output_version);
		if (probe->output == NULL) {
			probe->error = 1;
			return;
		}
		if (fb_wayland_add_listener(probe->output,
			&fb_wayland_output_listener, probe) != 0)
			probe->error = 1;
		fb_wayland_bind_xdg_output(probe);
	} else if ((probe->xdg_manager == NULL) &&
		(strcmp(interface, fb_xdg_output_manager_interface.name) == 0) &&
		(version >= 1)) {
		probe->xdg_manager_name = name;
		probe->xdg_manager = (struct zxdg_output_manager_v1 *)
			fb_wayland_registry_bind(registry, name,
				&fb_xdg_output_manager_interface, 1);
		if (probe->xdg_manager == NULL) {
			probe->error = 1;
			return;
		}
		fb_wayland_bind_xdg_output(probe);
	} else if ((probe->seat == NULL) &&
		(strcmp(interface, fb_wayland_seat_interface->name) == 0)) {
		uint32_t seat_version = version;
		if (seat_version > 5)
			seat_version = 5;
		probe->seat_name = name;
		probe->seat = (struct wl_seat *)fb_wayland_registry_bind(registry,
			name, fb_wayland_seat_interface, seat_version);
		if (probe->seat == NULL) {
			probe->error = 1;
			return;
		}
		if (fb_wayland_add_listener(probe->seat,
			&fb_wayland_seat_listener, probe) != 0)
			probe->error = 1;
	} else if ((strcmp(interface, "zwp_text_input_manager_v3") == 0) ||
		(strcmp(interface, "zwp_text_input_manager_v2") == 0)) {
		(void)name;
		(void)version;
		probe->text_input_available = 1;
	}
}

static void fb_wayland_registry_remove(void *data, struct wl_registry *registry,
	uint32_t name)
{
	FB_WAYLAND_PROBE *probe = (FB_WAYLAND_PROBE *)data;
	(void)registry;
	if (name == probe->output_name)
		probe->have_mode = 0;
	if (name == probe->seat_name)
		probe->touch_capable = 0;
	if (name == probe->xdg_manager_name)
		fb_wayland_destroy_xdg_output(probe, 1);
}

static const FB_WAYLAND_REGISTRY_LISTENER fb_wayland_registry_listener = {
	fb_wayland_registry_global,
	fb_wayland_registry_remove
};

static void fb_wayland_probe_close_locked(void)
{
	fb_wayland_destroy_xdg_output(&fb_wayland_probe_state, 0);
	fb_wayland_destroy_proxy(fb_wayland_probe_state.output);
	fb_wayland_destroy_proxy(fb_wayland_probe_state.seat);
	fb_wayland_destroy_proxy(fb_wayland_probe_registry);
	if (fb_wayland_probe_display)
		fb_wayland_api->display_disconnect(fb_wayland_probe_display);
	memset(&fb_wayland_probe_state, 0, sizeof(fb_wayland_probe_state));
	fb_wayland_probe_display = NULL;
	fb_wayland_probe_registry = NULL;
	fb_wayland_cache_valid = 0;
}

/*
 * Lomiri associates a process with its first Wayland session. Closing a
 * metrics-only session and opening another one can leave the window unmapped
 * and without input focus. Transfer the original connection to the window and
 * keep other metrics queries waiting until the window has registered.
 *
 * Output and seat objects have release requests in newer protocol versions.
 * Older versions and the registry have no release request; destroying their
 * local proxies is the normal Wayland client cleanup for those versions.
 */
int fb_hWaylandDisplayBeginWindow(struct wl_display **display)
{
	if (display == NULL)
		return -1;
	*display = NULL;
	if (pthread_mutex_lock(&fb_wayland_probe_mutex) != 0)
		return -1;
	if (fb_wayland_probe_display != NULL) {
		fb_wayland_destroy_xdg_output(&fb_wayland_probe_state, 1);
		if (fb_wayland_api->proxy_marshal != NULL &&
		    fb_wayland_probe_state.output != NULL &&
		    fb_wayland_api->proxy_get_version((struct wl_proxy *)
			fb_wayland_probe_state.output) >= 3)
			fb_wayland_api->proxy_marshal((struct wl_proxy *)
				fb_wayland_probe_state.output, FB_WL_OUTPUT_RELEASE);
		if (fb_wayland_api->proxy_marshal != NULL &&
		    fb_wayland_probe_state.seat != NULL &&
		    fb_wayland_api->proxy_get_version((struct wl_proxy *)
			fb_wayland_probe_state.seat) >= 5)
			fb_wayland_api->proxy_marshal((struct wl_proxy *)
				fb_wayland_probe_state.seat, FB_WL_SEAT_RELEASE);
		fb_wayland_destroy_proxy(fb_wayland_probe_state.output);
		fb_wayland_destroy_proxy(fb_wayland_probe_state.seat);
		fb_wayland_destroy_proxy(fb_wayland_probe_registry);
		*display = fb_wayland_probe_display;
		memset(&fb_wayland_probe_state, 0, sizeof(fb_wayland_probe_state));
		fb_wayland_probe_display = NULL;
		fb_wayland_probe_registry = NULL;
		fb_wayland_cache_valid = 0;
	}
	return 0;
}

void fb_hWaylandDisplayEndWindow(void)
{
	pthread_mutex_unlock(&fb_wayland_probe_mutex);
}

static void fb_wayland_probe_cleanup(void)
{
	pthread_mutex_lock(&fb_wayland_probe_mutex);
	fb_wayland_probe_close_locked();
	pthread_mutex_unlock(&fb_wayland_probe_mutex);
}

static int fb_wayland_probe_open_locked(void)
{
	FB_WAYLAND_PROBE *probe = &fb_wayland_probe_state;

	if (fb_wayland_probe_display)
		return 0;
	memset(probe, 0, sizeof(*probe));
	probe->scale = 1;
	if (fb_wayland_load_api() != 0)
		return -1;
	fb_wayland_probe_display = fb_wayland_api->display_connect(NULL);
	if (!fb_wayland_probe_display)
		return -1;
	fb_wayland_probe_registry = fb_wayland_get_registry(fb_wayland_probe_display);
	if (!fb_wayland_probe_registry ||
	    fb_wayland_add_listener(fb_wayland_probe_registry,
		&fb_wayland_registry_listener, probe) != 0 ||
	    fb_wayland_api->display_roundtrip(fb_wayland_probe_display) < 0 ||
	    probe->error || !probe->output ||
	    fb_wayland_api->display_roundtrip(fb_wayland_probe_display) < 0 ||
	    probe->error || !probe->have_mode)
		goto fail;
	if (!fb_wayland_probe_cleanup_registered) {
		if (atexit(fb_wayland_probe_cleanup) != 0)
			goto fail;
		fb_wayland_probe_cleanup_registered = 1;
	}
	return 0;

fail:
	fb_wayland_probe_close_locked();
	return -1;
}

/* Dispatch output changes without waiting for a compositor round trip. */
static int fb_wayland_probe_pump_locked(void)
{
	struct pollfd descriptor;
	int result;

	if (!fb_wayland_api->display_prepare_read ||
	    !fb_wayland_api->display_dispatch_pending ||
	    !fb_wayland_api->display_get_fd || !fb_wayland_api->display_flush ||
	    !fb_wayland_api->display_read_events || !fb_wayland_api->display_cancel_read)
		return fb_wayland_api->display_roundtrip(fb_wayland_probe_display) < 0 ? -1 : 0;
	while (fb_wayland_api->display_prepare_read(fb_wayland_probe_display) != 0) {
		if (fb_wayland_api->display_dispatch_pending(fb_wayland_probe_display) < 0)
			return -1;
	}
	if (fb_wayland_api->display_flush(fb_wayland_probe_display) < 0 && errno != EAGAIN) {
		fb_wayland_api->display_cancel_read(fb_wayland_probe_display);
		return -1;
	}
	descriptor.fd = fb_wayland_api->display_get_fd(fb_wayland_probe_display);
	descriptor.events = POLLIN;
	descriptor.revents = 0;
	result = poll(&descriptor, 1, 0);
	if (result > 0 && (descriptor.revents & POLLIN)) {
		if (fb_wayland_api->display_read_events(fb_wayland_probe_display) < 0)
			return -1;
	} else {
		fb_wayland_api->display_cancel_read(fb_wayland_probe_display);
		if ((result < 0 && errno != EINTR) ||
		    (result > 0 && (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))))
			return -1;
	}
	return fb_wayland_api->display_dispatch_pending(fb_wayland_probe_display) < 0 ? -1 : 0;
}

static int fb_wayland_probe(FB_WAYLAND_DISPLAY_INFO *info)
{
	FB_WAYLAND_PROBE *probe = &fb_wayland_probe_state;
	int rotation = 0;
	int dpi = 96;
	int width, height, scale = 1;

	if (fb_wayland_probe_open_locked() != 0)
		return -1;
	if (fb_wayland_probe_pump_locked() != 0 || probe->error || !probe->have_mode) {
		fb_wayland_probe_close_locked();
		return -1;
	}

	width = probe->mode_width;
	height = probe->mode_height;
	if ((width <= 0) || (height <= 0)) {
		fb_wayland_probe_close_locked();
		return -1;
	}
	scale = probe->scale > 0 ? probe->scale : 1;
	switch (probe->transform) {
	case FB_WL_OUTPUT_TRANSFORM_90:
	case FB_WL_OUTPUT_TRANSFORM_FLIPPED_90:
		rotation = 90;
		break;
	case FB_WL_OUTPUT_TRANSFORM_180:
	case FB_WL_OUTPUT_TRANSFORM_FLIPPED_180:
		rotation = 180;
		break;
	case FB_WL_OUTPUT_TRANSFORM_270:
	case FB_WL_OUTPUT_TRANSFORM_FLIPPED_270:
		rotation = 270;
		break;
	default:
		rotation = 0;
		break;
	}
	if (probe->have_logical_size) {
		/* xdg-output dimensions already include the output transform. */
		width = probe->logical_width;
		height = probe->logical_height;
	} else {
		if ((rotation == 90) || (rotation == 270)) {
			int temporary = width;
			width = height;
			height = temporary;
		}
		width /= scale;
		height /= scale;
	}
	if (width <= 0 || height <= 0)
		return -1;
	if ((probe->physical_width_mm > 0) && (probe->physical_height_mm > 0)) {
		double native_width = (double)probe->mode_width / (double)scale;
		double native_height = (double)probe->mode_height / (double)scale;
		double horizontal_dpi, vertical_dpi, average_dpi;
		if (probe->have_logical_size) {
			/* Physical panel dimensions are in the unrotated mode axes. */
			native_width = (rotation == 90 || rotation == 270) ? height : width;
			native_height = (rotation == 90 || rotation == 270) ? width : height;
		}
		horizontal_dpi = (native_width * 25.4) / probe->physical_width_mm;
		vertical_dpi = (native_height * 25.4) / probe->physical_height_mm;
		average_dpi = (horizontal_dpi + vertical_dpi) * 0.5;
		if (isfinite(average_dpi) && average_dpi >= FB_WAYLAND_MIN_DPI &&
		    average_dpi <= FB_WAYLAND_MAX_DPI)
			dpi = (int)(average_dpi + 0.5);
	}
	info->width = width;
	info->height = height;
	info->depth = 32;
	info->refresh = (probe->refresh_millihz > 0) ?
		(int)(((int64_t)probe->refresh_millihz + 500) / 1000) : 0;
	info->dpi = dpi;
	info->rotation = rotation;
	info->scale = scale;
	info->touch_capable = probe->touch_capable;
	info->text_input_available = probe->text_input_available;
	return 0;
}

int fb_hWaylandDisplayInfo(FB_WAYLAND_DISPLAY_INFO *info)
{
	struct timespec now;
	int64_t elapsed_ns;
	int result;

	if (info == NULL)
		return -1;
	if (!fb_hWaylandShouldUse()) {
		pthread_mutex_lock(&fb_wayland_probe_mutex);
		fb_wayland_probe_close_locked();
		pthread_mutex_unlock(&fb_wayland_probe_mutex);
		return -1;
	}
	/* This connection is independent of any window and observes output changes. */
	pthread_mutex_lock(&fb_wayland_probe_mutex);
	if ((clock_gettime(CLOCK_MONOTONIC, &now) == 0) &&
		fb_wayland_cache_valid) {
		elapsed_ns = ((int64_t)now.tv_sec - (int64_t)fb_wayland_cached_at.tv_sec) *
			1000000000LL + (int64_t)now.tv_nsec -
			(int64_t)fb_wayland_cached_at.tv_nsec;
		if ((elapsed_ns >= 0) && (elapsed_ns < FB_WAYLAND_CACHE_NS)) {
			*info = fb_wayland_cached_info;
			pthread_mutex_unlock(&fb_wayland_probe_mutex);
			return 0;
		}
	}

	result = fb_wayland_probe(info);
	if (result == 0) {
		fb_wayland_cached_info = *info;
		if (clock_gettime(CLOCK_MONOTONIC, &fb_wayland_cached_at) == 0)
			fb_wayland_cache_valid = 1;
	} else {
		fb_wayland_cache_valid = 0;
	}
	pthread_mutex_unlock(&fb_wayland_probe_mutex);
	return result;
}

/* end of fb_wayland_display.c */

