/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_vga_api.h

    Purpose:

        Declare installation of the indexed-mode VGA compatibility hooks.

    Responsibilities:

        - install emulated VGA palette and status port handlers

    This file intentionally does NOT contain:

        - arbitrary hardware port access
        - palette storage or presentation shaders
        - screen mode lifecycle
*/

#ifndef __FB_GFX3_VGA_API_H__
#define __FB_GFX3_VGA_API_H__

#include "fb_gfx3.h"

void fb_gfx3_vga_install_hooks_locked(void);

#endif

/* end of gfx3_vga_api.h */
