/*
    FreeBASIC gfxlib2 support for AROS
    ----------------------------------

    File: gfx_unix.c

    Purpose:

        Register the native AROS driver at the Unix compatibility boundary.

    Responsibilities:

        - replace shared X11 driver registration for AROS
        - provide SCREENINFO and native handle queries
        - retain the null fallback after the native driver

    This file intentionally does NOT contain:

        - display lifecycle operations
        - input translation
        - X11 assumptions
*/

#include "fb_gfx_aros.h"

extern const GFXDRIVER fb_gfxDriverAros;

const GFXDRIVER *__fb_gfx_drivers_list[] =
{
    &fb_gfxDriverAros,
    &__fb_gfxDriverNull,
    NULL
};

void fb_hScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth,
    ssize_t *refresh)
{
    fb_arosGfxReadScreenInfo(width, height, depth, refresh);
}

ssize_t fb_hGetWindowHandle(void)
{
    return (ssize_t)(IPTR)fb_aros_gfx.window;
}

ssize_t fb_hGetDisplayHandle(void)
{
    return (ssize_t)(IPTR)fb_aros_gfx.screen;
}

/* end of gfx_unix.c */
