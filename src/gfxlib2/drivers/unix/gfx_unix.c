/*
 * Project: FreeBASIC gfxlib2 Unix graphics drivers
 * File: gfx_unix.c
 * Purpose: Select Unix graphics backends and report display dimensions.
 * Responsibilities: Assemble the driver list and query the active display.
 * This file intentionally does not implement window or rendering callbacks.
 */
#include "../../fb_gfx.h"
#include "fb_gfx_x11.h"

#ifdef HOST_LINUX
#include "../linux/fb_gfx_linux.h"
#endif

#if defined HOST_FREEBSD || defined HOST_OPENBSD || defined HOST_LINUX || defined HOST_DARWIN || defined HOST_SOLARIS || defined HOST_DRAGONFLY || defined HOST_NETBSD

const GFXDRIVER *__fb_gfx_drivers_list[] = {

#if defined HOST_LINUX && !defined DISABLE_WAYLAND
	&fb_gfxDriverWayland,
#endif

#ifndef DISABLE_X11
	&fb_gfxDriverX11,
#ifndef DISABLE_OPENGL
	&fb_gfxDriverOpenGL,
#endif

#endif

#if defined HOST_LINUX && !defined DISABLE_FBDEV
	&fb_gfxDriverFBDev,
#endif

	NULL
};

void fb_hScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh)
{
#ifdef HOST_LINUX
	if (getenv("WAYLAND_DISPLAY") && fb_hWaylandShouldUse()) {
		FB_WAYLAND_DISPLAY_INFO info;
		if (fb_hWaylandDisplayInfo(&info) == 0) {
			*width = info.width;
			*height = info.height;
			*depth = info.depth;
			*refresh = info.refresh;
			return;
		}
	}
#endif
#ifndef DISABLE_X11
	/* A successful X11 query already supplied the active desktop dimensions. */
	if (fb_hX11ScreenInfo(width, height, depth, refresh) == 0)
		return;
#endif
#ifdef HOST_LINUX
	if (fb_hWaylandShouldUse()) {
		FB_WAYLAND_DISPLAY_INFO info;
		if (fb_hWaylandDisplayInfo(&info) == 0) {
			*width = info.width;
			*height = info.height;
			*depth = info.depth;
			*refresh = info.refresh;
			return;
		}
	}
#endif
#if defined HOST_LINUX && !defined DISABLE_FBDEV
	if (fb_hFBDevInfo(width, height, depth, refresh))
		return;
#endif
	*width = *height = *depth = *refresh = 0;
}

#endif

/* end of gfx_unix.c */
