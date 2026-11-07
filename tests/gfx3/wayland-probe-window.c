/*
 * Project: FreeBASIC gfxlib3 tests
 * File: wayland-probe-window.c
 * Purpose: Check capability discovery and Wayland window connection setup.
 * Responsibilities: Test the connection-free probe and simulated registry.
 * This file intentionally does NOT connect to a compositor or initialize EGL.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx3/wayland-probe-window.c -Wl,--gc-sections -ldl -lm \
 *      -o /tmp/wayland-probe-window
 *   /tmp/wayland-probe-window
 */
#include "../../src/gfxlib3/drivers/linux/gfx3_platform_wayland.c"

static struct wl_display *const test_display = (struct wl_display *)(uintptr_t)1;
static struct wl_proxy *const test_registry = (struct wl_proxy *)(uintptr_t)2;
static const struct wl_interface test_interface = { 0 };
static void *registry_data;
static int constructor_calls;
static int unexpected_constructor;
static int advertise_shell;
static int connect_calls;
static int missing_symbols;

static struct wl_display *connect_display(const char *name)
{
	(void)name;
	connect_calls++;
	return test_display;
}

static struct wl_proxy *create_proxy(struct wl_proxy *proxy, uint32_t opcode,
	const struct wl_interface *interface, uint32_t version, ...)
{
	(void)interface;
	(void)version;
	constructor_calls++;
	if (proxy != (struct wl_proxy *)test_display || opcode != 1u) {
		unexpected_constructor = 1;
		return NULL;
	}
	return test_registry;
}

static int add_listener(struct wl_proxy *proxy, void (**listener)(void), void *data)
{
	(void)listener;
	if (proxy != test_registry)
		return -1;
	registry_data = data;
	return 0;
}

static int roundtrip(struct wl_display *display)
{
	if (display != test_display || registry_data == NULL)
		return -1;
	platform_wayland_registry_global(registry_data, test_registry,
		1u, "wl_compositor", 4u);
	if (advertise_shell)
		platform_wayland_registry_global(registry_data, test_registry,
			2u, "xdg_wm_base", 1u);
	return 0;
}

int fb_hWaylandGetApi(const FB_WAYLAND_API **api)
{
	static const FB_WAYLAND_API mock_api = {
		.display_connect = connect_display,
		.display_roundtrip = roundtrip,
		.proxy_marshal_constructor_versioned = create_proxy,
		.proxy_add_listener = add_listener
	};
	*api = &mock_api;
	return 0;
}

int fb_hWaylandInitProtocols(void)
{
	return 0;
}

int fb_hWaylandLoadSymbols(const char *const *names, void **addresses, size_t count)
{
	size_t index;
	(void)names;
	if (missing_symbols)
		return -1;
	for (index = 0; index < count; index++)
		addresses[index] = (void *)&test_interface;
	return 0;
}

int main(void)
{
	FB_GFX3_PLATFORM_WAYLAND platform;
	/* The probe checks libraries without becoming a compositor session. */
	fb_gfx3_wayland_egl_cache_ready = TRUE;
	fb_gfx3_wayland_egl_cache_result = FB_GFX3_OK;
	if (platform_wayland_probe_opengl() != FB_GFX3_OK || connect_calls != 0) {
		fputs("Wayland capability probe opened a connection\n", stderr);
		return EXIT_FAILURE;
	}
	missing_symbols = 1;
	if (platform_wayland_probe_opengl() != FB_GFX3_UNSUPPORTED ||
	    connect_calls != 0) {
		fputs("Wayland capability probe accepted missing symbols\n", stderr);
		return EXIT_FAILURE;
	}
	missing_symbols = 0;
	fb_gfx3_wayland_egl_cache_result = FB_GFX3_UNSUPPORTED;
	if (platform_wayland_probe_opengl() != FB_GFX3_UNSUPPORTED ||
	    connect_calls != 0) {
		fputs("Wayland capability probe accepted missing EGL\n", stderr);
		return EXIT_FAILURE;
	}
	fb_gfx3_wayland_egl_cache_result = FB_GFX3_OK;

	memset(&platform, 0, sizeof(platform));
	advertise_shell = 1;
	if (platform_wayland_connect(&platform) != FB_GFX3_OK ||
	    constructor_calls != 1 || unexpected_constructor || platform.surface) {
		fputs("Wayland connection created a window or missed globals\n", stderr);
		return EXIT_FAILURE;
	}

	memset(&platform, 0, sizeof(platform));
	registry_data = NULL;
	constructor_calls = 0;
	advertise_shell = 0;
	if (platform_wayland_connect(&platform) != FB_GFX3_UNSUPPORTED ||
	    constructor_calls != 1 || unexpected_constructor || platform.surface) {
		fputs("Wayland connection accepted a missing shell\n", stderr);
		return EXIT_FAILURE;
	}
	puts("GFX3_WAYLAND_PROBE_WINDOW_PASS");
	return EXIT_SUCCESS;
}

/* end of wayland-probe-window.c */
