/*
 * FreeBASIC gfxlib3 Linux graphics: gfx3_mobile.h
 * Purpose: Declare Linux display metrics and session keyboard controls.
 * Responsibilities: Keep the public compatibility API separate from X11 and
 *     session-bus implementation details.
 * This file intentionally does NOT contain: rendering or application layout.
 */
#ifndef FB_GFX3_LINUX_MOBILE_H
#define FB_GFX3_LINUX_MOBILE_H

#include <sys/types.h>
#include "../../gfx3_compat.h"

void fb_gfx3_linux_display_metrics(ssize_t *dpi, ssize_t *rotation);
int fb_gfx3_linux_initial_window_metrics(ssize_t *width, ssize_t *height,
	ssize_t *dpi, ssize_t *rotation);
void fb_gfx3_linux_mobile_control(int what, ssize_t *p1, ssize_t *p2,
	ssize_t *p3, ssize_t *p4, FB_GFX3_INPUT_STATE *input);

/* A transition owns the lease until the replacement mode is ready. */
void fb_gfx3_linux_mode_transition_begin(void);
void fb_gfx3_linux_mode_transition_end(FB_GFX3_MODE *mode);
/* A caller must release even when acquire returns NULL. */
FB_GFX3_MODE *fb_gfx3_linux_mode_acquire(void);
void fb_gfx3_linux_mode_release(void);

/* Called by the public control API with non-NULL parameter pointers. */
void fb_gfx3_linux_control_mode(int what, ssize_t *p1, ssize_t *p2,
	ssize_t *p3, ssize_t *p4);

#endif
/* end of gfx3_mobile.h */
