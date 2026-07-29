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

#ifndef __FB_GFX3_BACKEND_OPENGL_H__
#define __FB_GFX3_BACKEND_OPENGL_H__

#include "gfx3_backend.h"

extern const FB_GFX3_BACKEND_VTABLE __fb_gfx3_backend_opengl;

#endif

/* end of gfx3_backend_opengl.h */
