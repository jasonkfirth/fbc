/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_blender_compat.c

    Purpose:

        Preserve the small exported CPU pixel-helper ABI used by FreeBASIC
        programs and the existing gfx test suite.

    Responsibilities:

        - apply gfxlib2-compatible constant-alpha blending to 32-bit pixels
        - accept unaligned destination storage without undefined behavior

    This file intentionally does NOT contain:

        - GPU PUT shaders or surface transfers
        - image clipping, pitch, or coordinate handling
        - MMX, SIMD, or platform-specific acceleration
*/

#include "fb_gfx3.h"

#define FB_GFX3_MASK_RED_BLUE 0x00FF00FFu
#define FB_GFX3_MASK_GREEN    0x0000FF00u
#define FB_GFX3_MASK_ALPHA    0xFF000000u

void *fb_hPixelSetAlpha4(void *destination, int color, size_t count)
{
	unsigned char *output = (unsigned char *)destination;
	uint32_t source = (uint32_t)color;
	uint32_t source_red_blue = source & FB_GFX3_MASK_RED_BLUE;
	uint32_t source_green = source & FB_GFX3_MASK_GREEN;
	uint32_t source_alpha = source & FB_GFX3_MASK_ALPHA;
	uint32_t alpha = source >> 24;
	size_t pixel;

	if ((destination == NULL) && (count != 0))
		return destination;
	for (pixel = 0; pixel < count; pixel++) {
		uint32_t destination_color;
		uint32_t destination_red_blue;
		uint32_t destination_green;
		uint32_t interpolated_red_blue;
		uint32_t interpolated_green;

		memcpy(&destination_color, output, sizeof(destination_color));
		destination_red_blue = destination_color &
			FB_GFX3_MASK_RED_BLUE;
		destination_green = destination_color & FB_GFX3_MASK_GREEN;
		interpolated_red_blue = ((source_red_blue -
			destination_red_blue) * alpha) >> 8;
		interpolated_green = ((source_green - destination_green) *
			alpha) >> 8;
		destination_color = ((destination_red_blue +
			interpolated_red_blue) & FB_GFX3_MASK_RED_BLUE) |
			((destination_green + interpolated_green) &
			FB_GFX3_MASK_GREEN) | source_alpha;
		memcpy(output, &destination_color, sizeof(destination_color));
		output += sizeof(destination_color);
	}
	return destination;
}

/* end of gfx3_blender_compat.c */
