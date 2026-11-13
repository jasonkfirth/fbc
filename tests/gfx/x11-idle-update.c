/*
 * Project: FreeBASIC gfxlib2 X11 tests
 * File: x11-idle-update.c
 * Purpose: Avoid framebuffer conversion and X transfers without dirty rows.
 * Responsibilities: Exercise the production updater's clean and dirty paths.
 * This file does not open a display or benchmark physical hardware.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx/x11-idle-update.c -Wl,--gc-sections -lX11 -lXext \
 *      -o /tmp/x11-idle-update
 *   /tmp/x11-idle-update
 */
#include "../../src/gfxlib2/drivers/unix/gfx_driver_x11.c"
#include <stdio.h>
#include <stdlib.h>

FBGFX *fb_gfx;
X11DRIVER fb_x11;
void *(*fb_hMemCpy)(void *, const void *, size_t) = memcpy;
void *(*fb_hMemSet)(void *, int, size_t) = memset;
static int conversions, transfers, last_y, last_height;

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "x11-idle-update: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

void fb_hX11RefreshLayout(int width, int height)
{
	(void)width;
	(void)height;
}

static void record_conversion(unsigned char *pixels, int pitch)
{
	(void)pixels;
	require(pitch == 8, "unexpected image pitch");
	conversions++;
}

int XPutImage(Display *display, Drawable drawable, GC gc, XImage *pixels,
	int src_x, int src_y, int dst_x, int dst_y,
	unsigned int width, unsigned int height)
{
	(void)display;
	(void)drawable;
	(void)gc;
	(void)pixels;
	(void)src_x;
	(void)dst_x;
	require(width == 2 && src_y == dst_y, "dirty region coordinates changed");
	transfers++;
	last_y = src_y;
	last_height = (int)height;
	return 0;
}

Bool XShmPutImage(Display *display, Drawable drawable, GC gc, XImage *pixels,
	int src_x, int src_y, int dst_x, int dst_y,
	unsigned int width, unsigned int height, Bool send_event)
{
	(void)send_event;
	XPutImage(display, drawable, gc, pixels, src_x, src_y, dst_x, dst_y,
		width, height);
	return True;
}

int main(void)
{
	FBGFX graphics;
	XImage pixels;
	char dirty[3] = { 0, 0, 0 };
	unsigned char storage[24] = { 0 };
	memset(&graphics, 0, sizeof(graphics));
	memset(&pixels, 0, sizeof(pixels));
	fb_gfx = &graphics;
	graphics.dirty = dirty;
	graphics.pitch = 8;
	fb_x11.w = 2;
	fb_x11.h = 3;
	fb_x11.scale = 1;
	pixels.bytes_per_line = 8;
	pixels.bits_per_pixel = 32;
	pixels.data = (char *)storage;
	image = &pixels;
	blitter = record_conversion;

	x11_update();
	require(conversions == 0 && transfers == 0,
		"clean framebuffer was converted or transferred");
	dirty[1] = TRUE;
	x11_update();
	require(conversions == 1 && transfers == 1 && last_y == 1 && last_height == 1,
		"single dirty row was not presented");
	require(!dirty[0] && !dirty[1] && !dirty[2], "dirty flags were not cleared");
	dirty[0] = dirty[2] = TRUE;
	is_shm = TRUE;
	x11_update();
	require(conversions == 2 && transfers == 3 && last_y == 2 && last_height == 1,
		"separate dirty runs or final scanline were lost");
	x11_update();
	require(conversions == 2 && transfers == 3,
		"clean shared-memory frame performed presentation work");
	puts("X11_IDLE_UPDATE=PASS");
	return EXIT_SUCCESS;
}

/* end of x11-idle-update.c */
