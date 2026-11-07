/*
 * Project: FreeBASIC graphics tests
 * File: screenunlock-resize.c
 * Purpose: Check when a pending resize can be applied after framebuffer access.
 * Responsibilities: Exercise nested frames and internal empty-range unlocks.
 * This file intentionally does NOT create a native window or resize a display.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx/screenunlock-resize.c -Wl,--gc-sections \
 *      -o /tmp/screenunlock-resize
 *   /tmp/screenunlock-resize
 */
#include "../../src/gfxlib2/core/gfx_access.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FBGFX *__fb_gfx;
void *(*fb_hMemSet)(void *, int, size_t) = memset;

static FB_GFXCTX test_context;
static int driver_locks;
static int driver_unlocks;
static int resize_calls;

FBCALL void fb_GraphicsLock(void) { }
FBCALL void fb_GraphicsUnlock(void) { }
FB_GFXCTX *fb_hGetContext(void) { return &test_context; }
int fb_hApplyPendingResize(void) { resize_calls++; return 0; }

static void driver_lock(void) { driver_locks++; }
static void driver_unlock(void) { driver_unlocks++; }

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "screenunlock-resize: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	static const GFXDRIVER driver = {
		.lock = driver_lock,
		.unlock = driver_unlock
	};
	FBGFX gfx = { 0 };
	char dirty[4] = { 0 };

	__fb_gfx = &gfx;
	gfx.driver = &driver;
	gfx.dirty = dirty;
	gfx.h = 4;
	gfx.visible_page = 0;
	test_context.work_page = 0;

	fb_GfxLock();
	fb_GfxLock();
	fb_GfxUnlock(0, -1);
	require(resize_calls == 0 && driver_unlocks == 0,
		"nested frame released the driver too early");
	fb_GfxUnlock(0, -1);
	require(resize_calls == 1 && driver_locks == 1 && driver_unlocks == 1,
		"outer frame did not apply the pending resize");
	require(dirty[0] && dirty[1] && dirty[2] && dirty[3],
		"frame rows were not marked dirty");

	fb_GfxLock();
	fb_GfxUnlock(1, 0);
	require(resize_calls == 1 && driver_locks == 2 && driver_unlocks == 2,
		"internal empty-range unlock applied a resize inside drawing");

	puts("SCREENUNLOCK_RESIZE_PASS");
	return EXIT_SUCCESS;
}

/* end of screenunlock-resize.c */
