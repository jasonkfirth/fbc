/*
 * FreeBASIC graphics: mobile_control.h
 * Purpose: Share window metrics and software-keyboard controls between gfxlibs.
 * Responsibilities: Control IDs and the platform dispatch contract.
 * This file does not own windows, input queues or rendering state.
 */
#ifndef FB_MOBILE_CONTROL_H
#define FB_MOBILE_CONTROL_H

#if defined(HOST_JS) || defined(HOST_ANDROID) || defined(HOST_LINUX)
#include <sys/types.h>
#endif

#define GET_WINDOW_METRICS 19
#define GET_SOFT_KEYBOARD 20
#define GET_TOUCH_CAPABILITY 21
#define GET_DEVICE_CLASS 22
#define SET_SOFT_KEYBOARD 119
#define SET_SOFT_KEYBOARD_REGION 120
#define CLEAR_SOFT_KEYBOARD_REGIONS 201

#define FB_GFX_DEVICE_CLASS_UNKNOWN 0
#define FB_GFX_DEVICE_CLASS_DESKTOP 1
#define FB_GFX_DEVICE_CLASS_HANDHELD 2

/* Called on the application thread, outside the graphics lock. The platform
 * owns synchronization with native callbacks. Metrics are usable window width,
 * height, layout DPI and clockwise rotation in degrees. Browser sizes are CSS
 * pixels; Android sizes are native pixels. A zero size means unavailable.
 * Touch capability returns -1 for present, 0 for absent, and -2 when unknown.
 * Device class returns unknown, desktop, or handheld. Keyboard queries return
 * observed visibility and availability separately; visibility -2 means that
 * the platform cannot observe the IME reliably. The third/fourth outputs may
 * report measured overlap top/height in framebuffer pixels. Zero/zero means
 * unknown geometry; a known-visible keyboard with zero height at the drawable
 * end confirms that layout space was already reserved outside that page. */
#if defined(HOST_JS) || defined(HOST_ANDROID) || defined(HOST_LINUX)
void fb_hMobileControl(int what, ssize_t *p1, ssize_t *p2,
	ssize_t *p3, ssize_t *p4);
#endif

#endif
/* end of mobile_control.h */
