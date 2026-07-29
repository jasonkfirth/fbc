/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_console.h

    Purpose:

        Declare the graphical text-console lifecycle owned by an active
        gfxlib3 mode.

    Responsibilities:

        - allocate page-specific character cells while both runtime locks held
		- install and remove FreeBASIC runtime console hooks
		- keep console resources inside the graphics mode lifetime
		- expose the graphical cursor and print operations used by line input

    This file intentionally does NOT contain:

        - font data, console algorithms, or public hook implementations
		- native keyboard and mouse input
		- platform window declarations
*/

#ifndef __FB_GFX3_CONSOLE_H__
#define __FB_GFX3_CONSOLE_H__

#include "gfx3_compat.h"

int fb_gfx3_console_init_locked(FB_GFX3_MODE *mode);
void fb_gfx3_console_shutdown_locked(FB_GFX3_MODE *mode);
int fb_gfx3_console_page_copy_locked(FB_GFX3_MODE *mode,
	uint32_t source_page, uint32_t destination_page);
int fb_gfx3_console_resize_locked(FB_GFX3_MODE *mode,
	uint32_t foreground, uint32_t background);

int fb_GfxLocate(int row, int column, int cursor);
void fb_GfxGetXY(int *column, int *row);
void fb_GfxGetSize(int *columns, int *rows);
void fb_GfxPrintBufferEx(const void *buffer, size_t length, int mask);
void fb_GfxViewUpdate(void);

#endif

/* end of gfx3_console.h */
