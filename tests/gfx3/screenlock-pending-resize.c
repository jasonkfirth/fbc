/*
 * Project: FreeBASIC gfxlib3 tests
 * File: screenlock-pending-resize.c
 * Purpose: Check that resize requests survive a live framebuffer lock.
 * Responsibilities: Exercise the input queue and graphics API resize boundary.
 * This file intentionally does NOT create a window or access a GPU.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx3/screenlock-pending-resize.c \
 *      src/gfxlib3/api/gfx3_api.c src/gfxlib3/core/gfx3_input.c \
 *      -Wl,--gc-sections -ldl -lm -o /tmp/gfx3-screenlock-pending-resize
 *   /tmp/gfx3-screenlock-pending-resize
 */

#include "../../src/gfxlib3/gfx3_api_internal.h"
#include "../../src/gfxlib3/gfx3_compat.h"
#include "../../src/gfxlib3/gfx3_input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int resize_calls;
static int queue_new_configure;

FBCALL void fb_MutexLock(FBMUTEX *mutex)
{
	(void)mutex;
}

FBCALL void fb_MutexUnlock(FBMUTEX *mutex)
{
	(void)mutex;
}

int fb_gfx3_mode_resize(FB_GFX3_MODE *mode, FB_GFX3_DRAW_STATE *state,
	uint32_t width, uint32_t height)
{
	(void)state;
	resize_calls++;
	/* The real mode must keep the existing framebuffer while locked. */
	if (mode->access_lock_count == 0u) {
		mode->width = width;
		mode->height = height;
		if (queue_new_configure) {
			queue_new_configure = FALSE;
			fb_gfx3_input_platform_resize(&mode->input, 340u, 600u);
		}
		fb_gfx3_input_complete_resize(&mode->input, width, height);
	}
	return FB_GFX3_OK;
}

int fb_gfx3_draw_state_sync_resize(FB_GFX3_DRAW_STATE *state)
{
	(void)state;
	return FB_GFX3_OK;
}

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "gfx3-screenlock-pending-resize: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	FB_GFX3_MODE mode;
	FB_GFX3_DRAW_STATE state;

	memset(&mode, 0, sizeof(mode));
	memset(&state, 0, sizeof(state));
	mode.resizable = TRUE;
	mode.width = 360u;
	mode.height = 640u;
	mode.console_font_width = 8u;
	mode.console_font_height = 16u;
	mode.input.mutex = (FBMUTEX *)(uintptr_t)1;
	mode.input.initialized = TRUE;
	mode.input.resizable = TRUE;
	state.mode = &mode;

	/* Rotation can arrive during a nested SCREENLOCK. */
	mode.access_lock_count = 2u;
	fb_gfx3_input_platform_resize(&mode.input, 640u, 360u);
	require(fb_gfx3_api_apply_pending_resize_locked(&state) == FB_GFX3_OK,
		"resize check failed while locked");
	require(resize_calls == 0u && mode.width == 360u && mode.height == 640u &&
		mode.input.pending_width == 640u && mode.input.pending_height == 360u,
		"the pending rotation was attempted while locked");

	/* A later configure replaces the first while the outer lock is live. */
	mode.access_lock_count = 1u;
	fb_gfx3_input_platform_resize(&mode.input, 600u, 340u);
	require(fb_gfx3_api_apply_pending_resize_locked(&state) == FB_GFX3_OK &&
		resize_calls == 0u && mode.input.pending_width == 600u &&
		mode.input.pending_height == 340u,
		"the most recent configure was not retained");

	mode.access_lock_count = 0u;
	require(fb_gfx3_api_apply_pending_resize_locked(&state) == FB_GFX3_OK &&
		resize_calls == 1u && mode.width == 600u && mode.height == 340u &&
		mode.input.pending_width == 0u && mode.input.pending_height == 0u,
		"the final configure was not applied after unlocking");
	require(fb_gfx3_api_apply_pending_resize_locked(&state) == FB_GFX3_OK &&
		resize_calls == 1u, "the resize was applied more than once");
	require(mode.input.event_tail == 1u &&
		mode.input.event_queue[0].type == EVENT_WINDOW_RESIZE &&
		mode.input.event_queue[0].width == 600 &&
		mode.input.event_queue[0].height == 340,
		"the completed resize event was not delivered");

	/* The renderer can publish another configure during mode replacement. */
	fb_gfx3_input_platform_resize(&mode.input, 400u, 500u);
	queue_new_configure = TRUE;
	require(fb_gfx3_api_apply_pending_resize_locked(&state) == FB_GFX3_OK &&
		mode.width == 400u && mode.height == 500u &&
		mode.input.pending_width == 340u && mode.input.pending_height == 600u,
		"completion discarded a newer configure");
	require(fb_gfx3_api_apply_pending_resize_locked(&state) == FB_GFX3_OK &&
		mode.width == 340u && mode.height == 600u && resize_calls == 3u &&
		mode.input.pending_width == 0u && mode.input.pending_height == 0u,
		"the newer configure was not applied");

	puts("GFX3_SCREENLOCK_PENDING_RESIZE_PASS");
	return EXIT_SUCCESS;
}

/* end of screenlock-pending-resize.c */
