/*
 * FreeBASIC Linux graphics: gfx_mobile.c
 * Purpose: Report live display metrics and request a session virtual keyboard.
 * Responsibilities: X11 geometry, DPI and rotation; optional session-bus OSK.
 * This file does not classify applications or implement widget behavior.
 * The shared OSK helper selects a supported session interface.
 */
#include "../../fb_gfx.h"
#include "../mobile_control.h"
#include "fb_linux_device.h"
#include "fb_linux_display.h"
#include "fb_linux_osk.h"
#include "fb_wayland_display.h"
#include "fb_wayland_window.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DISABLE_X11
#include "../unix/fb_gfx_x11.h"
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#endif

#define FB_LINUX_METRIC_MIN_DPI 72
#define FB_LINUX_METRIC_MAX_DPI 768
/* Bound the root resource property before parsing a complete Xrm database. */
#define FB_LINUX_XRESOURCE_MAX_BYTES (64u * 1024u)

static void linux_maliit_post_key(void *user_data, uint32_t key,
	FB_LINUX_MALIIT_INPUT_KIND kind)
{
	EVENT event;
	const GFXDRIVER *driver;
	int scan_code;
	(void)user_data;
	if (kind == FB_LINUX_MALIIT_TEXT && key == '\n')
		key = '\r';
	scan_code = (kind == FB_LINUX_MALIIT_KEY || key <= 0xffu) ?
		fb_hLinuxMaliitKeyScancode(key) : 0;
	/*
	 * The bridge keeps the mode alive while this callback runs. Take only
	 * the driver lock here: shutdown may already hold the optional runtime
	 * graphics lock while it waits for the callback to finish.
	 */
	if (__fb_gfx && __fb_gfx->driver && __fb_gfx->driver->lock &&
	    __fb_gfx->driver->unlock) {
		driver = __fb_gfx->driver;
		driver->lock();
		/* Unicode text is an event, not a legacy extended INKEY code. */
		if (kind == FB_LINUX_MALIIT_KEY || key <= 0xffu)
			fb_hPostKey((int)key);
		memset(&event, 0, sizeof(event));
		event.type = EVENT_KEY_PRESS;
		event.scancode = scan_code;
		event.ascii = (kind == FB_LINUX_MALIIT_TEXT || key <= 0xffu) ? (int)key : 0;
		fb_hPostEvent(&event);
		event.type = EVENT_KEY_RELEASE;
		fb_hPostEvent(&event);
		driver->unlock();
	}
}

#ifndef DISABLE_X11
static int linux_display_dpi(Display *display, int screen)
{
	XrmDatabase database;
	XrmValue value;
	char *type = NULL;
	unsigned char *resources = NULL;
	Atom resource_type;
	int resource_format;
	unsigned long resource_count, resource_remaining;
	double dpi = 0.0;
	int millimeters;

	/* XResourceManagerString is a connection-start snapshot. Read the live
	 * property so desktop DPI changes reach an already-running application.
	 * XGetWindowProperty adds a NUL byte; reject incomplete/oversized data. */
	if (XGetWindowProperty(display, RootWindow(display, screen),
		XA_RESOURCE_MANAGER, 0, FB_LINUX_XRESOURCE_MAX_BYTES / 4u, False,
		XA_STRING, &resource_type, &resource_format, &resource_count,
		&resource_remaining, &resources) == Success &&
		resource_type == XA_STRING && resource_format == 8 &&
		resource_remaining == 0 && resource_count > 0 && resources) {
		XrmInitialize();
		database = XrmGetStringDatabase((char *)resources);
		if (database) {
			if (XrmGetResource(database, "Xft.dpi", "Xft.Dpi",
				&type, &value) && value.addr && value.size > 0) {
				dpi = strtod(value.addr, NULL);
			}
			XrmDestroyDatabase(database);
		}
	}
	if (resources)
		XFree(resources);

	if (!isfinite(dpi) || (dpi < FB_LINUX_METRIC_MIN_DPI) ||
	    (dpi > FB_LINUX_METRIC_MAX_DPI)) {
		millimeters = XDisplayWidthMM(display, screen);
		if (millimeters > 0) {
			dpi = ((double)XDisplayWidth(display, screen) * 25.4) /
				(double)millimeters;
		} else {
			dpi = 96.0;
		}
	}
	if (!isfinite(dpi) || (dpi < FB_LINUX_METRIC_MIN_DPI) ||
	    (dpi > FB_LINUX_METRIC_MAX_DPI))
		dpi = 96.0;
	return (int)(dpi + 0.5);
}

static int linux_rotation_degrees(Rotation rotation)
{
	if (rotation & RR_Rotate_90)
		return 90;
	if (rotation & RR_Rotate_180)
		return 180;
	if (rotation & RR_Rotate_270)
		return 270;
	return 0;
}

static int linux_output_rotation(Display *display,
	XRRScreenResources *resources, RROutput output, int *active_output)
{
	XRROutputInfo *output_info;
	XRRCrtcInfo *crtc_info;
	int result = 0;

	if (active_output)
		*active_output = 0;
	if (output == None)
		return 0;

	output_info = XRRGetOutputInfo(display, resources, output);
	if (!output_info)
		return 0;

	if ((output_info->connection == RR_Connected) &&
	    (output_info->crtc != None)) {
		crtc_info = XRRGetCrtcInfo(display, resources, output_info->crtc);
		if (crtc_info) {
			if (active_output)
				*active_output = 1;
			result = linux_rotation_degrees(crtc_info->rotation);
			XRRFreeCrtcInfo(crtc_info);
		}
	}

	XRRFreeOutputInfo(output_info);
	return result;
}

static int linux_display_rotation(Display *display, int screen)
{
	XRRScreenResources *resources;
	XRRScreenConfiguration *configuration;
	RROutput primary_output;
	int active_output = 0;
	int output_index;
	int result = 0;

	/*
	 * RandR 1.1 only reports screen-wide rotation. Mobile sessions commonly
	 * rotate one RandR 1.2 output, so prefer its active CRTC transform.
	 */
	resources = XRRGetScreenResourcesCurrent(display,
		RootWindow(display, screen));
	if (resources) {
		primary_output = XRRGetOutputPrimary(display,
			RootWindow(display, screen));
		result = linux_output_rotation(display, resources, primary_output,
			&active_output);
		if (!active_output) {
			for (output_index = 0; output_index < resources->noutput;
			     output_index++) {
				if (resources->outputs[output_index] == primary_output)
					continue;
				result = linux_output_rotation(display, resources,
					resources->outputs[output_index], &active_output);
				if (active_output)
					break;
			}
		}
		XRRFreeScreenResources(resources);
		if (active_output)
			return result;
	}

	configuration = XRRGetScreenInfo(display, RootWindow(display, screen));
	if (!configuration)
		return 0;
	{
		Rotation rotation = RR_Rotate_0;
		(void)XRRConfigCurrentConfiguration(configuration, &rotation);
		result = linux_rotation_degrees(rotation);
	}
	XRRFreeScreenConfigInfo(configuration);
	return result;
}
#endif

void fb_hMobileControl(int what, ssize_t *p1, ssize_t *p2,
	ssize_t *p3, ssize_t *p4)
{
	int mir_framebuffer_metrics = 0;

	if (what == GET_DEVICE_CLASS) {
		*p1 = fb_hLinuxDeviceClass();
		*p2 = *p3 = *p4 = 0;
		return;
	}
	if (what == SET_SOFT_KEYBOARD) {
		if (*p1 != (ssize_t)0x80000000 &&
			fb_hWaylandSetKeyboard(*p1 != 0) != 0) {
			uint64_t window_id = 0;
#ifndef DISABLE_X11
			if (fb_x11.display)
				window_id = (uint64_t)fb_x11.window;
#endif
			/* Focus can request the keyboard before the first field rectangle. */
			if (*p1 != 0)
				fb_hLinuxMaliitSetWindowId(window_id);
			fb_hLinuxMaliitSetKeyCallback(linux_maliit_post_key, NULL);
			fb_hLinuxOskRequest(*p1 != 0);
		}
		return;
	}
	if (what == GET_SOFT_KEYBOARD) {
		int visible = -2;
		if (fb_hWaylandGetKeyboardState(&visible)) {
			int observed_visible = -2;
			int observed_available = fb_hLinuxOskQuery(&observed_visible);
			if (observed_available != 0) {
				visible = observed_visible;
				*p2 = observed_available;
			} else {
				*p2 = 1;
			}
		} else {
			*p2 = fb_hLinuxOskQuery(&visible);
		}
#ifndef DISABLE_X11
		/* Hildon's finger keyboard uses its own focused window and editor.
		 * When our X11 window owns focus, that keyboard is inactive. Keep
		 * unknown visibility while another window owns focus, but do not
		 * leave an application permanently panned after the IME closes. */
		if (visible == -2 && *p2 != 0 && fb_hLinuxOskIsHildon()) {
			FB_GRAPHICS_LOCK();
			if (__fb_gfx && fb_x11.display && fb_hX11WindowHasFocus())
				visible = 0;
			FB_GRAPHICS_UNLOCK();
		}
#endif
		*p1 = visible;
		*p3 = *p4 = 0;
		{
			int top, height;
			if (fb_hWaylandGetKeyboardOcclusion(&top, &height)) {
				*p3 = top;
				*p4 = height;
			}
		}
		return;
	}
	if (what == SET_SOFT_KEYBOARD_REGION) {
		uint64_t window_id = 0;
#ifndef DISABLE_X11
		if (fb_x11.display)
			window_id = (uint64_t)fb_x11.window;
#endif
		fb_hLinuxMaliitSetTextRegion((int)*p1, (int)*p2, (int)*p3,
			(int)*p4, window_id);
		(void)fb_hWaylandSetTextInputRegion((int)*p1, (int)*p2,
			(int)*p3, (int)*p4);
		return;
	}
	if (what == CLEAR_SOFT_KEYBOARD_REGIONS) {
		(void)fb_hWaylandClearTextInputRegions();
		return;
	}
	if (what == GET_TOUCH_CAPABILITY) {
		FB_WAYLAND_DISPLAY_INFO info;
		if (fb_hWaylandDisplayInfo(&info) == 0)
			*p1 = info.touch_capable ? -1 : 0;
		else
			*p1 = -2;
		*p2 = *p3 = *p4 = 0;
		return;
	}
	if (what != GET_WINDOW_METRICS)
		return;

	FB_GRAPHICS_LOCK();
	fb_hApplyPendingResize();
	if (__fb_gfx) {
		*p1 = __fb_gfx->w;
		*p2 = __fb_gfx->h;
	} else {
		const char *mir_socket = getenv("MIR_SOCKET");
		const char *display = getenv("DISPLAY");
		if (mir_socket && *mir_socket && display && *display) {
			mir_framebuffer_metrics = fb_hLinuxFramebufferMetrics(
				p1, p2, p3, p4);
			if (!mir_framebuffer_metrics)
				*p1 = *p2 = 0;
		} else {
			ssize_t depth = 0, refresh = 0;
			fb_hScreenInfo(p1, p2, &depth, &refresh);
		}
	}
	if (!mir_framebuffer_metrics) {
		*p3 = 96;
		*p4 = 0;
	}
#ifndef DISABLE_X11
	if (fb_x11.display && !mir_framebuffer_metrics) {
		*p3 = linux_display_dpi(fb_x11.display, fb_x11.screen);
		*p4 = linux_display_rotation(fb_x11.display, fb_x11.screen);
	} else
#endif
	if (!mir_framebuffer_metrics) {
		FB_WAYLAND_DISPLAY_INFO info;
		if (fb_hWaylandDisplayInfo(&info) == 0) {
			/* Before mode setup, the active Wayland output supplies screen size. */
			if (!__fb_gfx && info.width > 0 && info.height > 0) {
				*p1 = info.width;
				*p2 = info.height;
			}
			*p3 = info.dpi;
			*p4 = info.rotation;
		}
	}
	FB_GRAPHICS_UNLOCK();
}

/* end of gfx_mobile.c */
