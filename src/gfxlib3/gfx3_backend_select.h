/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_backend_select.h

    Purpose:

        Declare gfxlib3's ordered renderer selection and mode retry policy.

    Responsibilities:

        - build the backend attempt order for the current platform
        - preserve the FBGFX and SCREENCONTROL driver-name override
        - expose a small testable contract to the SCREEN implementation

    This file intentionally does NOT contain:

        - backend initialization or graphics API probing
        - platform window creation
        - public FreeBASIC declarations
*/

#ifndef __FB_GFX3_BACKEND_SELECT_H__
#define __FB_GFX3_BACKEND_SELECT_H__

#include "gfx3_backend.h"

#define FB_GFX3_BACKEND_PLAN_CAPACITY 8u

size_t fb_gfx3_backend_plan(int flags, const char *requested_name,
	const FB_GFX3_BACKEND_VTABLE **plan, size_t capacity);
size_t fb_gfx3_backend_attempt_plan(int flags, const char *requested_name,
	const FB_GFX3_BACKEND_VTABLE **plan, int *attempt_flags,
	size_t capacity);
const char *fb_gfx3_backend_requested_name(void);
int fb_gfx3_backend_set_requested_name(const char *name, size_t length);

#endif

/* end of gfx3_backend_select.h */
