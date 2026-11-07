/*
 * Project: FreeBASIC Linux graphics tests
 * File: wayland-display-cache.c
 * Purpose: Check persistent Wayland display metrics and reconnection.
 * Responsibilities: Simulate output changes, window setup and lost connections.
 * This file intentionally does NOT connect to a compositor or open a window.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx/wayland-display-cache.c -Wl,--gc-sections -lm \
 *      -o /tmp/wayland-display-cache
 *   /tmp/wayland-display-cache
 */
#include "../../src/gfxlib2/drivers/linux/fb_wayland_display.c"

#include <limits.h>
#include <stdio.h>
#include <unistd.h>

static const struct wl_interface registry_interface = { .name = "wl_registry" };
static const struct wl_interface output_interface = { .name = "wl_output" };
static const struct wl_interface seat_interface = { .name = "wl_seat" };
static const FB_WAYLAND_REGISTRY_LISTENER *registry_listener;
static const FB_WAYLAND_OUTPUT_LISTENER *output_listener;
static const FB_WAYLAND_SEAT_LISTENER *seat_listener;
static const FB_WAYLAND_XDG_OUTPUT_LISTENER *xdg_output_listener;
static void *registry_data;
static void *output_data;
static void *seat_data;
static void *xdg_output_data;
static int connection_count;
static int disconnect_count;
static int output_release_count;
static int seat_release_count;
static int xdg_output_release_count;
static int xdg_manager_release_count;
static int xdg_output_destroy_count;
static int xdg_manager_destroy_count;
static int roundtrip_phase;
static int rotate_pending;
static int fractional_pending;
static int fractional_done_pending;
static int invalid_logical_pending;
static int manager_remove_pending;
static int manager_return_pending;
static int extreme_pending;
static int fail_flush;
static int advertise_xdg = 1;
static int pipe_fds[2];

static struct wl_display *mock_connect(const char *name)
{
	(void)name;
	connection_count++;
	roundtrip_phase = 0;
	xdg_output_listener = NULL;
	return (struct wl_display *)(uintptr_t)1;
}

static void mock_disconnect(struct wl_display *display)
{
	(void)display;
	disconnect_count++;
}

static struct wl_proxy *mock_constructor(struct wl_proxy *proxy, uint32_t opcode,
	const struct wl_interface *interface, uint32_t version, ...)
{
	(void)proxy;
	(void)opcode;
	(void)version;
	if (interface == &registry_interface)
		return (struct wl_proxy *)(uintptr_t)2;
	if (interface == &output_interface)
		return (struct wl_proxy *)(uintptr_t)3;
	if (interface == &seat_interface)
		return (struct wl_proxy *)(uintptr_t)4;
	if (strcmp(interface->name, "zxdg_output_manager_v1") == 0)
		return (struct wl_proxy *)(uintptr_t)5;
	if (strcmp(interface->name, "zxdg_output_v1") == 0)
		return (struct wl_proxy *)(uintptr_t)6;
	return NULL;
}

static uint32_t mock_version(struct wl_proxy *proxy)
{
	if (proxy == (struct wl_proxy *)(uintptr_t)3)
		return 3;
	if (proxy == (struct wl_proxy *)(uintptr_t)4)
		return 5;
	return 1;
}

static void mock_marshal(struct wl_proxy *proxy, uint32_t opcode, ...)
{
	if (proxy == (struct wl_proxy *)(uintptr_t)3 &&
	    opcode == FB_WL_OUTPUT_RELEASE)
		output_release_count++;
	if (proxy == (struct wl_proxy *)(uintptr_t)4 &&
	    opcode == FB_WL_SEAT_RELEASE)
		seat_release_count++;
	if (proxy == (struct wl_proxy *)(uintptr_t)6 && opcode == 0)
		xdg_output_release_count++;
	if (proxy == (struct wl_proxy *)(uintptr_t)5 && opcode == 0)
		xdg_manager_release_count++;
}

static int mock_add_listener(struct wl_proxy *proxy,
	void (**implementation)(void), void *data)
{
	if (proxy == (struct wl_proxy *)(uintptr_t)2) {
		registry_listener = (const FB_WAYLAND_REGISTRY_LISTENER *)implementation;
		registry_data = data;
	} else if (proxy == (struct wl_proxy *)(uintptr_t)3) {
		output_listener = (const FB_WAYLAND_OUTPUT_LISTENER *)implementation;
		output_data = data;
	} else if (proxy == (struct wl_proxy *)(uintptr_t)4) {
		seat_listener = (const FB_WAYLAND_SEAT_LISTENER *)implementation;
		seat_data = data;
	} else if (proxy == (struct wl_proxy *)(uintptr_t)6) {
		xdg_output_listener = (const FB_WAYLAND_XDG_OUTPUT_LISTENER *)implementation;
		xdg_output_data = data;
	}
	return 0;
}

static int mock_roundtrip(struct wl_display *display)
{
	(void)display;
	roundtrip_phase++;
	if (roundtrip_phase == 1) {
		/* Registry global order is not guaranteed by Wayland. */
		if (advertise_xdg && connection_count % 2 != 0)
			registry_listener->global(registry_data, (struct wl_registry *)(uintptr_t)2,
				13, "zxdg_output_manager_v1", 3);
		registry_listener->global(registry_data, (struct wl_registry *)(uintptr_t)2,
			10, "wl_output", 3);
		if (advertise_xdg && connection_count % 2 == 0)
			registry_listener->global(registry_data, (struct wl_registry *)(uintptr_t)2,
				13, "zxdg_output_manager_v1", 3);
		registry_listener->global(registry_data, (struct wl_registry *)(uintptr_t)2,
			11, "wl_seat", 5);
		registry_listener->global(registry_data, (struct wl_registry *)(uintptr_t)2,
			12, "zwp_text_input_manager_v3", 1);
	} else if (roundtrip_phase == 2) {
		output_listener->geometry(output_data, (struct wl_output *)(uintptr_t)3,
			0, 0, 100, 200, 0, "test", "test", 0);
		output_listener->mode(output_data, (struct wl_output *)(uintptr_t)3,
			FB_WL_OUTPUT_MODE_CURRENT, 1000, 2000, 60000);
		output_listener->scale(output_data, (struct wl_output *)(uintptr_t)3, 2);
		seat_listener->capabilities(seat_data, (struct wl_seat *)(uintptr_t)4,
			FB_WL_SEAT_CAPABILITY_TOUCH);
		if (xdg_output_listener) {
			xdg_output_listener->logical_size(xdg_output_data, (void *)(uintptr_t)6,
				500, 1000);
			xdg_output_listener->done(xdg_output_data, (void *)(uintptr_t)6);
		}
	}
	return 0;
}

static int mock_dispatch(struct wl_display *display)
{
	(void)display;
	if (rotate_pending) {
		output_listener->geometry(output_data, (struct wl_output *)(uintptr_t)3,
			0, 0, 100, 200, 0, "test", "test", FB_WL_OUTPUT_TRANSFORM_90);
		if (xdg_output_listener) {
			xdg_output_listener->logical_size(xdg_output_data, (void *)(uintptr_t)6,
				1600, 800);
			xdg_output_listener->done(xdg_output_data, (void *)(uintptr_t)6);
		}
		rotate_pending = 0;
	}
	if (fractional_pending) {
		if (xdg_output_listener)
			xdg_output_listener->logical_size(xdg_output_data, (void *)(uintptr_t)6,
				800, 1600);
		fractional_pending = 0;
	}
	if (fractional_done_pending) {
		if (xdg_output_listener)
			xdg_output_listener->done(xdg_output_data, (void *)(uintptr_t)6);
		fractional_done_pending = 0;
	}
	if (invalid_logical_pending) {
		if (xdg_output_listener) {
			xdg_output_listener->logical_size(xdg_output_data, (void *)(uintptr_t)6,
				0, -1);
			xdg_output_listener->done(xdg_output_data, (void *)(uintptr_t)6);
		}
		invalid_logical_pending = 0;
	}
	if (manager_remove_pending) {
		registry_listener->global_remove(registry_data,
			(struct wl_registry *)(uintptr_t)2, 13);
		xdg_output_listener = NULL;
		manager_remove_pending = 0;
	}
	if (manager_return_pending) {
		registry_listener->global(registry_data, (struct wl_registry *)(uintptr_t)2,
			14, "zxdg_output_manager_v1", 3);
		if (xdg_output_listener) {
			xdg_output_listener->logical_size(xdg_output_data, (void *)(uintptr_t)6,
				1600, 800);
			xdg_output_listener->done(xdg_output_data, (void *)(uintptr_t)6);
		}
		manager_return_pending = 0;
	}
	if (extreme_pending) {
		output_listener->geometry(output_data, (struct wl_output *)(uintptr_t)3,
			0, 0, 1, 1, 0, "test", "test", 0);
		output_listener->mode(output_data, (struct wl_output *)(uintptr_t)3,
			FB_WL_OUTPUT_MODE_CURRENT, INT_MAX, INT_MAX, INT_MAX);
		output_listener->scale(output_data, (struct wl_output *)(uintptr_t)3, 1);
		extreme_pending = 0;
	}
	return 0;
}

static int mock_prepare(struct wl_display *display) { (void)display; return 0; }
static int mock_read(struct wl_display *display) { (void)display; return 0; }
static void mock_cancel(struct wl_display *display) { (void)display; }
static int mock_fd(struct wl_display *display) { (void)display; return pipe_fds[0]; }
static int mock_flush(struct wl_display *display)
{
	(void)display;
	if (fail_flush) {
		errno = EPIPE;
		return -1;
	}
	return 0;
}
static void mock_destroy(struct wl_proxy *proxy)
{
	if (proxy == (struct wl_proxy *)(uintptr_t)6)
		xdg_output_destroy_count++;
	if (proxy == (struct wl_proxy *)(uintptr_t)5)
		xdg_manager_destroy_count++;
}

static const FB_WAYLAND_API mock_api = {
	.display_connect = mock_connect,
	.display_disconnect = mock_disconnect,
	.display_roundtrip = mock_roundtrip,
	.display_dispatch_pending = mock_dispatch,
	.display_get_fd = mock_fd,
	.display_flush = mock_flush,
	.display_prepare_read = mock_prepare,
	.display_read_events = mock_read,
	.display_cancel_read = mock_cancel,
	.proxy_marshal = mock_marshal,
	.proxy_marshal_constructor_versioned = mock_constructor,
	.proxy_get_version = mock_version,
	.proxy_add_listener = mock_add_listener,
	.proxy_destroy = mock_destroy
};

int fb_hWaylandShouldUse(void) { return 1; }
int fb_hWaylandGetApi(const FB_WAYLAND_API **api) { *api = &mock_api; return 0; }
int fb_hWaylandInitProtocols(void) { return 0; }
int fb_hWaylandLoadSymbols(const char *const *names, void **addresses, size_t count)
{
	if (count != 3 || strcmp(names[0], "wl_registry_interface") != 0)
		return -1;
	addresses[0] = (void *)&registry_interface;
	addresses[1] = (void *)&output_interface;
	addresses[2] = (void *)&seat_interface;
	return 0;
}

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "wayland-display-cache: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

static void expire_cache(void)
{
	fb_wayland_cached_at.tv_sec--;
}

int main(void)
{
	FB_WAYLAND_DISPLAY_INFO info;
	struct wl_display *window_display;
	int i;

	require(pipe(pipe_fds) == 0, "pipe creation failed");
	require(fb_hWaylandDisplayInfo(&info) == 0, "initial query failed");
	require(info.width == 500 && info.height == 1000 && info.dpi == 127 &&
		info.rotation == 0 && info.refresh == 60 && info.touch_capable &&
		info.text_input_available, "initial output metrics differ");
	for (i = 0; i < 4; i++) {
		expire_cache();
		require(fb_hWaylandDisplayInfo(&info) == 0, "reused query failed");
	}
	require(connection_count == 1 && disconnect_count == 0,
		"output queries reopened the display connection");
	require(fb_hWaylandDisplayBeginWindow(&window_display) == 0,
		"window setup could not reserve the metrics connection");
	require(window_display == (struct wl_display *)(uintptr_t)1 &&
		disconnect_count == 0 && !fb_wayland_cache_valid &&
		fb_wayland_probe_display == NULL,
		"window setup did not transfer the metrics connection");
	require(output_release_count == 1 && seat_release_count == 1,
		"transferred connection retained its metrics objects");
	require(pthread_mutex_trylock(&fb_wayland_probe_mutex) == EBUSY,
		"metrics queries were not held during window setup");
	fb_hWaylandDisplayEndWindow();
	require(xdg_output_release_count == 1 && xdg_manager_release_count == 1,
		"transferred connection retained xdg-output objects");
	require(xdg_output_destroy_count == 1 && xdg_manager_destroy_count == 1,
		"transferred connection retained xdg-output proxies");
	mock_disconnect(window_display);
	require(fb_hWaylandDisplayInfo(&info) == 0 && connection_count == 2,
		"metrics did not reconnect after window setup");

	/* wl_output.scale is integer even when the output uses fractional scale. */
	fractional_pending = 1;
	expire_cache();
	require(fb_hWaylandDisplayInfo(&info) == 0 && info.width == 500 &&
		info.height == 1000, "uncommitted logical dimensions were used");
	fractional_done_pending = 1;
	expire_cache();
	require(fb_hWaylandDisplayInfo(&info) == 0 && info.width == 800 &&
		info.height == 1600 && info.dpi == 203,
		"fractional logical size or density was lost");
	invalid_logical_pending = 1;
	expire_cache();
	require(fb_hWaylandDisplayInfo(&info) == 0 && info.width == 800 &&
		info.height == 1600, "invalid logical dimensions replaced valid ones");

	rotate_pending = 1;
	expire_cache();
	require(fb_hWaylandDisplayInfo(&info) == 0 && info.width == 1600 &&
		info.height == 800 && info.rotation == 90 && info.dpi == 203,
		"rotation update was not observed");
	manager_remove_pending = 1;
	expire_cache();
	require(fb_hWaylandDisplayInfo(&info) == 0 && info.width == 1000 &&
		info.height == 500 && info.dpi == 127,
		"removed xdg-output manager did not fall back to core output");
	require(xdg_output_release_count == 2 && xdg_manager_release_count == 2 &&
		xdg_output_destroy_count == 2 && xdg_manager_destroy_count == 2,
		"removed xdg-output manager retained its objects");
	manager_return_pending = 1;
	expire_cache();
	require(fb_hWaylandDisplayInfo(&info) == 0 && info.width == 1600 &&
		info.height == 800 && info.dpi == 203,
		"returning xdg-output manager was not rebound");
	extreme_pending = 1;
	expire_cache();
	require(fb_hWaylandDisplayInfo(&info) == 0 && info.dpi == 96 &&
		info.refresh == 2147484, "extreme output values were not bounded");

	fail_flush = 1;
	expire_cache();
	require(fb_hWaylandDisplayInfo(&info) == -1 && disconnect_count == 2,
		"broken connection was not closed");
	fail_flush = 0;
	require(fb_hWaylandDisplayInfo(&info) == 0 && connection_count == 3,
		"display did not reconnect after failure");
	require(xdg_output_release_count == 2 && xdg_manager_release_count == 2 &&
		xdg_output_destroy_count == 3 && xdg_manager_destroy_count == 3,
		"broken connection sent protocol requests during cleanup");
	fb_wayland_probe_cleanup();
	require(disconnect_count == 3, "live connection was not cleaned up");
	advertise_xdg = 0;
	require(fb_hWaylandDisplayInfo(&info) == 0 && info.width == 500 &&
		info.height == 1000 && info.dpi == 127,
		"core output fallback failed without xdg-output");
	fb_wayland_probe_cleanup();
	require(fb_hWaylandDisplayBeginWindow(NULL) == -1,
		"window setup accepted a missing connection destination");
	require(fb_hWaylandDisplayBeginWindow(&window_display) == 0 &&
		window_display == NULL, "empty probe unexpectedly supplied a connection");
	fb_hWaylandDisplayEndWindow();
	close(pipe_fds[0]);
	close(pipe_fds[1]);
	puts("WAYLAND_DISPLAY_CACHE_PASS");
	return EXIT_SUCCESS;
}

/* end of wayland-display-cache.c */
