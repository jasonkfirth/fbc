/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: win32/gfx3_screenlist.c

    Purpose:

        Enumerate Win32 display modes for the public SCREENLIST API.

    Responsibilities:

        - collect modes advertised by EnumDisplaySettings
        - reject dimensions that cannot use the packed SCREENLIST layout
        - pass native modes through the shared checked collection helpers

    This file intentionally does NOT contain:

        - iterator state or standard-mode fallback behavior
        - display-mode changes, fullscreen policy, or window creation
        - non-Win32 display enumeration
*/

#include "../gfx3_screenlist_internal.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int fb_gfx3_platform_screenlist_modes(int depth, int **modes,
	size_t *mode_count)
{
	DEVMODEA mode;
	int *result = NULL;
	size_t count = 0;
	size_t capacity = 0;
	DWORD index;

	if ((modes == NULL) || (mode_count == NULL) || (depth <= 0))
		return FB_GFX3_INVALID;
	*modes = NULL;
	*mode_count = 0;
	for (index = 0; ; ++index) {
		memset(&mode, 0, sizeof(mode));
		mode.dmSize = sizeof(mode);
		if (!EnumDisplaySettingsA(NULL, index, &mode))
			break;
		if (!fb_gfx3_screenlist_depth_matches(mode.dmBitsPerPel, depth) ||
		    (mode.dmPelsWidth == 0) || (mode.dmPelsHeight == 0) ||
		    (mode.dmPelsWidth > 0x7FFFu) || (mode.dmPelsHeight > 0xFFFFu))
			continue;
		if (fb_gfx3_screenlist_append(&result, &count, &capacity,
			((int)mode.dmPelsWidth << 16) | (int)mode.dmPelsHeight) !=
		    FB_GFX3_OK) {
			free(result);
			return FB_GFX3_OUT_OF_MEMORY;
		}
	}
	return fb_gfx3_screenlist_finish(result, count, modes, mode_count);
}

/* end of win32/gfx3_screenlist.c */
