/*
 * Project: FreeBASIC gfxlib3 Linux tests
 * File: wayland-window-title.c
 * Purpose: Check Wayland window-title flush results.
 * Responsibilities: Cover successful writes, temporary backpressure, and errors.
 * This file does not connect to a compositor or create a window.
 *
 * Build on Linux with:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx3/wayland-window-title.c -Wl,--gc-sections -ldl -lm \
 *      -o /tmp/wayland-window-title
 */
#include "../../src/gfxlib3/drivers/linux/gfx3_platform_wayland.c"
#include <errno.h>

static int flush_result;
static int flush_error;
static int title_requests;

static void mock_marshal(struct wl_proxy *proxy, uint32_t opcode, ...)
{
	(void)proxy;
	if (opcode == FB_GFX3_XDG_TOPLEVEL_SET_TITLE)
		title_requests++;
}

static int mock_flush(struct wl_display *display)
{
	(void)display;
	errno = flush_error;
	return flush_result;
}

int main(void)
{
	FB_GFX3_PLATFORM_WAYLAND platform;
	FB_WAYLAND_API api;

	memset(&platform, 0, sizeof(platform));
	memset(&api, 0, sizeof(api));
	api.proxy_marshal = mock_marshal;
	api.display_flush = mock_flush;
	platform.api = &api;
	platform.display = (struct wl_display *)(uintptr_t)1;
	platform.toplevel = (struct wl_proxy *)(uintptr_t)1;

	flush_result = 12;
	if (platform_wayland_set_window_title(&platform, "test") != FB_GFX3_OK)
		return EXIT_FAILURE;
	flush_result = 0;
	if (platform_wayland_set_window_title(&platform, "test") != FB_GFX3_OK)
		return EXIT_FAILURE;
	flush_result = -1;
	flush_error = EAGAIN;
	if (platform_wayland_set_window_title(&platform, "test") != FB_GFX3_OK)
		return EXIT_FAILURE;
	flush_error = EPIPE;
	if (platform_wayland_set_window_title(&platform, "test") != FB_GFX3_FAILED)
		return EXIT_FAILURE;
	if (title_requests != 4)
		return EXIT_FAILURE;
	puts("GFX3_WAYLAND_WINDOW_TITLE_PASS");
	return EXIT_SUCCESS;
}

/* end of wayland-window-title.c */
