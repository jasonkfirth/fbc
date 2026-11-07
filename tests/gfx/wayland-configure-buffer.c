/*
 * Project: FreeBASIC gfxlib2 Linux tests
 * File: wayland-configure-buffer.c
 * Purpose: Check configure acknowledgements against framebuffer transactions.
 * Responsibilities: Cover deferred resize, coalescing and fixed-size clients.
 * This file does not connect to a compositor or allocate presentation buffers.
 */
#include "../../src/gfxlib2/drivers/linux/gfx_driver_wayland.c"
#include <stdarg.h>

static FBGFX context;
FBGFX *__fb_gfx = &context;
static unsigned checks, requests, acknowledgements;
static int requested_width, requested_height;
static uint32_t acknowledged_serial;

void fb_hRequestResize(int width, int height)
{
	requests++;
	requested_width = width;
	requested_height = height;
}

static void require(int condition, const char *message)
{
	checks++;
	if (!condition) {
		fprintf(stderr, "wayland-configure-buffer: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

static void record_request(struct wl_proxy *target, uint32_t opcode, ...)
{
	va_list args;
	require(target == (struct wl_proxy *)fb_wayland_window.xdg_surface &&
		opcode == FB_WL_XDG_SURFACE_ACK_CONFIGURE, "unexpected protocol request");
	va_start(args, opcode);
	acknowledged_serial = va_arg(args, uint32_t);
	va_end(args);
	acknowledgements++;
}

static void resize_buffers(int width, int height)
{
	fb_wayland_window.width = width;
	fb_wayland_window.height = height;
	context.w = width;
	context.h = height;
}

static void configure(int width, int height, uint32_t serial)
{
	fb_wayland_toplevel_configure(NULL, NULL, width, height, NULL);
	fb_wayland_xdg_configure(NULL, fb_wayland_window.xdg_surface, serial);
}

int main(void)
{
	FB_WAYLAND_API api;
	memset(&api, 0, sizeof(api));
	memset(&fb_wayland_window, 0, sizeof(fb_wayland_window));
	api.proxy_marshal = record_request;
	fb_wayland_window.api = &api;
	fb_wayland_window.xdg_surface = (struct xdg_surface *)(uintptr_t)1;
	context.flags = DRIVER_RESIZABLE;
	resize_buffers(1280, 800);
	fb_wayland_toplevel_configure(NULL, NULL, 1280, 742, NULL);
	require(requests == 0 && acknowledgements == 0, "role event applied before transaction end");
	fb_wayland_xdg_configure(NULL, fb_wayland_window.xdg_surface, 7);
	require(requests == 1 && requested_width == 1280 && requested_height == 742 &&
		acknowledgements == 0, "resize acknowledged before buffers matched");
	require(!fb_wayland_ack_configure(&fb_wayland_window), "old buffer was accepted");
	fb_wayland_window.height = 742;
	require(!fb_wayland_ack_configure(&fb_wayland_window), "old framebuffer was accepted");
	context.h = 742;
	require(fb_wayland_ack_configure(&fb_wayland_window) &&
		acknowledgements == 1 && acknowledged_serial == 7, "ready configure not acknowledged");
	require(fb_wayland_ack_configure(&fb_wayland_window) && acknowledgements == 1,
		"configure acknowledged twice");
	configure(800, 1222, 8);
	configure(1024, 582, 9);
	resize_buffers(800, 1222);
	require(!fb_wayland_ack_configure(&fb_wayland_window), "superseded geometry was accepted");
	resize_buffers(1024, 582);
	require(fb_wayland_ack_configure(&fb_wayland_window) && acknowledged_serial == 9 &&
		acknowledgements == 2, "latest configure was not coalesced");
	context.flags = 0;
	unsigned previous_requests = requests;
	resize_buffers(640, 480);
	configure(900, 600, 10);
	require(requests == previous_requests && acknowledged_serial == 10 &&
		!fb_wayland_window.configure_pending, "fixed-size client was left waiting");
	context.flags = DRIVER_RESIZABLE;
	configure(0, 300, 11);
	require(requested_width == 640 && requested_height == 300 &&
		fb_wayland_window.configure_pending, "unspecified axis did not retain current size");
	resize_buffers(640, 300);
	require(fb_wayland_ack_configure(&fb_wayland_window) && acknowledged_serial == 11,
		"single-axis configure not acknowledged");
	configure(0, 0, 0);
	require(acknowledged_serial == 0 && !fb_wayland_window.configure_pending,
		"zero-size configure or wrapped serial was rejected");
	printf("wayland-configure-buffer: PASS checks=%u\n", checks);
	return EXIT_SUCCESS;
}

/* end of wayland-configure-buffer.c */
