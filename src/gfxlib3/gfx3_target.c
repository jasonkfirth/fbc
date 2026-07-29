/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_target.c

    Purpose:

        Provide the hosted desktop target policy shared by Linux and Win32.

    Responsibilities:

        - accept the common SCREEN flag set
        - prefer desktop OpenGL before Vulkan
        - retain a bounded desktop CPU image snapshot budget

    This file intentionally does NOT contain:

        - operating-system APIs
        - renderer probing or initialization
        - Android memory and window policy
*/

#include "gfx3_target.h"

#include "gfx3_backend_opengl.h"
#include "gfx3_backend_vulkan.h"

int fb_gfx3_target_screen_flags_valid(uint32_t flags)
{
	(void)flags;
	return TRUE;
}

size_t fb_gfx3_target_backend_default_list(
	const FB_GFX3_BACKEND_VTABLE **backends, size_t capacity)
{
	size_t count = 0;

	if ((backends == NULL) || (capacity == 0u))
		return 0u;
	backends[count++] = &__fb_gfx3_backend_opengl;
	if (count < capacity)
		backends[count++] = &__fb_gfx3_backend_vulkan;
	return count;
}

const FB_GFX3_BACKEND_VTABLE *fb_gfx3_target_opengl_backend(void)
{
	return &__fb_gfx3_backend_opengl;
}

size_t fb_gfx3_target_image_cache_snapshot_budget(void)
{
	return 64u * 1024u * 1024u;
}

/* end of gfx3_target.c */
