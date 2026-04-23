/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: haiku_platform.cpp

    Purpose:

        Platform-level helpers and shared state management.

    Responsibilities:

        • initialize/reset backend state
        • query desktop dimensions
        • provide coordinate conversion helpers

    Mouse logic lives in haiku_mouse.cpp
    Mode enumeration lives in haiku_modes.cpp
*/

#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"
#include "haiku_debug.h"

#include <Screen.h>

#include <string.h>

/* ------------------------------------------------------------------------- */
/* Global platform objects                                                   */
/* ------------------------------------------------------------------------- */

BBitmap *g_bmp = NULL;
BWindow *g_win = NULL;
BView   *g_view = NULL;

/* ------------------------------------------------------------------------- */
/* Reset backend state                                                       */
/* ------------------------------------------------------------------------- */

void fb_hHaikuResetState(void)
{
    memset(&fb_haiku, 0, sizeof(fb_haiku));

    fb_haiku.mouse_visible = 1;

    g_bmp = NULL;
    g_win = NULL;
    g_view = NULL;
}

/* ------------------------------------------------------------------------- */
/* Query desktop size                                                        */
/* ------------------------------------------------------------------------- */

void fb_hHaikuQueryDesktop(void)
{
    BScreen screen;

    BRect frame = screen.Frame();

    fb_haiku.desktop_width  = (int)frame.Width() + 1;
    fb_haiku.desktop_height = (int)frame.Height() + 1;

    HAIKU_DEBUG(
        "Desktop size: %dx%d",
        fb_haiku.desktop_width,
        fb_haiku.desktop_height
    );
}

/* ------------------------------------------------------------------------- */
/* Window → framebuffer coordinate conversion                                */
/* ------------------------------------------------------------------------- */

void fb_hHaikuWindowToFramebuffer(int wx, int wy, int *fx, int *fy)
{
    if (!fx || !fy)
        return;

    *fx = wx;
    *fy = wy;

    if (*fx < 0) *fx = 0;
    if (*fy < 0) *fy = 0;

    if (*fx >= fb_haiku.width)
        *fx = fb_haiku.width - 1;

    if (*fy >= fb_haiku.height)
        *fy = fb_haiku.height - 1;
}

#endif

/* end of haiku_platform.cpp */
