/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_png.h

    Purpose:

        Declare gfxlib3's dependency-free PNG reader and writer.

    Responsibilities:

        - expose PNG file load and save helpers to the BLOAD/BSAVE module
        - keep PNG implementation details out of the public runtime ABI

    This file intentionally does NOT contain:

        - public BASIC declarations
        - filename or file-signature dispatch
        - PNG codec implementation
*/

#ifndef __FB_GFX3_PNG_H__
#define __FB_GFX3_PNG_H__

#include "gfx3_file_api.h"

int fb_gfx3_png_load_locked(FILE *file, void *destination, void *palette);
int fb_gfx3_png_load_pixels_locked(FILE *file, uint32_t depth,
	unsigned char **pixels, uint32_t *width, uint32_t *height,
	uint32_t *pitch);
int fb_gfx3_png_save_locked(FILE *file, const FB_GFX3_FILE_VIEW *view,
	void *palette, int bits_per_pixel);

#endif

/* end of gfx3_png.h */
