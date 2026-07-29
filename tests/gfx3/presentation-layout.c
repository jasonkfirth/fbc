/*
    Project: FreeBASIC gfxlib3 tests
    --------------------------------

    File: presentation-layout.c

    Purpose:

        Verify the graphics-API-independent fixed-screen presentation layout.

    Responsibilities:

        - verify largest-integer scaling and centered native letterboxing
        - verify one-to-one presentation when the native client is smaller
        - reject invalid logical and unrepresentable native dimensions
        - remain independent of a window system, OpenGL, and Vulkan

    This file intentionally does NOT contain:

        - native window creation
        - displayed-pixel capture
        - mouse event dispatch
*/

#include "../../src/gfxlib3/gfx3_presentation.h"

#include <limits.h>
#include <stdio.h>

static int failures = 0;

#define CHECK(expression) \
	do { \
		if (!(expression)) { \
			fprintf(stderr, "failed: %s at line %d\n", #expression, \
				__LINE__); \
			failures++; \
		} \
	} while (0)

static void check_layout(uint32_t logical_width, uint32_t logical_height,
	uint32_t client_width, uint32_t client_height, int32_t expected_x,
	int32_t expected_y, uint32_t expected_width, uint32_t expected_height,
	uint32_t expected_scale)
{
	FB_GFX3_PRESENTATION_LAYOUT layout;

	CHECK(fb_gfx3_presentation_layout_calculate(logical_width,
		logical_height, client_width, client_height, &layout) ==
		FB_GFX3_OK);
	CHECK(layout.x == expected_x);
	CHECK(layout.y == expected_y);
	CHECK(layout.width == expected_width);
	CHECK(layout.height == expected_height);
	CHECK(layout.scale == expected_scale);
}

int main(void)
{
	FB_GFX3_PRESENTATION_LAYOUT layout;

#if defined(GFXLIB_NEVERSCALE)
	check_layout(160u, 120u, 1366u, 696u, 603, 288, 160u, 120u, 1u);
#else
	check_layout(160u, 120u, 1366u, 696u, 283, 48, 800u, 600u, 5u);
#endif
	check_layout(160u, 120u, 160u, 120u, 0, 0, 160u, 120u, 1u);
	check_layout(160u, 120u, 100u, 50u, -30, -35, 160u, 120u, 1u);
	check_layout(160u, 120u, 0u, 0u, -80, -60, 160u, 120u, 1u);

	CHECK(fb_gfx3_presentation_layout_calculate(0u, 120u, 800u, 600u,
		&layout) == FB_GFX3_INVALID);
	CHECK(fb_gfx3_presentation_layout_calculate(160u, 0u, 800u, 600u,
		&layout) == FB_GFX3_INVALID);
	CHECK(fb_gfx3_presentation_layout_calculate(160u, 120u, 800u, 600u,
		NULL) == FB_GFX3_INVALID);
	CHECK(fb_gfx3_presentation_layout_calculate((uint32_t)INT_MAX + 1u,
		120u, 800u, 600u, &layout) == FB_GFX3_INVALID);
	CHECK(fb_gfx3_presentation_layout_calculate(160u, 120u,
		(uint32_t)INT_MAX + 1u, 600u, &layout) == FB_GFX3_INVALID);

	if (failures != 0)
		return 1;
	puts("gfxlib3 presentation layout: all checks passed");
	return 0;
}

/* end of presentation-layout.c */
