/*
 * Project: FreeBASIC Linux graphics tests
 * File: wayland-buffer-presentation.c
 * Purpose: Check changed rows across reusable Wayland presentation buffers.
 * Responsibilities: Simulate delayed compositor releases and pending drawing.
 * This file intentionally does NOT connect to a compositor or create a window.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx/wayland-buffer-presentation.c -Wl,--gc-sections \
 *      -ldl -lm -lrt -o /tmp/wayland-buffer-presentation
 *   /tmp/wayland-buffer-presentation
 */
#include "../../src/gfxlib2/drivers/linux/gfx_driver_wayland.c"

#include <stdarg.h>

FBGFX *__fb_gfx;

/* Both buffers are preallocated here; the allocation path must not be needed. */
int fb_hWaylandLoadSymbols(const char *const *names, void **addresses, size_t count)
{
	(void)names;
	(void)addresses;
	(void)count;
	return -1;
}

static struct wl_buffer *last_attached;
static int attach_count;
static int callback_count;
static int callback_destroy_count;
static int fail_callback_creation;
static int fail_callback_listener;

static struct wl_proxy *create_frame_callback(struct wl_proxy *proxy,
	uint32_t opcode, const struct wl_interface *interface, uint32_t version, ...)
{
	(void)proxy;
	(void)interface;
	(void)version;
	if (opcode != FB_WL_SURFACE_FRAME || fail_callback_creation)
		return NULL;
	callback_count++;
	return (struct wl_proxy *)(uintptr_t)(100 + callback_count);
}

static int record_listener(struct wl_proxy *proxy, void (**listener)(void),
	void *data)
{
	(void)proxy;
	(void)listener;
	(void)data;
	return fail_callback_listener ? -1 : 0;
}

static void destroy_callback(struct wl_proxy *proxy)
{
	(void)proxy;
	callback_destroy_count++;
}

static void record_request(struct wl_proxy *proxy, uint32_t opcode, ...)
{
	va_list arguments;
	(void)proxy;
	if (opcode != FB_WL_SURFACE_ATTACH)
		return;
	va_start(arguments, opcode);
	last_attached = va_arg(arguments, struct wl_buffer *);
	va_end(arguments);
	attach_count++;
}

static int record_flush(struct wl_display *display)
{
	(void)display;
	return 0;
}

static void copy_dirty_rows(unsigned char *destination, int pitch)
{
	int y;
	for (y = 0; y < __fb_gfx->h; y++) {
		if (__fb_gfx->dirty[y])
			memcpy(destination + (size_t)y * (size_t)pitch,
				__fb_gfx->framebuffer + (size_t)y * sizeof(uint32_t),
				sizeof(uint32_t));
	}
}

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "wayland-buffer-presentation: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	static const FB_WAYLAND_API api = {
		.display_flush = record_flush,
		.proxy_marshal = record_request,
		.proxy_marshal_constructor_versioned = create_frame_callback,
		.proxy_add_listener = record_listener,
		.proxy_destroy = destroy_callback
	};
	static const struct wl_interface callback_interface = { 0 };
	FBGFX gfx = { 0 };
	uint32_t source[3] = { 1, 2, 3 };
	uint32_t pixels[2][3] = { { 0 }, { 0 } };
	char pending[3] = { 1, 1, 1 };
	char buffer_dirty[2][3] = { { 1, 1, 1 }, { 1, 1, 1 } };
	struct wl_buffer *first = (struct wl_buffer *)(uintptr_t)1;
	struct wl_buffer *second = (struct wl_buffer *)(uintptr_t)2;
	int i;

	__fb_gfx = &gfx;
	gfx.framebuffer = (unsigned char *)source;
	gfx.dirty = pending;
	gfx.h = 3;
	memset(&fb_wayland_window, 0, sizeof(fb_wayland_window));
	fb_wayland_window.api = &api;
	fb_wayland_window.callback_interface = &callback_interface;
	fb_wayland_window.surface = (struct wl_surface *)(uintptr_t)3;
	fb_wayland_window.display = (struct wl_display *)(uintptr_t)4;
	fb_wayland_window.width = 1;
	fb_wayland_window.height = 3;
	fb_wayland_window.stride = (int)sizeof(uint32_t);
	fb_wayland_window.active = 1;
	fb_wayland_window.configured = 1;
	fb_wayland_window.blitter = copy_dirty_rows;
	for (i = 0; i < 2; i++) {
		FB_WAYLAND_BUFFER *slot = &fb_wayland_window.buffers[i];
		slot->buffer = i == 0 ? first : second;
		slot->pixels = pixels[i];
		slot->dirty = buffer_dirty[i];
		slot->height = 3;
	}

	fb_wayland_window.repaint_pending = 1;
	gfx.lock_count = 1;
	fb_wayland_present();
	require(attach_count == 0 && fb_wayland_window.repaint_pending && pending[0],
		"a partially drawn locked frame was submitted");
	gfx.lock_count = 0;
	fb_wayland_present();
	require(last_attached == first && attach_count == 1, "first frame missing");
	require(memcmp(pixels[0], source, sizeof(source)) == 0, "first frame differs");

	source[0] = 10;
	pending[0] = 1;
	fb_wayland_window.repaint_pending = 1;
	fb_wayland_present();
	require(attach_count == 1 && fb_wayland_window.repaint_pending,
		"frame was submitted before compositor callback");
	fb_wayland_frame_done(&fb_wayland_window, fb_wayland_window.frame_callback, 0);
	fb_wayland_present();
	require(last_attached == second && attach_count == 2, "second buffer not used");
	require(memcmp(pixels[1], source, sizeof(source)) == 0, "second frame differs");

	fb_wayland_frame_done(&fb_wayland_window, fb_wayland_window.frame_callback, 0);
	source[1] = 20;
	pending[1] = 1;
	fb_wayland_window.repaint_pending = 1;
	fb_wayland_present();
	require(attach_count == 2 && fb_wayland_window.repaint_pending,
		"busy buffers lost a pending frame");
	fb_wayland_buffer_release(&fb_wayland_window, first);
	fb_wayland_present();
	require(last_attached == first && attach_count == 3, "released buffer not reused");
	require(memcmp(pixels[0], source, sizeof(source)) == 0,
		"reused buffer missed changes made while it was busy");

	fb_wayland_frame_done(&fb_wayland_window, fb_wayland_window.frame_callback, 0);
	fb_wayland_buffer_release(&fb_wayland_window, second);
	source[2] = 30;
	pending[2] = 1;
	fb_wayland_window.repaint_pending = 1;
	fb_wayland_present();
	require(last_attached == second && attach_count == 4, "second buffer not reused");
	require(memcmp(pixels[1], source, sizeof(source)) == 0,
		"second buffer missed changes made while it was busy");
	require(gfx.dirty == pending, "blitter did not restore the shared dirty mask");
	fb_wayland_frame_done(&fb_wayland_window, fb_wayland_window.frame_callback, 0);
	require(callback_count == 4 && callback_destroy_count == 4,
		"frame callbacks were not paired with cleanup");

	/* Allocation failures must retain drawing for the next successful frame. */
	fb_wayland_buffer_release(&fb_wayland_window, first);
	source[0] = 40;
	pending[0] = 1;
	fb_wayland_window.repaint_pending = 1;
	fail_callback_creation = 1;
	fb_wayland_present();
	require(attach_count == 4 && fb_wayland_window.repaint_pending,
		"callback allocation failure lost a frame");
	fail_callback_creation = 0;
	fail_callback_listener = 1;
	fb_wayland_present();
	require(attach_count == 4 && callback_destroy_count == 5 &&
		fb_wayland_window.repaint_pending,
		"callback listener failure lost a frame or leaked its proxy");
	fail_callback_listener = 0;
	fb_wayland_present();
	require(attach_count == 5 && pixels[0][0] == 40,
		"drawing was not preserved across callback failures");
	fb_wayland_frame_done(&fb_wayland_window, fb_wayland_window.frame_callback, 0);
	require(callback_count == 6 && callback_destroy_count == 6,
		"retry callback was not cleaned up");

	/* A resized buffer must not wait for a callback on the old buffer. */
	fb_wayland_buffer_release(&fb_wayland_window, first);
	fb_wayland_buffer_release(&fb_wayland_window, second);
	source[0] = 50;
	pending[0] = 1;
	fb_wayland_window.repaint_pending = 1;
	fb_wayland_present();
	require(attach_count == 6 && callback_count == 7,
		"frame before resize was not submitted");
	source[1] = 60;
	pending[1] = 1;
	fb_wayland_window.repaint_pending = 1;
	fb_wayland_window.resize_present_pending = 1;
	gfx.lock_count = 1;
	fb_wayland_present();
	require(attach_count == 6 && fb_wayland_window.resize_present_pending,
		"resize submitted a partially drawn locked frame");
	gfx.lock_count = 0;
	fb_wayland_present();
	require(attach_count == 7 && last_attached == second && callback_count == 7 &&
		!fb_wayland_window.resize_present_pending && pixels[1][1] == 60,
		"resize did not reuse the outstanding frame callback");
	fb_wayland_frame_done(&fb_wayland_window, fb_wayland_window.frame_callback, 0);
	require(callback_destroy_count == 7, "resize callback was not cleaned up");

	puts("WAYLAND_BUFFER_PRESENTATION_PASS");
	return EXIT_SUCCESS;
}

/* end of wayland-buffer-presentation.c */
