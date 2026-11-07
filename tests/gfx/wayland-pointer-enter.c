/*
 * Project: FreeBASIC gfxlib2 Linux tests
 * File: wayland-pointer-enter.c
 * Purpose: Verify pointer entry publishes a usable position before a click.
 * Responsibilities: Check event ordering, foreign surfaces and scanlines.
 * This file intentionally does NOT connect to a compositor or inject input.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx/wayland-pointer-enter.c -Wl,--gc-sections -ldl -lm \
 *      -o /tmp/wayland-pointer-enter
 *   /tmp/wayland-pointer-enter
 */
#include "../../src/gfxlib2/drivers/linux/gfx_driver_wayland.c"

FBGFX *__fb_gfx;

static EVENT events[8];
static size_t event_count;

void fb_hPostEvent(EVENT *event)
{
	if (event_count >= sizeof(events) / sizeof(events[0])) {
		fprintf(stderr, "wayland-pointer-enter: too many events\n");
		exit(EXIT_FAILURE);
	}
	events[event_count++] = *event;
}

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "wayland-pointer-enter: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	FB_WAYLAND_WINDOW window;
	FBGFX gfx;
	struct wl_surface *surface = (struct wl_surface *)(uintptr_t)1;
	struct wl_surface *other_surface = (struct wl_surface *)(uintptr_t)2;

	memset(&window, 0, sizeof(window));
	memset(&gfx, 0, sizeof(gfx));
	window.surface = surface;
	window.width = 400;
	window.height = 300;

	/* A different surface must not affect this window's input state. */
	fb_wayland_pointer_enter(&window, NULL, 0, other_surface, 27 * 256, 88 * 256);
	require(event_count == 0 && !window.pointer_focused,
		"foreign surface changed pointer state");

	/* Wayland may send a button directly after enter, without motion. */
	fb_wayland_pointer_enter(&window, NULL, 0, surface, 27 * 256, 88 * 256);
	require(event_count == 2, "entry did not publish enter and move");
	require(events[0].type == EVENT_MOUSE_ENTER, "missing enter event");
	require(events[1].type == EVENT_MOUSE_MOVE && events[1].x == 27 &&
		events[1].y == 88 && events[1].dx == 0 && events[1].dy == 0,
		"entry position or initial delta is wrong");
	fb_wayland_pointer_button(&window, NULL, 0, 0, 0x110u, 1u);
	require(event_count == 3 && events[2].type == EVENT_MOUSE_BUTTON_PRESS &&
		events[2].button == BUTTON_LEFT, "click was not published after position");

	/* Both entry and subsequent motion use BASIC's logical scanline height. */
	event_count = 0;
	gfx.scanline_size = 2;
	__fb_gfx = &gfx;
	fb_wayland_pointer_enter(&window, NULL, 0, surface, 30 * 256, 80 * 256);
	require(event_count == 2 && events[1].type == EVENT_MOUSE_MOVE &&
		events[1].x == 30 && events[1].y == 40 && events[1].dy == 0,
		"entry position ignored the scanline scale");
	fb_wayland_pointer_motion(&window, NULL, 0, 32 * 256, 100 * 256);
	require(event_count == 3 && events[2].type == EVENT_MOUSE_MOVE &&
		events[2].x == 32 && events[2].y == 50 &&
		events[2].dx == 2 && events[2].dy == 10,
		"motion position or scaled delta is wrong");

	/* An uninitialized scanline size must not cause division by zero. */
	event_count = 0;
	gfx.scanline_size = 0;
	fb_wayland_pointer_enter(&window, NULL, 0, surface, -10 * 256, 700 * 256);
	require(event_count == 2 && events[1].type == EVENT_MOUSE_MOVE &&
		events[1].x == 0 && events[1].y == 299,
		"entry did not clamp coordinates with an unset scanline size");

	puts("wayland-pointer-enter: PASS");
	return EXIT_SUCCESS;
}

/* end of wayland-pointer-enter.c */
