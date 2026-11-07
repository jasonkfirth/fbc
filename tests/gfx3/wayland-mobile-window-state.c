/*
 * Project: FreeBASIC gfxlib3 Linux tests
 * File: wayland-mobile-window-state.c
 * Purpose: Check the Wayland state requested for handheld windows.
 * Responsibilities: Verify desktop, handheld and fullscreen request policy.
 * This file intentionally does NOT connect to a compositor or create a window.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx3/wayland-mobile-window-state.c -Wl,--gc-sections \
 *      -ldl -lm -o /tmp/gfx3-wayland-mobile-window-state
 *   /tmp/gfx3-wayland-mobile-window-state
 */
#include "../../src/gfxlib3/drivers/linux/gfx3_platform_wayland.c"

#include <stdarg.h>
#include <stdio.h>

static int device_class;
static int request_count;
static uint32_t request_opcode;
static struct wl_proxy *request_target;
static void *fullscreen_output;

int fb_hLinuxDeviceClass(void)
{
	return device_class;
}

static void record_request(struct wl_proxy *target, uint32_t opcode, ...)
{
	request_target = target;
	request_opcode = opcode;
	request_count++;
	if (opcode == FB_GFX3_XDG_TOPLEVEL_SET_FULLSCREEN) {
		va_list args;
		va_start(args, opcode);
		fullscreen_output = va_arg(args, void *);
		va_end(args);
	}
}

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "gfx3-wayland-mobile-window-state: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	FB_GFX3_PLATFORM_WAYLAND platform;
	FB_WAYLAND_API api;
	struct wl_proxy *target = (struct wl_proxy *)(uintptr_t)1;

	memset(&platform, 0, sizeof(platform));
	memset(&api, 0, sizeof(api));
	api.proxy_marshal = record_request;
	platform.api = &api;
	platform.toplevel = target;
	platform.flags = FB_GFX3_WINDOW_RESIZABLE;

	platform_wayland_request_toplevel_state(NULL);
	device_class = FB_GFX_DEVICE_CLASS_DESKTOP;
	platform_wayland_request_toplevel_state(&platform);
	require(request_count == 0, "desktop window was maximized");
	device_class = FB_GFX_DEVICE_CLASS_UNKNOWN;
	platform_wayland_request_toplevel_state(&platform);
	require(request_count == 0, "unknown device was maximized");
	device_class = FB_GFX_DEVICE_CLASS_HANDHELD;
	platform.flags = 0;
	platform_wayland_request_toplevel_state(&platform);
	require(request_count == 0, "fixed-size window was maximized");
	platform.flags = FB_GFX3_WINDOW_RESIZABLE;
	platform_wayland_request_toplevel_state(&platform);
	require(request_count == 1 && request_target == target &&
		request_opcode == FB_GFX3_XDG_TOPLEVEL_SET_MAXIMIZED,
		"handheld resizable window was not maximized");
	fullscreen_output = target;
	platform.flags = FB_GFX3_WINDOW_FULLSCREEN | FB_GFX3_WINDOW_RESIZABLE;
	platform_wayland_request_toplevel_state(&platform);
	require(request_count == 2 && request_target == target &&
		request_opcode == FB_GFX3_XDG_TOPLEVEL_SET_FULLSCREEN &&
		fullscreen_output == NULL, "fullscreen request lost precedence");

	puts("gfx3-wayland-mobile-window-state: PASS");
	return EXIT_SUCCESS;
}

/* end of wayland-mobile-window-state.c */
