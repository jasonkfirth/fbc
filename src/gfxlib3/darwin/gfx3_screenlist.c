/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: darwin/gfx3_screenlist.c

    Purpose:

        Enumerate display modes for the public SCREENLIST API on macOS.

    Responsibilities:

        - collect CoreGraphics modes advertised by the main display
        - reject dimensions that cannot use the packed SCREENLIST layout
        - pass native modes through the shared checked collection helpers

    This file intentionally does NOT contain:

        - iterator state or standard-mode fallback behavior
        - display-mode changes, fullscreen policy, or window creation
        - Vulkan or Metal integration
*/

#include "../gfx3_screenlist_internal.h"

#include <CoreGraphics/CGDirectDisplay.h>

int fb_gfx3_platform_screenlist_modes(int depth, int **modes,
	size_t *mode_count)
{
	CFArrayRef display_modes;
	CFIndex display_mode_count;
	CFIndex index;
	int *result = NULL;
	size_t count = 0u;
	size_t capacity = 0u;

	if ((modes == NULL) || (mode_count == NULL) || (depth <= 0))
		return FB_GFX3_INVALID;
	*modes = NULL;
	*mode_count = 0u;
	if (!fb_gfx3_screenlist_depth_matches(32u, depth))
		return FB_GFX3_UNSUPPORTED;
	display_modes = CGDisplayCopyAllDisplayModes(CGMainDisplayID(), NULL);
	if (display_modes == NULL)
		return FB_GFX3_UNSUPPORTED;
	display_mode_count = CFArrayGetCount(display_modes);
	for (index = 0; index < display_mode_count; ++index) {
		CGDisplayModeRef display_mode;
		size_t width;
		size_t height;

		display_mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(
			display_modes, index);
		if (display_mode == NULL)
			continue;
		width = CGDisplayModeGetPixelWidth(display_mode);
		height = CGDisplayModeGetPixelHeight(display_mode);
		if ((width == 0u) || (height == 0u) || (width > 0x7FFFu) ||
		    (height > 0xFFFFu))
			continue;
		if (fb_gfx3_screenlist_append(&result, &count, &capacity,
		    ((int)width << 16) | (int)height) != FB_GFX3_OK) {
			free(result);
			CFRelease(display_modes);
			return FB_GFX3_OUT_OF_MEMORY;
		}
	}
	CFRelease(display_modes);
	return fb_gfx3_screenlist_finish(result, count, modes, mode_count);
}

/* end of darwin/gfx3_screenlist.c */
