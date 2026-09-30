/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: linux/gfx3_platform_drm_input.h

    Purpose:

        Declare the Linux evdev input adapter used by the native DRM platform.

    Responsibilities:

        - keep evdev descriptors separate from display management
        - publish keyboard and gamepad snapshots into shared input state

    This file intentionally does NOT contain:

        - DRM, GBM, EGL, X11, or SDL declarations
        - FreeBASIC renderer commands
        - public graphics API declarations
*/

#ifndef __FB_GFX3_PLATFORM_DRM_INPUT_H__
#define __FB_GFX3_PLATFORM_DRM_INPUT_H__

#include "../gfx3_input.h"

typedef struct FB_GFX3_DRM_INPUT_ADAPTER FB_GFX3_DRM_INPUT_ADAPTER;

FB_GFX3_DRM_INPUT_ADAPTER *fb_gfx3_platform_drm_input_create(
	FB_GFX3_INPUT_STATE *input);
void fb_gfx3_platform_drm_input_pump(
	FB_GFX3_DRM_INPUT_ADAPTER *adapter);
void fb_gfx3_platform_drm_input_destroy(
	FB_GFX3_DRM_INPUT_ADAPTER *adapter);

#endif

/* end of linux/gfx3_platform_drm_input.h */
