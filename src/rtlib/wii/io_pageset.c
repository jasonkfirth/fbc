/*
    FreeBASIC runtime Wii console page selection
    --------------------------------------------

    File: io_pageset.c

    Purpose:

        Provide the console SCREEN page hook for Wii targets.

    Responsibilities:

        - keep console page APIs linkable
        - report that the fallback libogc console has one page

    This file intentionally does NOT contain:

        - gfxlib page flipping
        - framebuffer copies
*/

#include "../fb.h"

int fb_ConsolePageSet(int active, int visible)
{
	(void)active;
	(void)visible;
	return -1;
}

/* end of io_pageset.c */
