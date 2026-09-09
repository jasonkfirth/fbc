/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: darwin/gfx3_target.c

    Purpose:

        Define the gfxlib3 renderer and memory policy for macOS.

    Responsibilities:

        - select Vulkan through MoltenVK as the native renderer
        - reject OpenGL selection because Apple exposes at most OpenGL 4.1
        - retain the hosted desktop CPU image snapshot budget

    This file intentionally does NOT contain:

        - Cocoa window management
        - MoltenVK loading or Metal surface creation
        - renderer implementation details
*/

#include "../gfx3_target.h"

#include "../gfx3_backend_vulkan.h"

int fb_gfx3_target_screen_flags_valid(uint32_t flags)
{
	(void)flags;
	return TRUE;
}

size_t fb_gfx3_target_backend_default_list(
	const FB_GFX3_BACKEND_VTABLE **backends, size_t capacity)
{
	if ((backends == NULL) || (capacity == 0u))
		return 0u;
	backends[0] = &__fb_gfx3_backend_vulkan;
	return 1u;
}

const FB_GFX3_BACKEND_VTABLE *fb_gfx3_target_opengl_backend(void)
{
	/* Apple's deprecated OpenGL implementation cannot provide compute shaders. */
	return NULL;
}

size_t fb_gfx3_target_image_cache_snapshot_budget(void)
{
	return 64u * 1024u * 1024u;
}

/* end of darwin/gfx3_target.c */
