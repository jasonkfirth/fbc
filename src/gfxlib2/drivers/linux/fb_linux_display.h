/*
 * Project: FreeBASIC Linux graphics support
 * File: fb_linux_display.h
 * Purpose: Expose bounded display geometry from the Linux framebuffer sysfs ABI.
 * Responsibilities: Declare the shared Mir-session geometry fallback.
 * This file intentionally does NOT contain display-server connections or policy.
 */
#ifndef FB_LINUX_DISPLAY_H
#define FB_LINUX_DISPLAY_H

#include <sys/types.h>

int fb_hLinuxFramebufferMetrics(ssize_t *width, ssize_t *height,
	ssize_t *dpi, ssize_t *rotation);

#endif
/* end of fb_linux_display.h */
