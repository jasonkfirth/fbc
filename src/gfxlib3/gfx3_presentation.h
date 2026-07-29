/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_presentation.h

    Purpose:

        Define the graphics-API-independent layout of a fixed logical screen
        inside a larger native presentation area.

    Responsibilities:

        - select the largest whole-number scale that fits the client area
        - center the scaled logical page
        - preserve the one-to-one layout when scaling is disabled at build time
        - reject dimensions which cannot be represented by backend coordinates

    This file intentionally does NOT contain:

        - native window-system calls
        - OpenGL or Vulkan commands
        - mouse or touch event state
*/

#ifndef __FB_GFX3_PRESENTATION_H__
#define __FB_GFX3_PRESENTATION_H__

#include "fb_gfx3.h"

#include <limits.h>

typedef struct FB_GFX3_PRESENTATION_LAYOUT {
	int32_t x;
	int32_t y;
	uint32_t width;
	uint32_t height;
	uint32_t scale;
} FB_GFX3_PRESENTATION_LAYOUT;

static __inline int fb_gfx3_presentation_layout_calculate(
	uint32_t logical_width, uint32_t logical_height, uint32_t client_width,
	uint32_t client_height, FB_GFX3_PRESENTATION_LAYOUT *layout)
{
	uint32_t scale = 1u;
	uint64_t scaled_width;
	uint64_t scaled_height;
	int64_t offset_x;
	int64_t offset_y;

	if ((layout == NULL) || (logical_width == 0u) ||
	    (logical_height == 0u) || (logical_width > INT_MAX) ||
	    (logical_height > INT_MAX) || (client_width > INT_MAX) ||
	    (client_height > INT_MAX))
		return FB_GFX3_INVALID;
#if !defined(GFXLIB_NEVERSCALE)
	if ((client_width >= logical_width) &&
	    (client_height >= logical_height)) {
		uint32_t scale_x = client_width / logical_width;
		uint32_t scale_y = client_height / logical_height;

		scale = (scale_x < scale_y) ? scale_x : scale_y;
		if (scale == 0u)
			scale = 1u;
	}
#endif
	scaled_width = (uint64_t)logical_width * scale;
	scaled_height = (uint64_t)logical_height * scale;
	if ((scaled_width > INT_MAX) || (scaled_height > INT_MAX))
		return FB_GFX3_INVALID;
	offset_x = ((int64_t)client_width - (int64_t)scaled_width) / 2;
	offset_y = ((int64_t)client_height - (int64_t)scaled_height) / 2;
	if ((offset_x < INT32_MIN) || (offset_x > INT32_MAX) ||
	    (offset_y < INT32_MIN) || (offset_y > INT32_MAX))
		return FB_GFX3_INVALID;
	layout->x = (int32_t)offset_x;
	layout->y = (int32_t)offset_y;
	layout->width = (uint32_t)scaled_width;
	layout->height = (uint32_t)scaled_height;
	layout->scale = scale;
	return FB_GFX3_OK;
}

#endif

/* end of gfx3_presentation.h */
