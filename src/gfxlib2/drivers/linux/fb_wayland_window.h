/*
 * Project: FreeBASIC gfxlib2 Linux Wayland support
 * File: fb_wayland_window.h
 * Purpose: Publish window and text-input controls for the Wayland driver.
 * Responsibilities: Expose the active driver and per-field keyboard requests.
 * This file intentionally does not define compositor policy or UI widgets.
 */
#ifndef FB_WAYLAND_WINDOW_H
#define FB_WAYLAND_WINDOW_H

int fb_hWaylandSetKeyboard(int visible);
int fb_hWaylandSetTextInputRegion(int x, int y, int width, int height);
int fb_hWaylandClearTextInputRegions(void);
int fb_hWaylandGetKeyboardState(int *visible);
int fb_hWaylandGetKeyboardOcclusion(int *top, int *height);

#endif

/* end of fb_wayland_window.h */
