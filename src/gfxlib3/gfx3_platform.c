/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_platform.c

    Purpose:

        Provide the platform selector for targets without a native gfxlib3
        window-system adapter.

    Responsibilities:

        - report that no native platform adapter is available
        - preserve the common backend contract on unsupported targets

    This file intentionally does NOT contain:

        - native window-system calls
        - supported-platform selection
        - presentation coordinate calculations
*/

#include "gfx3_platform.h"

const FB_GFX3_PLATFORM_VTABLE *fb_gfx3_platform_default(void)
{
	return NULL;
}

int fb_gfx3_platform_keyboard_overlay(void *platform,
	FB_GFX3_ANDROID_KEYBOARD_OVERLAY *overlay)
{
	(void)platform;
	if (overlay != NULL)
		memset(overlay, 0, sizeof(*overlay));
	return FB_GFX3_UNSUPPORTED;
}

/* end of gfx3_platform.c */
