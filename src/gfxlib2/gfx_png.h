/*
    FreeBASIC Graphics Library
    --------------------------

    File: gfx_png.h

    Purpose:

        Declare the dependency-free PNG reader and writer used by BLOAD and
        BSAVE.

    Responsibilities:

        - expose PNG file load and save helpers inside gfxlib2
        - keep PNG implementation details out of the public runtime ABI

    This file intentionally does NOT contain:

        - public BASIC declarations
        - file-format selection by filename or signature
        - PNG codec implementation
*/

#ifndef __FB_GFX_PNG_H__
#define __FB_GFX_PNG_H__

#include "fb_gfx.h"

int fb_hGfxPngLoad(FB_GFXCTX *ctx, FILE *file, void *dest, void *pal,
                   int usenewheader);
int fb_hGfxPngSave(FB_GFXCTX *ctx, FILE *file, void *src, void *pal,
                   int bitsperpixel);

#endif

/* end of gfx_png.h */
