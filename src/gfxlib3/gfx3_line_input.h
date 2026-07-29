/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_line_input.h

    Purpose:

        Declare the graphical input hooks installed for an active gfxlib3
        screen mode.

    Responsibilities:

        - install INPUT$ and narrow and wide LINE INPUT runtime hooks

    This file intentionally does NOT contain:

        - native keyboard event collection
        - graphical console drawing
        - mode creation or destruction
*/

#ifndef __FB_GFX3_LINE_INPUT_H__
#define __FB_GFX3_LINE_INPUT_H__

#include "fb_gfx3.h"

void fb_gfx3_line_input_install_hooks_locked(void);

#endif

/* end of gfx3_line_input.h */
