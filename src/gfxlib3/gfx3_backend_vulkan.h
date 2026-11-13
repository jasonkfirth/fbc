/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_backend_vulkan.h

    Purpose:

        Declare the Vulkan renderer backend used by the common render thread.

    Responsibilities:

        - expose the Vulkan backend vtable to selection and tests

    This file intentionally does NOT contain:

        - Vulkan ABI declarations
        - renderer implementation state
        - FreeBASIC compatibility state
*/

#ifndef FB_GFX3_BACKEND_VULKAN_H
#define FB_GFX3_BACKEND_VULKAN_H

#include "gfx3_backend.h"

extern const FB_GFX3_BACKEND_VTABLE fb_gfx3_backend_vulkan FB_C_SYMBOL("__fb_gfx3_backend_vulkan");

#endif

/* end of gfx3_backend_vulkan.h */
