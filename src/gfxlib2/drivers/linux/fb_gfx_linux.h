/*
 * Project: FreeBASIC gfxlib2 Linux drivers
 * File: fb_gfx_linux.h
 * Purpose: Declare the shared Linux graphics backend interfaces.
 * Responsibilities: Driver declarations and output capability structures.
 * This file intentionally does not contain driver implementations or policy.
 */

#ifndef FB_GFX_LINUX_H
#define FB_GFX_LINUX_H

#include "fb_wayland_display.h"
#include "fb_wayland_client.h"

#ifndef DISABLE_FBDEV

#define DOUBLE_CLICK_TIME		250

extern const GFXDRIVER fb_gfxDriverFBDev;
extern const GFXDRIVER fb_gfxDriverWayland;

extern int fb_hFBDevInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh);

#endif

#endif

/* end of fb_gfx_linux.h */
