/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_file_api.h

    Purpose:

        Expose the private bitmap decoder to the GPU asset loader.

    Responsibilities:

        - decode one BLOAD-compatible bitmap into owned staging pixels
        - report the exact staging layout selected for a GPU surface

    This file intentionally does NOT contain:

        - public FreeBASIC declarations
        - GPU allocation or upload commands
        - image format implementation details
*/

#ifndef __FB_GFX3_FILE_API_H__
#define __FB_GFX3_FILE_API_H__

#include "fb_gfx3.h"

int fb_gfx3_file_load_bitmap_pixels_locked(FBSTRING *filename,
	uint32_t depth, unsigned char **pixels, uint32_t *width,
	uint32_t *height, uint32_t *pitch);

#endif

/* end of gfx3_file_api.h */
