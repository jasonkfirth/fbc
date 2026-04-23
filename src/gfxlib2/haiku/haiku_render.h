/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: haiku_render.h

    Purpose:

        Declare rendering and coordinate-translation helpers used by
        the Haiku graphics backend.

    Design notes:

        The refactored backend separates responsibilities as follows:

            • GUI thread
                owns BApplication / BWindow / BView
                performs native Haiku drawing callbacks

            • runtime thread
                owns gfxlib rendering flow
                updates framebuffer contents
                requests presentation

        This header therefore exposes only the small shared rendering
        bridge required by both sides.

    This interface intentionally does NOT define ownership of any GUI
    objects. Ownership lives in the backend state and GUI thread logic.
*/

#ifndef FB_GFX_HAIKU_RENDER_H
#define FB_GFX_HAIKU_RENDER_H

#ifndef DISABLE_HAIKU

#ifdef __cplusplus
#include <View.h>
#include <Rect.h>
#endif

/* ------------------------------------------------------------------------- */
/* Coordinate helpers                                                        */
/* ------------------------------------------------------------------------- */

/*
    Convert a point from window coordinates to framebuffer coordinates.

    The current first-pass threaded backend uses a 1:1 mapping, but these
    helpers remain explicit so scaling or letterboxing can be reintroduced
    later without rewriting input code.
*/

void fb_hHaikuWindowToFramebuffer(int x, int y, int *outx, int *outy);

/*
    Convert a point from framebuffer coordinates to window coordinates.
*/

void fb_hHaikuFramebufferToWindow(int x, int y, int *outx, int *outy);

/* ------------------------------------------------------------------------- */
/* Renderer / platform reset                                                 */
/* ------------------------------------------------------------------------- */

/*
    Reset renderer/platform-side shared state.
*/

void fb_hHaikuResetState(void);

#ifdef __cplusplus

/* ------------------------------------------------------------------------- */
/* Native draw bridge                                                        */
/* ------------------------------------------------------------------------- */

/*
    Called by the Haiku view when the OS requests a redraw.

    The GUI thread invokes this from FBHaikuView::Draw().
*/

void fb_hHaikuDraw(BView *view, BRect update);

#endif

#endif
#endif

/* end of haiku_render.h */
