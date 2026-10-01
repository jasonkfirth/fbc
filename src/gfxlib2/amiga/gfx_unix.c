/*
    FreeBASIC gfxlib2 support for AmigaOS
    ----------------------------------

    File: gfx_unix.c

    Purpose:

        Register the native AmigaOS driver at the Unix compatibility boundary.

    Responsibilities:

        - replace shared X11 driver registration for AMIGA
        - provide SCREENINFO and native handle queries
        - retain the null fallback after the native driver

    This file intentionally does NOT contain:

        - display lifecycle operations
        - input translation
        - X11 assumptions
*/

#include "fb_gfx_amiga.h"

extern const GFXDRIVER fb_gfxDriverAmiga;

const GFXDRIVER *__fb_gfx_drivers_list[] =
{
    &fb_gfxDriverAmiga,
    &__fb_gfxDriverNull,
    NULL
};

void fb_hScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth,
    ssize_t *refresh)
{
    fb_amigaGfxReadScreenInfo(width, height, depth, refresh);
}

ssize_t fb_hGetWindowHandle(void)
{
    return (ssize_t)(uintptr_t)fb_amiga_gfx.window;
}

ssize_t fb_hGetDisplayHandle(void)
{
    return (ssize_t)(uintptr_t)fb_amiga_gfx.screen;
}

/* end of gfx_unix.c */
