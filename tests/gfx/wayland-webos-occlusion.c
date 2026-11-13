/*
 * Project: FreeBASIC Linux graphics tests
 * File: wayland-webos-occlusion.c
 * Purpose: Check observed IME bounds against the actual drawable.
 * Responsibilities: Exercise hidden/unknown panels, clipped coordinates and
 *     extreme compositor rectangles through the production query.
 * This file does not require a compositor or emulate widget layout.
 */
#include <assert.h>
#include "../../src/gfxlib2/drivers/linux/gfx_driver_wayland.c"

FBGFX *fb_gfx;

int main(void)
{
	FBGFX gfx = { 0 };
	FB_WAYLAND_WEBOS *webos = &fb_wayland_window.webos;
	int top = 0, height = 0;
	fb_gfx = &gfx;
	gfx.h = 1050;
	fb_wayland_window.active = 1;
	webos->visible = -1;
	webos->panel_top = 696;
	webos->panel_height = 354;
	assert(fb_hWaylandGetKeyboardOcclusion(&top, &height) == 1);
	assert(top == 696 && height == 354);
	webos->panel_top = -20; webos->panel_height = 200;
	assert(fb_hWaylandGetKeyboardOcclusion(&top, &height) == 1);
	assert(top == 0 && height == 180);
	webos->panel_top = 900; webos->panel_height = INT_MAX;
	assert(fb_hWaylandGetKeyboardOcclusion(&top, &height) == 1);
	assert(top == 900 && height == 150);
	webos->panel_top = INT_MAX;
	assert(fb_hWaylandGetKeyboardOcclusion(&top, &height) == 0);
	webos->panel_top = INT_MIN; webos->panel_height = 1;
	assert(fb_hWaylandGetKeyboardOcclusion(&top, &height) == 0);
	webos->panel_top = 696; webos->panel_height = 354; webos->visible = -2;
	assert(fb_hWaylandGetKeyboardOcclusion(&top, &height) == 0);
	webos->visible = 0;
	assert(fb_hWaylandGetKeyboardOcclusion(&top, &height) == 0);
	webos->visible = -1; fb_wayland_window.active = 0;
	assert(fb_hWaylandGetKeyboardOcclusion(&top, &height) == 0);
	assert(fb_hWaylandGetKeyboardOcclusion(NULL, &height) == 0);
	puts("wayland-webos-occlusion: PASS");
	return 0;
}

/* end of wayland-webos-occlusion.c */
