/*
 * Project: FreeBASIC gfxlib2 Linux tests
 * File: wayland-keyboard-occlusion.c
 * Purpose: Check native keyboard overlap through the public driver query.
 * Responsibilities: Cover visibility, drawable clipping and integer limits.
 * This file does not connect to a compositor or synthesize keyboard input.
 *
 * Build with the same command as wayland-window-title.c, changing the filename.
 */
#include "../../src/gfxlib2/drivers/linux/gfx_driver_wayland.c"
#include <limits.h>

static FBGFX context;
FBGFX *__fb_gfx = &context;
static unsigned checks;
static int maliit_available;
static unsigned maliit_queries;

int fb_hLinuxMaliitGetKeyboardOcclusion(int width, int height,
	int *top, int *overlap)
{
	maliit_queries++;
	if (!maliit_available || width != 800 || height != 1222)
		return 0;
	*top = 900;
	*overlap = 322;
	return 1;
}

static void require(int condition, const char *message)
{
	checks++;
	if (!condition) {
		fprintf(stderr, "wayland-keyboard-occlusion: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

static void panel(uint32_t state, int32_t x, int32_t y,
	int32_t width, int32_t height, int expected_top, int expected_height)
{
	int top = -7, overlap = -9;
	fb_wayland_text_v2_panel_state(&fb_wayland_window, NULL,
		state, x, y, width, height);
	int result = fb_hWaylandGetKeyboardOcclusion(&top, &overlap);
	if (expected_height > 0)
		require(result == 1 && top == expected_top && overlap == expected_height,
			"incorrect visible intersection");
	else
		require(result == 0 && top == -7 && overlap == -9,
			"absent overlap changed the caller's outputs");
}

int main(void)
{
	memset(&fb_wayland_window, 0, sizeof(fb_wayland_window));
	context.w = 800;
	context.h = 1222;
	fb_wayland_window.active = 1;
	fb_wayland_window.text_input_v2 = (struct zwp_text_input_v2 *)(uintptr_t)1;
	panel(1, 0, 982, 800, 240, 982, 240);
	panel(0, 0, 982, 800, 240, 0, 0);
	panel(2, 0, 982, 800, 240, 0, 0);
	panel(1, 0, 1222, 800, 240, 0, 0);
	panel(1, 0, 1200, 800, 240, 1200, 22);
	panel(1, -10, -20, 800, 50, 0, 30);
	panel(1, -800, 982, 800, 240, 0, 0);
	panel(1, 800, 982, 800, 240, 0, 0);
	panel(1, INT_MAX, 982, INT_MAX, 240, 0, 0);
	panel(1, -1, 982, INT_MAX, INT_MAX, 982, 240);
	panel(1, 0, INT_MIN, 800, INT_MAX, 0, 0);
	panel(1, 0, 982, 0, 240, 0, 0);
	panel(1, 0, 982, 800, -1, 0, 0);
	/* A compositor reservation ends exactly where the resized drawable ends. */
	context.h = 502;
	panel(1, 0, 502, 800, 240, 0, 0);
	context.h = 1222;
	fb_wayland_window.active = 0;
	panel(1, 0, 982, 800, 240, 0, 0);
	fb_wayland_window.active = 1;
	fb_wayland_window.text_input_v2 = NULL;
	panel(1, 0, 982, 800, 240, 0, 0);
	fb_wayland_window.webos.visible = -1;
	fb_wayland_window.webos.panel_top = 1000;
	fb_wayland_window.webos.panel_height = 300;
	int top = 0, height = 0;
	require(fb_hWaylandGetKeyboardOcclusion(&top, &height) == 1 &&
		top == 1000 && height == 222, "legacy webOS overlap changed");
	require(fb_hWaylandGetKeyboardOcclusion(NULL, &height) == 0 &&
		fb_hWaylandGetKeyboardOcclusion(&top, NULL) == 0,
		"null outputs were accepted");
	maliit_available = 1;
	unsigned before = maliit_queries;
	require(fb_hWaylandGetKeyboardOcclusion(&top, &height) == 1 &&
		top == 1000 && height == 222 && maliit_queries == before,
		"Maliit replaced the native webOS area");
	fb_wayland_window.webos.visible = 0;
	require(fb_hWaylandGetKeyboardOcclusion(&top, &height) == 1 &&
		top == 900 && height == 322, "Maliit fallback area was discarded");
	fb_wayland_window.text_input = (struct zwp_text_input_v3 *)(uintptr_t)1;
	before = maliit_queries;
	require(fb_hWaylandGetKeyboardOcclusion(&top, &height) == 0 &&
		maliit_queries == before, "Maliit replaced a native text-input context");
	fb_wayland_window.text_input = NULL;
	fb_wayland_window.active = 0;
	require(fb_hWaylandGetKeyboardOcclusion(&top, &height) == 0 &&
		maliit_queries == before, "inactive mode queried Maliit");
	printf("wayland-keyboard-occlusion: PASS checks=%u\n", checks);
	return EXIT_SUCCESS;
}

/* end of wayland-keyboard-occlusion.c */
