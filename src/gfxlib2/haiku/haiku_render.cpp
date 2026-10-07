
/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: haiku_render.cpp

    Purpose:

        Allocate native presentation bitmaps and render them to the Haiku view.

    Responsibilities:

        Validate bitmap geometry and storage before publishing it, and blit
        frames while the caller holds the native window lock.

    This file contains no window lifecycle or software page allocation.

    Design:

        The runtime thread updates the framebuffer bitmap.

        The GUI thread receives Draw() calls from Haiku and
        blits the bitmap into the window view.
*/

#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"

#include <Bitmap.h>
#include <View.h>
#include <limits.h>
#include <new>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* External platform objects                                                 */
/* ------------------------------------------------------------------------- */

extern BBitmap *g_bmp;

/* ------------------------------------------------------------------------- */
/* Presentation bitmap allocation                                            */
/* ------------------------------------------------------------------------- */

BBitmap *fb_hHaikuCreateBitmap(int width, int height)
{
    BBitmap *bitmap;

    /* BRect uses floats. Reject dimensions whose inclusive endpoint rounds. */
    if (width <= 0 || height <= 0 || width > INT_MAX / 4 ||
        (double)(float)(width - 1) != (double)(width - 1) ||
        (double)(float)(height - 1) != (double)(height - 1))
        return NULL;

    bitmap = new(std::nothrow) BBitmap(BRect(0, 0, width - 1, height - 1),
        B_BITMAP_ACCEPTS_VIEWS, B_RGB32);
    if (!bitmap || !bitmap->IsValid() || !bitmap->Bits() ||
        bitmap->BitsLength() <= 0 || bitmap->BytesPerRow() < width * 4 ||
        bitmap->Bounds().IntegerWidth() + 1 != width ||
        bitmap->Bounds().IntegerHeight() + 1 != height ||
        (size_t)height > ((size_t)bitmap->BitsLength() /
            (size_t)bitmap->BytesPerRow()))
    {
        delete bitmap;
        return NULL;
    }

    memset(bitmap->Bits(), 0, bitmap->BitsLength());
    return bitmap;
}

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
