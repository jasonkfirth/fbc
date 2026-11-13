/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_backend_opengl.h

    Purpose:

        Declare the compute-oriented OpenGL renderer backend.

    Responsibilities:

        - expose the OpenGL backend vtable to backend selection code

    This file intentionally does NOT contain:

        - OpenGL declarations or shader source
        - platform context creation details
        - compatibility front-end state
*/

#ifndef FB_GFX3_BACKEND_OPENGL_H
#define FB_GFX3_BACKEND_OPENGL_H

#include "gfx3_backend.h"

extern const FB_GFX3_BACKEND_VTABLE fb_gfx3_backend_opengl FB_C_SYMBOL("__fb_gfx3_backend_opengl");

#endif

/* end of gfx3_backend_opengl.h */
