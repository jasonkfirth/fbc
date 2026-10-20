/*
    FreeBASIC gfxlib2 support for RISC OS
    -------------------------------------

    File: gfx_unix.c

    Purpose:

        Register the RISC OS gfxlib2 driver at the Unix platform boundary.

    Responsibilities:

        - expose the native driver before the shared null fallback
        - implement SCREENINFO using RISC OS VDU state
        - replace the unrelated shared Unix registration module

    This file intentionally does NOT contain:

        - display lifecycle code
        - screen-memory conversion
        - input polling
*/

#include "fb_gfx_riscos.h"

extern const GFXDRIVER fb_gfxDriverRiscos;

const GFXDRIVER *__fb_gfx_drivers_list[] =
{
    &fb_gfxDriverRiscos,
    &__fb_gfxDriverNull,
    NULL
};

void fb_hScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth,
    ssize_t *refresh)
{
    fb_riscosGfxReadScreenInfo(width, height, depth, refresh);
}

ssize_t fb_hGetWindowHandle(void)
{
    return fb_riscosGfxWindowHandle();
}

ssize_t fb_hGetDisplayHandle(void)
{
    /* RISC OS has no desktop display object equivalent to X11 Display. */
    return 0;
}

/* end of gfx_unix.c */
