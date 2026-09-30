/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: linux/gfx3_backend_gles.c

    Purpose:

        Enable the shared OpenGL ES renderer on Linux through a native
        platform adapter which supplies GLES entry points at runtime.

    Responsibilities:

        - compile the common GLES renderer for Linux targets
        - keep the GLES implementation optional when GLES headers are absent

    This file intentionally does NOT contain:

        - SDL, EGL, KMS, or input lifecycle code
        - GLES shader or command execution logic
        - a direct link dependency on a GLES library
*/

#if defined(__has_include)
#if __has_include(<GLES3/gl3.h>)
#define FB_GFX3_GLES_DYNAMIC 1
#include "../android/gfx3_backend_gles.c"
#else
#include "../gfx3_backend_gles.c"
#endif
#else
#include "../gfx3_backend_gles.c"
#endif

/* end of linux/gfx3_backend_gles.c */
