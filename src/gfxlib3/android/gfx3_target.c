/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: android/gfx3_target.c

    Purpose:

        Provide gfxlib3 renderer and memory policy for Android devices.

    Responsibilities:

        - reject the unsupported resizable-window SCREEN flag
        - prefer Vulkan while retaining GLES for older Android devices
        - map the legacy OpenGL flag to GLES
        - bound retained CPU image snapshots for mobile memory budgets

    This file intentionally does NOT contain:

        - NativeActivity lifecycle or window calls
        - EGL, GLES, or Vulkan initialization
        - input event handling
*/

#include "../gfx3_target.h"

#include "../gfx3_backend_gles.h"
#include "../gfx3_backend_vulkan.h"

#define FB_GFX3_SCREEN_RESIZABLE 0x00000400u

int fb_gfx3_target_screen_flags_valid(uint32_t flags)
{
	return (flags & FB_GFX3_SCREEN_RESIZABLE) == 0u;
}

size_t fb_gfx3_target_backend_default_list(
	const FB_GFX3_BACKEND_VTABLE **backends, size_t capacity)
{
	size_t count = 0;

	if ((backends == NULL) || (capacity == 0u))
		return 0u;
	backends[count++] = &__fb_gfx3_backend_vulkan;
	if (count < capacity)
		backends[count++] = &__fb_gfx3_backend_gles;
	return count;
}

const FB_GFX3_BACKEND_VTABLE *fb_gfx3_target_opengl_backend(void)
{
	return &__fb_gfx3_backend_gles;
}

size_t fb_gfx3_target_image_cache_snapshot_budget(void)
{
	return 24u * 1024u * 1024u;
}

/* end of android/gfx3_target.c */
