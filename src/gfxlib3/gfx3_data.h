/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_data.h

    Purpose:

        Expose immutable compatibility data used by gfxlib3 drawing APIs.

    Responsibilities:

		- provide the standard FreeBASIC 8 by 8, 8 by 14, and 8 by 16 fonts
        - hide one-time decompression and storage ownership

    This file intentionally does NOT contain:

        - text layout or rasterization
        - mutable screen or palette state
        - file-format codecs
*/

#ifndef __FB_GFX3_DATA_H__
#define __FB_GFX3_DATA_H__

#include "fb_gfx3.h"

const unsigned char *fb_gfx3_data_font_8x8(void);
const unsigned char *fb_gfx3_data_font_8x14(void);
const unsigned char *fb_gfx3_data_font_8x16(void);
const unsigned char *fb_gfx3_data_palette_256(void);

#endif

/* end of gfx3_data.h */
