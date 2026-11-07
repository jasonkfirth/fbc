/*
 * Project: FreeBASIC gfxlib2 Linux Wayland support
 * File: fb_wayland_display.h
 * Purpose: Share display information collected from the Wayland registry.
 * Responsibilities: Define the output geometry and capability result contract.
 * This file does not contain window creation, rendering, or input policy.
 */
#ifndef FB_WAYLAND_DISPLAY_H
#define FB_WAYLAND_DISPLAY_H

#include <sys/types.h>

typedef struct FB_WAYLAND_DISPLAY_INFO {
	int width;
	int height;
	int depth;
	int refresh;
	int dpi;
	int rotation;
	int scale;
	int touch_capable;
	int text_input_available;
} FB_WAYLAND_DISPLAY_INFO;

struct wl_display;

int fb_hWaylandDisplayInfo(FB_WAYLAND_DISPLAY_INFO *info);

/*
 * Pair these calls around a window connection and its initial roundtrips.
 * Begin transfers any existing metrics connection to the caller, which owns
 * and must disconnect it. A metrics query on another thread waits until the
 * window is registered. A NULL result means the caller must connect normally.
 */
int fb_hWaylandDisplayBeginWindow(struct wl_display **display);
void fb_hWaylandDisplayEndWindow(void);

#endif
/* end of fb_wayland_display.h */
