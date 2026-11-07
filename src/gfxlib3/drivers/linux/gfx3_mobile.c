/*
    Project: FreeBASIC gfxlib3 Linux graphics
    ------------------------------------------

    File: linux/gfx3_mobile.c

    Purpose:

        Report Linux display geometry, density and rotation, and route
        optional virtual keyboard requests to the active user session.

    Responsibilities:

        - read Wayland output geometry before a graphics mode exists
        - read Wayland or X11 density and rotation
        - request the session's supported virtual keyboard bridge

    This file intentionally does NOT contain:

        - renderer state or GPU window creation
        - handheld classification or OMA GUI policy
        - Wayland text-input surface management
*/

#include "gfx3_mobile.h"
#include "../../../gfxlib2/drivers/mobile_control.h"
#include "../../../gfxlib2/drivers/linux/fb_linux_device.h"
#include "../../../gfxlib2/drivers/linux/fb_linux_display.h"
#include "../../../gfxlib2/drivers/linux/fb_linux_maliit.h"
#include "../../../gfxlib2/drivers/linux/fb_linux_osk.h"
#include "../../../gfxlib2/drivers/linux/fb_wayland_display.h"

#include <math.h>
#include <stdlib.h>

#ifndef DISABLE_X11
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/extensions/Xrandr.h>
#endif

#define FB_GFX3_LINUX_MIN_DPI 72
#define FB_GFX3_LINUX_MAX_DPI 768

static void gfx3_linux_maliit_post_key(void *user_data, uint32_t key,
	FB_LINUX_MALIIT_INPUT_KIND kind)
{
	FB_GFX3_INPUT_STATE *input = (FB_GFX3_INPUT_STATE *)user_data;
	int scan_code;
	int ascii;
	if (kind == FB_LINUX_MALIIT_TEXT) {
		fb_gfx3_input_platform_text(input, key);
		return;
	}
	scan_code = fb_hLinuxMaliitKeyScancode(key);
	ascii = key <= 0xffu ? (int)key : 0;
	fb_gfx3_input_platform_character(input, (int)key, 1);
	fb_gfx3_input_platform_key(input, EVENT_KEY_PRESS, scan_code, ascii);
	fb_gfx3_input_platform_key(input, EVENT_KEY_RELEASE, scan_code, ascii);
}

void fb_gfx3_linux_display_metrics(ssize_t *dpi, ssize_t *rotation)
{
	if (dpi)
		*dpi = 96;
	if (rotation)
		*rotation = 0;
	if (dpi || rotation) {
		FB_WAYLAND_DISPLAY_INFO info;
		/* Use compositor output metrics when Wayland is the active session. */
		if (fb_hWaylandDisplayInfo(&info) == 0) {
			if (dpi)
				*dpi = info.dpi;
			if (rotation)
				*rotation = info.rotation;
			return;
		}
	}

#ifndef DISABLE_X11
	if (dpi || rotation) {
		const char *display_name = getenv("DISPLAY");
		Display *display;
		if (!display_name || !*display_name)
			return;
		display = XOpenDisplay(display_name);
		if (display) {
			int screen = DefaultScreen(display);
			if (dpi) {
				XrmDatabase database = NULL;
				XrmValue value;
				char *type = NULL;
				char *resources = XResourceManagerString(display);
				double density = 0.0;
				if (resources && *resources) {
					XrmInitialize();
					database = XrmGetStringDatabase(resources);
				}
				if (database) {
					if (XrmGetResource(database, "Xft.dpi", "Xft.Dpi",
						&type, &value) && value.addr && value.size > 0)
						density = strtod(value.addr, NULL);
					XrmDestroyDatabase(database);
				}
				if (!isfinite(density) ||
				    density < FB_GFX3_LINUX_MIN_DPI ||
				    density > FB_GFX3_LINUX_MAX_DPI) {
					int millimeters = XDisplayWidthMM(display, screen);
					if (millimeters > 0)
						density = ((double)XDisplayWidth(display, screen) *
							25.4) / (double)millimeters;
					else
						density = 96.0;
				}
				if (!isfinite(density) ||
				    density < FB_GFX3_LINUX_MIN_DPI ||
				    density > FB_GFX3_LINUX_MAX_DPI)
					density = 96.0;
				*dpi = (ssize_t)(density + 0.5);
			}
			if (rotation) {
				XRRScreenConfiguration *configuration;
				Rotation current_rotation = RR_Rotate_0;
				configuration = XRRGetScreenInfo(display,
					RootWindow(display, screen));
				if (configuration) {
					(void)XRRConfigCurrentConfiguration(configuration,
						&current_rotation);
					if (current_rotation & RR_Rotate_90)
						*rotation = 90;
					else if (current_rotation & RR_Rotate_180)
						*rotation = 180;
					else if (current_rotation & RR_Rotate_270)
						*rotation = 270;
					XRRFreeScreenConfigInfo(configuration);
				}
			}
			XCloseDisplay(display);
		}
	}
#endif
}

int fb_gfx3_linux_initial_window_metrics(ssize_t *width, ssize_t *height,
	ssize_t *dpi, ssize_t *rotation)
{
	FB_WAYLAND_DISPLAY_INFO info;
	const char *mir_socket = getenv("MIR_SOCKET");
	const char *display = getenv("DISPLAY");

	if (fb_hWaylandDisplayInfo(&info) == 0) {
		if (width)
			*width = info.width;
		if (height)
			*height = info.height;
		if (dpi)
			*dpi = info.dpi;
		if (rotation)
			*rotation = info.rotation;
		return 1;
	}

	if (mir_socket && *mir_socket && display && *display) {
		if (fb_hLinuxFramebufferMetrics(width, height, dpi, rotation))
			return 1;
		/* Avoid an unbounded X11 connection when framebuffer sysfs is absent. */
		if (width)
			*width = 0;
		if (height)
			*height = 0;
		if (dpi)
			*dpi = 96;
		if (rotation)
			*rotation = 0;
		return 1;
	}
	return 0;
}

void fb_gfx3_linux_mobile_control(int what, ssize_t *p1, ssize_t *p2,
	ssize_t *p3, ssize_t *p4, FB_GFX3_INPUT_STATE *input)
{
	if (what == GET_DEVICE_CLASS) {
		*p1 = fb_hLinuxDeviceClass();
		*p2 = *p3 = *p4 = 0;
	} else if (what == SET_SOFT_KEYBOARD) {
		if (*p1 != (ssize_t)0x80000000) {
			if (input != NULL) {
				uintptr_t native_window = 0;
				/* Publish the native window before a focused field can request IME. */
				if (*p1 != 0 && fb_gfx3_input_window_snapshot(input,
				    &native_window, NULL, NULL, NULL, NULL, NULL) == FB_GFX3_OK)
					fb_hLinuxMaliitSetWindowId((uint64_t)native_window);
				fb_hLinuxMaliitSetKeyCallback(
					gfx3_linux_maliit_post_key, input);
				(void)fb_gfx3_input_request_soft_keyboard(input, *p1 != 0);
			}
			fb_hLinuxOskRequest(*p1 != 0);
		}
	} else if (what == GET_TOUCH_CAPABILITY) {
		FB_WAYLAND_DISPLAY_INFO info;
		if (fb_hWaylandDisplayInfo(&info) == 0)
			*p1 = info.touch_capable ? -1 : 0;
		else
			*p1 = -2;
		*p2 = *p3 = *p4 = 0;
	} else if (what == GET_SOFT_KEYBOARD) {
		int visible = -2;
		int available = fb_hLinuxOskQuery(&visible);
		if ((available == 0) && (input != NULL)) {
			(void)fb_gfx3_input_keyboard_state(input, &available, &visible);
			*p2 = available ? -1 : 0;
		} else {
			*p2 = available;
		}
		*p1 = visible;
		*p3 = *p4 = 0;
	} else if (what == SET_SOFT_KEYBOARD_REGION) {
		if (input != NULL) {
			uintptr_t native_window = 0;
			(void)fb_gfx3_input_window_snapshot(input, &native_window,
				NULL, NULL, NULL, NULL, NULL);
			fb_hLinuxMaliitSetTextRegion((int)*p1, (int)*p2,
				(int)*p3, (int)*p4, (uint64_t)native_window);
			(void)fb_gfx3_input_request_text_region(input, (int)*p1,
				(int)*p2, (int)*p3, (int)*p4);
		}
	} else if (what == CLEAR_SOFT_KEYBOARD_REGIONS) {
		if (input != NULL)
			(void)fb_gfx3_input_request_text_region(input, 0, 0, 1, 1);
	}
}

/* end of gfx3_mobile.c */
