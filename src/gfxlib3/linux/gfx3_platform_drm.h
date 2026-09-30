/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: linux/gfx3_platform_drm.h

    Purpose:

        Declare the native Linux DRM/GBM/EGL display adapter.

    Responsibilities:

        - expose the direct display adapter to Linux platform selection
        - report when a console session should select direct DRM

    This file intentionally does NOT contain:

        - DRM, GBM, EGL, or evdev declarations
        - GLES rendering or command execution
        - public FreeBASIC interfaces
*/

#ifndef __FB_GFX3_PLATFORM_DRM_H__
#define __FB_GFX3_PLATFORM_DRM_H__

#include "../gfx3_platform.h"

const FB_GFX3_PLATFORM_VTABLE *fb_gfx3_platform_drm(void);
int fb_gfx3_platform_drm_should_default(void);

#endif

/* end of linux/gfx3_platform_drm.h */
