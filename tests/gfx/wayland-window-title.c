/*
 * Project: FreeBASIC gfxlib2 Linux tests
 * File: wayland-window-title.c
 * Purpose: Check title changes after Wayland window creation.
 * Responsibilities: Cover xdg-shell, legacy wl_shell, and inactive windows.
 * This file does not connect to a compositor or create a native window.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx/wayland-window-title.c -Wl,--gc-sections \
 *      -ldl -lm -o /tmp/wayland-window-title
 *   /tmp/wayland-window-title
 */
#include "../../src/gfxlib2/drivers/linux/gfx_driver_wayland.c"
#include <errno.h>
#include <stdarg.h>

static int request_count;
static int flush_count;
static uint32_t request_opcode;
static struct wl_proxy *request_target;
static const char *request_title;

static void record_request(struct wl_proxy *target, uint32_t opcode, ...)
{
	va_list args;
	request_count++;
	request_target = target;
	request_opcode = opcode;
	va_start(args, opcode);
	request_title = va_arg(args, const char *);
	va_end(args);
}

static int record_flush(struct wl_display *display)
{
	(void)display;
	flush_count++;
	errno = EAGAIN;
	return -1;
}

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "wayland-window-title: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	FB_WAYLAND_API api;
	struct wl_proxy *target = (struct wl_proxy *)(uintptr_t)1;
	memset(&api, 0, sizeof(api));
	memset(&fb_wayland_window, 0, sizeof(fb_wayland_window));
	api.proxy_marshal = record_request;
	api.display_flush = record_flush;
	fb_wayland_window.api = &api;
	fb_wayland_window.display = (struct wl_display *)target;
	fb_wayland_window.toplevel = (struct xdg_toplevel *)target;

	fb_wayland_driver_set_window_title("inactive");
	require(request_count == 0 && flush_count == 0,
		"inactive window sent a title request");
	fb_wayland_window.active = 1;
	fb_wayland_driver_set_window_title("Templari");
	require(request_count == 1 && flush_count == 1 &&
		request_target == target && request_opcode == FB_WL_XDG_TOPLEVEL_SET_TITLE &&
		strcmp(request_title, "Templari") == 0, "xdg-shell title was not updated");

	fb_wayland_window.toplevel = NULL;
	fb_wayland_window.shell_surface = (struct wl_shell_surface *)target;
	fb_wayland_driver_set_window_title("legacy");
	require(request_count == 2 && flush_count == 2 &&
		request_target == target && request_opcode == FB_WL_SHELL_SURFACE_SET_TITLE &&
		strcmp(request_title, "legacy") == 0, "wl_shell title was not updated");
	fb_wayland_driver_set_window_title(NULL);
	require(request_count == 3 && flush_count == 3 && request_title[0] == '\0',
		"null title was not cleared safely");
	fb_wayland_window.shell_surface = NULL;
	fb_wayland_driver_set_window_title("no role");
	require(request_count == 3 && flush_count == 3,
		"window without a role sent a title request");

	puts("wayland-window-title: PASS");
	return EXIT_SUCCESS;
}

/* end of wayland-window-title.c */
