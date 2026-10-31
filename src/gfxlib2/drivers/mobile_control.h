/*
 * FreeBASIC graphics: mobile_control.h
 * Purpose: Share window metrics and software-keyboard controls between gfxlibs.
 * Responsibilities: Control IDs and the platform dispatch contract.
 * This file does not own windows, input queues or rendering state.
 */
#ifndef FB_MOBILE_CONTROL_H
#define FB_MOBILE_CONTROL_H

#define GET_WINDOW_METRICS 19
#define GET_SOFT_KEYBOARD 20
#define SET_SOFT_KEYBOARD 119
#define SET_SOFT_KEYBOARD_REGION 120
#define CLEAR_SOFT_KEYBOARD_REGIONS 201

/* Called on the application thread, outside the graphics lock. The platform
 * owns synchronization with native callbacks. Metrics are usable window width,
 * height, layout DPI and clockwise rotation in degrees. Browser sizes are CSS
 * pixels; Android sizes are native pixels. A zero size means unavailable.
 * Keyboard queries return observed visibility and availability separately;
 * visibility -2 means that the platform cannot observe the IME reliably. */
#if defined(HOST_JS) || defined(HOST_ANDROID)
void fb_hMobileControl(int what, ssize_t *p1, ssize_t *p2,
	ssize_t *p3, ssize_t *p4);
#endif

#endif
/* end of mobile_control.h */
