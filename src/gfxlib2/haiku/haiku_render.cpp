
/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: haiku_render.cpp

    Purpose:

        Render the framebuffer bitmap to the Haiku view.

    Design:

        The runtime thread updates the framebuffer bitmap.

        The GUI thread receives Draw() calls from Haiku and
        blits the bitmap into the window view.
*/

#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"

#include <Bitmap.h>
#include <View.h>

/* ------------------------------------------------------------------------- */
/* External platform objects                                                 */
/* ------------------------------------------------------------------------- */

extern BBitmap *g_bmp;

/* ------------------------------------------------------------------------- */
/* Draw handler                                                              */
/* ------------------------------------------------------------------------- */

void fb_hHaikuDraw(BView *view, BRect update)
{
    if (!view)
        return;

    if (!g_bmp)
        return;

    /* Fast copy mode (no blending) */
    view->SetDrawingMode(B_OP_COPY);

    view->DrawBitmap(g_bmp, update, update);
}

#endif

/* end of haiku_render.cpp */
