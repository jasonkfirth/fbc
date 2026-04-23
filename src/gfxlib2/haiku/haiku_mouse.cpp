/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: haiku_mouse.cpp

    Purpose:

        Mouse handling helpers for the Haiku graphics backend.

    Design:

        • The GUI thread receives native mouse events in FBHaikuView
        • Those events update fb_haiku shared state
        • The runtime thread queries that state through gfxlib APIs

    This file intentionally contains no GUI code.
*/

#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"
#include "haiku_render.h"
#include "haiku_debug.h"

#include <InterfaceDefs.h>

/* ------------------------------------------------------------------------- */
/* Mouse query                                                               */
/* ------------------------------------------------------------------------- */

int fb_hHaikuGetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
    if (x)
        *x = fb_haiku.mouse_x;

    if (y)
        *y = fb_haiku.mouse_y;

    if (z)
        *z = fb_haiku.mouse_z;

    if (buttons)
        *buttons = fb_haiku.mouse_buttons;

    if (clip)
        *clip = fb_haiku.mouse_clip;

    return 0;
}

/* ------------------------------------------------------------------------- */
/* Mouse positioning                                                         */
/* ------------------------------------------------------------------------- */

void fb_hHaikuSetMouse(int x, int y, int cursor, int clip)
{
    /*
        Runtime wants to reposition the mouse cursor.

        Current implementation updates backend state only.
        A future improvement could move the real cursor using
        Haiku APIs if desired.
    */

    fb_haiku.mouse_x = x;
    fb_haiku.mouse_y = y;

    fb_haiku.mouse_clip = clip;

    /*
        Cursor visibility not yet implemented.
    */

    (void)cursor;
}

/* ------------------------------------------------------------------------- */
/* Mouse button update                                                       */
/* ------------------------------------------------------------------------- */

void fb_hHaikuMouseUpdateButtons(int buttons)
{
    fb_haiku.mouse_buttons = buttons;
}

/* ------------------------------------------------------------------------- */
/* Internal helper used by event bridge                                      */
/* ------------------------------------------------------------------------- */

void fb_hHaikuMouseUpdatePosition(int wx, int wy)
{
    int fx;
    int fy;

    fb_hHaikuWindowToFramebuffer(wx, wy, &fx, &fy);

    fb_haiku.mouse_x = fx;
    fb_haiku.mouse_y = fy;
}

#endif

/* end of haiku_mouse.cpp */
