/*
 * Project: FreeBASIC DOS graphics qualification
 * File: cursor_capture.c
 * Purpose: Observe the framebuffer submitted by the real IRQ0 cursor path.
 * Responsibilities: Substitute a locked scanline or bank-span presentation
 * target, retain its pixels and count updates. Foreground callers must hold
 * ScreenLock while installing, reading pixels or restoring the real driver.
 * This file does not implement cursor drawing or synthesize input events.
 */
#include "fb_gfx.h"
#include "fb_gfx_dos.h"
#include <go32.h>
#include <stdint.h>

static struct {
	unsigned char *pixels;
	void (*update)(void);
	int width, height, pitch, bpp, banked;
	volatile unsigned int updates;
} capture;

static void capture_update(void)
{
	int first = capture.height, last = -1, row;
	if (capture.banked) {
		for (row = 0; row < capture.height; row++) {
			if (__fb_gfx->dirty[row]) {
				if (first == capture.height) first = row;
				last = row;
			}
		}
	}
	for (row = 0; row < capture.height; row++) {
		if (__fb_gfx->dirty[row] || (row >= first && row <= last))
			memcpy(capture.pixels + row * capture.pitch,
				__fb_gfx->framebuffer + row * capture.pitch, capture.pitch);
	}
	capture.updates++;
}
static void capture_update_end(void) { }

int cursor_test_install(void)
{
	size_t size;
	int length = (int)((uintptr_t)capture_update_end - (uintptr_t)capture_update);
	if (!__fb_gfx || capture.pixels || __fb_gfx->w != 640 || __fb_gfx->h != 480 ||
		__fb_gfx->pitch < 1 || __fb_gfx->pitch > 8192 ||
		__fb_gfx->bpp < 1 || __fb_gfx->bpp > 4 || length < 1 || length > 65536)
		return 0;
	size = (size_t)__fb_gfx->h * __fb_gfx->pitch;
	/* The test uses 640x480; reject an unexpected mode before allocating. */
	if (size > 8u * 1024u * 1024u) return 0;
	capture.pixels = calloc(1, size);
	if (!capture.pixels) return 0;
	if (fb_dos_lock_data(&capture, sizeof(capture)) != 0) goto fail;
	if (fb_dos_lock_data(capture.pixels, size) != 0) goto unlock_state;
	if (fb_dos_lock_code(capture_update, length) != 0) goto unlock_pixels;
	capture.width = __fb_gfx->w;
	capture.height = __fb_gfx->h;
	capture.pitch = __fb_gfx->pitch;
	capture.bpp = __fb_gfx->bpp;
	capture.banked = 0;
	capture.updates = 0;
	capture.update = fb_dos.update;
	fb_dos.update = capture_update;
	return 1;
unlock_pixels:
	fb_dos_unlock_data(capture.pixels, size);
unlock_state:
	fb_dos_unlock_data(&capture, sizeof(capture));
fail:
	free(capture.pixels);
	capture.pixels = NULL;
	return 0;
}

void cursor_test_restore(void)
{
	size_t size;
	int length;
	if (!capture.pixels) return;
	fb_dos.update = capture.update;
	size = (size_t)capture.height * capture.pitch;
	length = (int)((uintptr_t)capture_update_end - (uintptr_t)capture_update);
	fb_dos_unlock_code(capture_update, length);
	fb_dos_unlock_data(capture.pixels, size);
	fb_dos_unlock_data(&capture, sizeof(capture));
	free(capture.pixels);
	capture.pixels = NULL;
}

unsigned int cursor_test_updates(void) { return capture.updates; }
void cursor_test_banked(int enabled) { capture.banked = enabled; }

unsigned int cursor_test_pixel(int x, int y, int presented)
{
	unsigned int value = 0;
	unsigned char *pixels = presented ? capture.pixels : __fb_gfx->framebuffer;
	if (!pixels || x < 0 || y < 0 || x >= capture.width || y >= capture.height)
		return 0;
	memcpy(&value, pixels + y * capture.pitch + x * capture.bpp, capture.bpp);
	return value;
}

unsigned int cursor_test_hash(int x, int y)
{
	unsigned int hash = 2166136261u;
	int row, column, byte;
	if (!capture.pixels || x < 0 || y < 0 || x >= capture.width || y >= capture.height)
		return 0;
	/* FNV-1a over a clipped 48x48 capture region; unsigned overflow is defined. */
	for (row = y; row < y + 48 && row < capture.height; row++) {
		for (column = x; column < x + 48 && column < capture.width; column++) {
			for (byte = 0; byte < capture.bpp; byte++) {
				hash ^= capture.pixels[row * capture.pitch + column * capture.bpp + byte];
				hash *= 16777619u;
			}
		}
	}
	return hash;
}
/* end of cursor_capture.c */
