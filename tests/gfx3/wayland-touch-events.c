/*
 * Project: FreeBASIC gfxlib3 Linux tests
 * File: wayland-touch-events.c
 * Purpose: Verify the first Wayland touch publishes its position before press.
 * Responsibilities: Check the synchronized event queue and release state.
 * This file intentionally does NOT connect to a compositor or inject input.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx3/wayland-touch-events.c -Wl,--gc-sections -ldl -lm \
 *      -o /tmp/gfx3-wayland-touch-events
 *   /tmp/gfx3-wayland-touch-events
 */
#include "../../src/gfxlib3/drivers/linux/gfx3_platform_wayland.c"
#include "../../src/gfxlib3/core/gfx3_input.c"

FBCALL void fb_MutexLock(FBMUTEX *mutex)
{
	(void)mutex;
}

FBCALL void fb_MutexUnlock(FBMUTEX *mutex)
{
	(void)mutex;
}

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "gfx3-wayland-touch-events: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	FB_GFX3_PLATFORM_WAYLAND platform;
	FB_GFX3_INPUT_STATE input;
	struct wl_proxy *surface = (struct wl_proxy *)(uintptr_t)1;

	memset(&platform, 0, sizeof(platform));
	memset(&input, 0, sizeof(input));
	input.mutex = (FBMUTEX *)(uintptr_t)1;
	input.initialized = TRUE;
	input.mouse_x = -1;
	input.mouse_y = -1;
	platform.input = &input;
	platform.surface = surface;

	platform_wayland_touch_down(&platform, NULL, 0, 0, surface, 7,
		153 * 256, 71 * 256);
	require(input.event_tail == 3 &&
		input.event_queue[0].type == EVENT_MOUSE_ENTER &&
		input.event_queue[1].type == EVENT_MOUSE_MOVE &&
		input.event_queue[1].x == 153 && input.event_queue[1].y == 71 &&
		input.event_queue[2].type == EVENT_MOUSE_BUTTON_PRESS &&
		input.event_queue[2].button == BUTTON_LEFT,
		"contact coordinates did not precede the button press");
	platform_wayland_touch_up(&platform, NULL, 0, 0, 7);
	require(input.event_tail == 4 &&
		input.event_queue[3].type == EVENT_MOUSE_BUTTON_RELEASE &&
		input.event_queue[3].button == BUTTON_LEFT && input.mouse_buttons == 0,
		"contact release did not clear the button");

	puts("GFX3_WAYLAND_TOUCH_EVENTS_PASS");
	return EXIT_SUCCESS;
}

/* end of wayland-touch-events.c */
