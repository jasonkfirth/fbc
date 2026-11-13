/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: linux/gfx3_platform_wayland.h

    Purpose:

        Declare the compositor-managed Wayland adapter used by gfxlib3.

    Responsibilities:

        - select Wayland when a Wayland session is available
        - expose the Wayland EGL/GLES platform vtable

    This file intentionally does NOT contain:

        - Wayland protocol state or EGL resource ownership
        - rendering commands or mobile form-factor policy
*/

#ifndef FB_GFX3_PLATFORM_WAYLAND_H
#define FB_GFX3_PLATFORM_WAYLAND_H

#include "../../gfx3_platform.h"

const FB_GFX3_PLATFORM_VTABLE *fb_gfx3_platform_wayland(void);
int fb_gfx3_platform_wayland_should_default(void);

#endif

/* end of linux/gfx3_platform_wayland.h */

/* end of gfx3_platform_wayland.h */
