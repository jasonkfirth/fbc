/*
 * Project: FreeBASIC gfxlib2 X11 tests
 * File: x11-window-map.c
 * Purpose: Preserve queued window changes while waiting for one map event.
 * Responsibilities: Exercise production WaitMapped against a private X queue.
 * This file does not map windows or inject events into another X client.
 *
 * Build and run with an available X server:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx/x11-window-map.c -Wl,--gc-sections -lX11 \
 *      -o /tmp/x11-window-map
 *   /tmp/x11-window-map
 */
#include "../../src/gfxlib2/drivers/unix/gfx_x11.c"
#include <stdio.h>
#include <stdlib.h>

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "x11-window-map: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	Display *display = XOpenDisplay(NULL);
	Window target = 1, other = 2;
	XEvent event;
	require(display != NULL, "X server unavailable");
	fb_x11.display = display;
	XSync(display, False);

	/* XPutBackEvent changes only this connection's queue. The XIDs are
	 * labels for synthetic local records, not resources sent to the server. */
	memset(&event, 0, sizeof(event));
	event.type = MapNotify;
	event.xmap.event = event.xmap.window = target;
	XPutBackEvent(display, &event);
	event.xmap.event = event.xmap.window = other;
	XPutBackEvent(display, &event);
	memset(&event, 0, sizeof(event));
	event.type = ConfigureNotify;
	event.xconfigure.event = event.xconfigure.window = target;
	event.xconfigure.width = 1024;
	event.xconfigure.height = 712;
	XPutBackEvent(display, &event);

	WaitMapped(target);
	require(XPending(display) == 2,
		"mapping wait discarded another window event");
	XNextEvent(display, &event);
	require(event.type == ConfigureNotify && event.xconfigure.height == 712,
		"window manager's usable height was not retained");
	XNextEvent(display, &event);
	require(event.type == MapNotify && event.xmap.window == other,
		"unrelated map event was not retained");
	XCloseDisplay(display);
	fb_x11.display = NULL;
	puts("X11_WINDOW_MAP=PASS");
	return EXIT_SUCCESS;
}

/* end of x11-window-map.c */
