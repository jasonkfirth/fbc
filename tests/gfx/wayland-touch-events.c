/*
 * Project: FreeBASIC Linux graphics tests
 * File: wayland-touch-events.c
 * Purpose: Verify the event sequence for a Wayland touch contact.
 * Responsibilities: Check move coordinates, button edges, motion and release.
 * This file intentionally does NOT connect to a compositor or inject input.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx/wayland-touch-events.c -Wl,--gc-sections -ldl -lm \
 *      -o /tmp/wayland-touch-events
 *   /tmp/wayland-touch-events
 */
#include "../../src/gfxlib2/drivers/linux/gfx_driver_wayland.c"

static EVENT events[8];
static size_t event_count;

void fb_hPostEvent(EVENT *event)
{
	if (event_count < sizeof(events) / sizeof(events[0]))
		events[event_count++] = *event;
}

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "wayland-touch-events: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	struct wl_surface *surface = (struct wl_surface *)(uintptr_t)1;
	memset(&fb_wayland_window, 0, sizeof(fb_wayland_window));
	fb_wayland_window.surface = surface;

	/* Wayland coordinates are 24.8 fixed point, in surface pixels. */
	fb_wayland_touch_down(NULL, NULL, 0, 0, surface, 7, 153 * 256, 71 * 256);
	require(event_count == 2 && events[0].type == EVENT_MOUSE_MOVE &&
		events[0].x == 153 && events[0].y == 71 &&
		events[1].type == EVENT_MOUSE_BUTTON_PRESS &&
		events[1].button == BUTTON_LEFT,
		"contact coordinates were lost before the button press");
	fb_wayland_touch_motion(NULL, NULL, 0, 7, 160 * 256, 75 * 256);
	require(event_count == 3 && events[2].type == EVENT_MOUSE_MOVE &&
		events[2].x == 160 && events[2].y == 75,
		"contact motion lost its coordinates");
	fb_wayland_touch_up(NULL, NULL, 0, 0, 7);
	require(event_count == 4 && events[3].type == EVENT_MOUSE_BUTTON_RELEASE &&
		events[3].button == BUTTON_LEFT && fb_wayland_window.mouse_buttons == 0,
		"contact release did not clear the button");

	puts("WAYLAND_TOUCH_EVENTS_PASS");
	return EXIT_SUCCESS;
}

/* end of wayland-touch-events.c */
