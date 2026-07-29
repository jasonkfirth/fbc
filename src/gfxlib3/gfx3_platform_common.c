/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_platform_common.c

    Purpose:

        Provide window-system-independent presentation coordinate helpers.

    Responsibilities:

        - calculate the shared integer-scaled presentation layout
        - translate between native client and logical framebuffer coordinates

    This file intentionally does NOT contain:

        - native window-system calls or adapter selection
        - OpenGL or Vulkan object lifecycle
        - runtime fallback between different desktop protocols
*/

#include "gfx3_platform.h"

/* ------------------------------------------------------------------------- */
/* Fixed-screen presentation layout                                          */
/* ------------------------------------------------------------------------- */

int fb_gfx3_platform_presentation_layout(uint32_t logical_width,
	uint32_t logical_height, uint32_t client_width, uint32_t client_height,
	FB_GFX3_PRESENTATION_LAYOUT *layout)
{
	return fb_gfx3_presentation_layout_calculate(logical_width,
		logical_height, client_width, client_height, layout);
}

int fb_gfx3_platform_client_to_logical(
	const FB_GFX3_PRESENTATION_LAYOUT *layout, uint32_t logical_width,
	uint32_t logical_height, int client_x, int client_y, int *logical_x,
	int *logical_y)
{
	int64_t relative_x;
	int64_t relative_y;
	int mapped_x;
	int mapped_y;

	if ((layout == NULL) || (logical_width == 0u) ||
	    (logical_height == 0u) || (layout->width == 0u) ||
	    (layout->height == 0u) || (layout->scale == 0u) ||
	    (logical_width > INT_MAX) || (logical_height > INT_MAX))
		return FB_GFX3_INVALID;
	relative_x = (int64_t)client_x - layout->x;
	relative_y = (int64_t)client_y - layout->y;
	if (relative_x < 0)
		relative_x = 0;
	else if (relative_x >= layout->width)
		relative_x = (int64_t)layout->width - 1;
	if (relative_y < 0)
		relative_y = 0;
	else if (relative_y >= layout->height)
		relative_y = (int64_t)layout->height - 1;
	mapped_x = (int)(relative_x / layout->scale);
	mapped_y = (int)(relative_y / layout->scale);
	if (mapped_x >= (int)logical_width)
		mapped_x = (int)logical_width - 1;
	if (mapped_y >= (int)logical_height)
		mapped_y = (int)logical_height - 1;
	if (logical_x != NULL)
		*logical_x = mapped_x;
	if (logical_y != NULL)
		*logical_y = mapped_y;
	return FB_GFX3_OK;
}

int fb_gfx3_platform_logical_to_client(
	const FB_GFX3_PRESENTATION_LAYOUT *layout, uint32_t logical_width,
	uint32_t logical_height, int logical_x, int logical_y, int *client_x,
	int *client_y)
{
	int64_t mapped_x;
	int64_t mapped_y;

	if ((layout == NULL) || (logical_width == 0u) ||
	    (logical_height == 0u) || (layout->width == 0u) ||
	    (layout->height == 0u) || (layout->scale == 0u) ||
	    (logical_width > INT_MAX) || (logical_height > INT_MAX))
		return FB_GFX3_INVALID;
	if (logical_x < 0)
		logical_x = 0;
	else if (logical_x >= (int)logical_width)
		logical_x = (int)logical_width - 1;
	if (logical_y < 0)
		logical_y = 0;
	else if (logical_y >= (int)logical_height)
		logical_y = (int)logical_height - 1;
	mapped_x = (int64_t)layout->x +
		((int64_t)logical_x * layout->scale) + (layout->scale / 2u);
	mapped_y = (int64_t)layout->y +
		((int64_t)logical_y * layout->scale) + (layout->scale / 2u);
	if ((mapped_x < INT_MIN) || (mapped_x > INT_MAX) ||
	    (mapped_y < INT_MIN) || (mapped_y > INT_MAX))
		return FB_GFX3_INVALID;
	if (client_x != NULL)
		*client_x = (int)mapped_x;
	if (client_y != NULL)
		*client_y = (int)mapped_y;
	return FB_GFX3_OK;
}

/* end of gfx3_platform_common.c */
