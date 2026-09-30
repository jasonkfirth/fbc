/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: linux/gfx3_target.c

    Purpose:

        Choose Linux renderer preference from the active display environment.

    Responsibilities:

        - prefer OpenGL ES on native DRM/KMS handheld displays
        - retain desktop OpenGL and Vulkan preferences under X11
        - reject resizable requests on KMSDRM fullscreen displays
        - reduce retained image snapshots for handheld memory budgets

    This file intentionally does NOT contain:

        - renderer initialization or command execution
        - DRM, X11, EGL, or Vulkan lifecycle code
        - public FreeBASIC SCREEN behavior
*/

#include "../gfx3_target.h"

#include "../gfx3_backend_gles.h"
#include "../gfx3_backend_opengl.h"
#include "../gfx3_backend_vulkan.h"
#include "gfx3_platform_drm.h"

#define FB_GFX3_SCREEN_RESIZABLE 0x00000400u

int fb_gfx3_target_screen_flags_valid(uint32_t flags)
{
	/* The legacy null backend uses -1 as its complete mode sentinel. */
	if (flags == UINT32_MAX)
		return TRUE;
	if (fb_gfx3_platform_drm_should_default())
		return (flags & FB_GFX3_SCREEN_RESIZABLE) == 0u;
	return TRUE;
}

size_t fb_gfx3_target_backend_default_list(
	const FB_GFX3_BACKEND_VTABLE **backends, size_t capacity)
{
	const FB_GFX3_BACKEND_VTABLE *preferred[3];
	size_t count = 0;
	size_t preferred_count;
	size_t index;

	if ((backends == NULL) || (capacity == 0u))
		return 0u;
	if (fb_gfx3_platform_drm_should_default()) {
		preferred[0] = &__fb_gfx3_backend_gles;
		preferred_count = 1u;
	} else {
		preferred[0] = &__fb_gfx3_backend_opengl;
		preferred[1] = &__fb_gfx3_backend_vulkan;
		preferred[2] = &__fb_gfx3_backend_gles;
		preferred_count = sizeof(preferred) / sizeof(preferred[0]);
	}
	for (index = 0; (index < preferred_count) && (count < capacity); index++)
		backends[count++] = preferred[index];
	return count;
}

const FB_GFX3_BACKEND_VTABLE *fb_gfx3_target_opengl_backend(void)
{
	if (fb_gfx3_platform_drm_should_default())
		return &__fb_gfx3_backend_gles;
	return &__fb_gfx3_backend_opengl;
}

size_t fb_gfx3_target_image_cache_snapshot_budget(void)
{
	if (fb_gfx3_platform_drm_should_default())
		return 24u * 1024u * 1024u;
	return 64u * 1024u * 1024u;
}

/* end of linux/gfx3_target.c */
