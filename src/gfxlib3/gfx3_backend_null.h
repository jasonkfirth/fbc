/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_backend_null.h

    Purpose:

		Declare the deterministic headless CPU reference backend.

    Responsibilities:

        - expose the null backend vtable

    This file intentionally does NOT contain:

        - null surface storage
        - raster algorithms
        - renderer lifecycle code
*/

#ifndef FB_GFX3_BACKEND_NULL_H
#define FB_GFX3_BACKEND_NULL_H

#include "gfx3_backend.h"

extern const FB_GFX3_BACKEND_VTABLE fb_gfx3_backend_null FB_C_SYMBOL("__fb_gfx3_backend_null");

#endif

/* end of gfx3_backend_null.h */
