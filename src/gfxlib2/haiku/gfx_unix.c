/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: gfx_unix.c

    Purpose:

        Provide the screen information hook expected by the shared
        gfxlib screen-query code.

    Responsibilities:

        • report desktop dimensions when no graphics mode is active
        • report active mode dimensions when the Haiku backend is running
        • satisfy the fb_hScreenInfo() symbol required by gfx_screeninfo.c

    This file intentionally does NOT contain:

        • driver registration
        • window creation
        • event handling
        • rendering code
*/

#ifndef DISABLE_HAIKU

#include "../fb_gfx.h"
#include "fb_gfx_haiku.h"

void fb_hScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh)
{
    if (fb_haiku.initialized) {
        if (width) {
            *width = fb_haiku.width;
        }
        if (height) {
            *height = fb_haiku.height;
        }
        if (depth) {
            *depth = fb_haiku.depth;
        }
        if (refresh) {
            *refresh = fb_haiku.refresh;
        }
        return;
    }

    /*
        Query the desktop size on demand so SCREENINFO still reports
        useful dimensions before SCREENRES/SCREEN has initialized the
        graphics backend.
    */
    fb_hHaikuQueryDesktop();

    if (width) {
        *width = fb_haiku.desktop_width;
    }
    if (height) {
        *height = fb_haiku.desktop_height;
    }
    if (depth) {
        *depth = 0;
    }
    if (refresh) {
        *refresh = 0;
    }
}

#endif

/* end of gfx_unix.c */
