/*
    Project: FreeBASIC gfxlib3 tests
    --------------------------------

    File: x11-screenlist-live.c

    Purpose:

        Exercise gfxlib3's optional XRandR SCREENLIST boundary against a live
        X11 display.

    Responsibilities:

        - require native modes for the supported 8, 16, and 32-bit families
        - verify 15/16 and 24/32 compatibility depth aliases
        - verify the packed results are sorted and unique
        - reject unsupported historical depths without returning storage

    This file intentionally does NOT contain:

        - SCREENLIST iterator or standard-mode fallback testing
        - window creation, OpenGL, or Vulkan rendering
        - X server setup or teardown
*/

#include "../../src/gfxlib3/gfx3_screenlist.h"

#include <stdio.h>
#include <stdlib.h>

static int failures;

static void check(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "x11 screenlist failure: %s\n", message);
		failures++;
	}
}

static void verify_mode_list(int depth, int required)
{
	int *modes = NULL;
	size_t count = 0;
	size_t index;
	int result;

	result = fb_gfx3_platform_screenlist_modes(depth, &modes, &count);
	if (!required) {
		check(result == FB_GFX3_UNSUPPORTED,
			"unsupported depth did not return FB_GFX3_UNSUPPORTED");
		check(modes == NULL, "unsupported depth returned mode storage");
		check(count == 0, "unsupported depth returned mode count");
		free(modes);
		return;
	}

	check(result == FB_GFX3_OK, "supported depth was not enumerated");
	check(modes != NULL, "supported depth returned no mode storage");
	check(count != 0, "supported depth returned no modes");
	for (index = 0; index < count; ++index) {
		unsigned int width = ((unsigned int)modes[index] >> 16) & 0xFFFFu;
		unsigned int height = (unsigned int)modes[index] & 0xFFFFu;

		check((width != 0u) && (height != 0u),
			"mode has a zero dimension");
		if (index != 0) {
			unsigned int previous_width =
				((unsigned int)modes[index - 1u] >> 16) & 0xFFFFu;
			unsigned int previous_height =
				(unsigned int)modes[index - 1u] & 0xFFFFu;

			check((width > previous_width) ||
				((width == previous_width) && (height > previous_height)),
				"modes are not sorted unique width/height values");
		}
	}
	free(modes);
}

static void verify_alias(int first_depth, int second_depth)
{
	int *first_modes = NULL;
	int *second_modes = NULL;
	size_t first_count = 0;
	size_t second_count = 0;
	size_t index;

	check(fb_gfx3_platform_screenlist_modes(first_depth, &first_modes,
		&first_count) == FB_GFX3_OK, "first depth alias was not enumerated");
	check(fb_gfx3_platform_screenlist_modes(second_depth, &second_modes,
		&second_count) == FB_GFX3_OK, "second depth alias was not enumerated");
	check(first_count == second_count, "depth aliases have different counts");
	if ((first_modes != NULL) && (second_modes != NULL) &&
		(first_count == second_count)) {
		for (index = 0; index < first_count; ++index) {
			check(first_modes[index] == second_modes[index],
				"depth aliases have different modes");
		}
	}
	free(first_modes);
	free(second_modes);
}

int main(void)
{
	verify_mode_list(8, 1);
	verify_mode_list(16, 1);
	verify_mode_list(32, 1);
	verify_alias(15, 16);
	verify_alias(24, 32);
	verify_mode_list(1, 0);
	verify_mode_list(3, 0);

	if (failures != 0)
		return 1;
	puts("gfxlib3 X11/RandR screenlist passed");
	return 0;
}

/* end of x11-screenlist-live.c */
