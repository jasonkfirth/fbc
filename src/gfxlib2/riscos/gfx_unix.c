/*
    FreeBASIC gfxlib2 support for RISC OS
    -------------------------------------

    File: gfx_unix.c

    Purpose:

        Define the RISC OS gfxlib2 platform boundary for the initial port.

    Responsibilities:

        - expose an empty native graphics-driver list
        - return defined zero values from SCREENINFO
        - keep gfxlib2 linkable without selecting an unrelated Unix backend

    This file intentionally does NOT contain:

        - Wimp window management
        - framebuffer conversion or redraw handling
        - keyboard or mouse event processing

    Driver behavior:

        A list containing only NULL tells shared gfxlib2 code that no graphics
        mode can be opened.  This is preferable to registering a partial driver
        that could leave global graphics state half-initialized.
*/

#include "../fb_gfx.h"

const GFXDRIVER *__fb_gfx_drivers_list[] = {
	NULL
};

void fb_hScreenInfo( ssize_t *width, ssize_t *height, ssize_t *depth,
	ssize_t *refresh )
{
	if( width != NULL )
		*width = 0;
	if( height != NULL )
		*height = 0;
	if( depth != NULL )
		*depth = 0;
	if( refresh != NULL )
		*refresh = 0;
}

/* end of gfx_unix.c */
