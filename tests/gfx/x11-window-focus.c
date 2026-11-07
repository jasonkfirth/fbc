/*
 * Project: FreeBASIC gfxlib2 X11 tests
 * File: x11-window-focus.c
 * Purpose: Keep public focus events independent of legacy pointer activity.
 * Responsibilities: Exercise the production focus transition helper.
 * This file does not connect to an X server or synthesize input devices.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx/x11-window-focus.c -Wl,--gc-sections \
 *      -o /tmp/x11-window-focus
 *   /tmp/x11-window-focus
 */
#include "../../src/gfxlib2/drivers/unix/gfx_x11.c"
#include <stdio.h>
#include <stdlib.h>

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "x11-window-focus: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	EVENT event;
	memset(&event, 0, sizeof(event));
	has_focus = FALSE;
	focus_notified = FALSE;
	hSetWindowFocus(TRUE, &event);
	require(event.type == EVENT_WINDOW_GOT_FOCUS,
		"initial focus notification missing");
	memset(&event, 0, sizeof(event));
	hSetWindowFocus(TRUE, &event);
	require(event.type == 0, "unchanged focus emitted another notification");
	hSetWindowFocus(FALSE, &event);
	require(!has_focus && event.type == EVENT_WINDOW_LOST_FOCUS,
		"focus loss was not reported");

	/* EnterNotify, ButtonPress and TouchBegin retain this legacy assignment.
	 * None is a replacement for the later public FocusIn notification. */
	has_focus = TRUE;
	memset(&event, 0, sizeof(event));
	hSetWindowFocus(TRUE, &event);
	require(event.type == EVENT_WINDOW_GOT_FOCUS,
		"pointer activity hid the regained keyboard focus");
	memset(&event, 0, sizeof(event));
	hSetWindowFocus(TRUE, &event);
	require(event.type == 0, "repeated FocusIn was not coalesced");
	hSetWindowFocus(FALSE, &event);
	require(event.type == EVENT_WINDOW_LOST_FOCUS,
		"second focus loss was not reported");
	puts("X11_WINDOW_FOCUS=PASS");
	return EXIT_SUCCESS;
}

/* end of x11-window-focus.c */
