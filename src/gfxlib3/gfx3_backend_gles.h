/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_backend_gles.h

    Purpose:

        Declare the OpenGL ES renderer used on Android-class devices.

    Responsibilities:

        - expose the GLES backend vtable to renderer selection code

    This file intentionally does NOT contain:

        - EGL or OpenGL ES declarations
        - shader programs or platform lifecycle state
        - compatibility front-end behavior
*/

#ifndef __FB_GFX3_BACKEND_GLES_H__
#define __FB_GFX3_BACKEND_GLES_H__

#include "gfx3_backend.h"

extern const FB_GFX3_BACKEND_VTABLE __fb_gfx3_backend_gles;

#endif

/* end of gfx3_backend_gles.h */
