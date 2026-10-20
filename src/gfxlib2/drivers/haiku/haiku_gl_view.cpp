/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: haiku_gl_view.cpp

    Purpose:

        Provide the BGLView used by the native OpenGL driver.

    Responsibilities:

        - own the Haiku OpenGL drawable
        - forward keyboard and mouse input to the shared event bridge
        - translate resized-view mouse positions to gfxlib coordinates

    This file intentionally does NOT contain context setup, buffer swapping,
    or software framebuffer rendering.
*/

#if !defined(DISABLE_HAIKU) && !defined(DISABLE_OPENGL)

#include "fb_gfx_haiku.h"
#include "haiku_window.h"

#include <Message.h>
#include <Window.h>

static void haiku_gl_map_point(BGLView *view, BPoint where, int *x, int *y)
{
    BRect bounds;
    int view_width;
    int view_height;

    if (!view || !x || !y || fb_haiku.width <= 0 || fb_haiku.height <= 0)
        return;

    bounds = view->Bounds();
    view_width = bounds.IntegerWidth() + 1;
    view_height = bounds.IntegerHeight() + 1;

    if (view_width <= 0 || view_height <= 0)
        return;

    if (where.x < bounds.left)
        where.x = bounds.left;
    if (where.y < bounds.top)
        where.y = bounds.top;
    if (where.x > bounds.right)
        where.x = bounds.right;
    if (where.y > bounds.bottom)
        where.y = bounds.bottom;

    *x = (int)(((where.x - bounds.left) * fb_haiku.width) / view_width);
    *y = (int)(((where.y - bounds.top) * fb_haiku.height) / view_height);

    if (*x >= fb_haiku.width)
        *x = fb_haiku.width - 1;
    if (*y >= fb_haiku.height)
        *y = fb_haiku.height - 1;
}

FBHaikuGLView::FBHaikuGLView(BRect frame, uint32 options)
    : BGLView(frame, "fb_gl_view", B_FOLLOW_ALL,
              B_WILL_DRAW | B_FRAME_EVENTS, options)
{
    SetViewColor(0, 0, 0);
    SetFlags(Flags() | B_NAVIGABLE);
    SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}

void FBHaikuGLView::AttachedToWindow()
{
    BGLView::AttachedToWindow();
    MakeFocus(true);
    if (Window())
        Window()->Activate(true);
}

void FBHaikuGLView::KeyDown(const char *bytes, int32)
{
    int32 key = 0;
    BMessage *message = Window() ? Window()->CurrentMessage() : NULL;

    if (message)
        message->FindInt32("key", &key);

    fb_hHaikuHandleKeyDown(this, bytes, key);
}

void FBHaikuGLView::KeyUp(const char *bytes, int32)
{
    int32 key = 0;
    BMessage *message = Window() ? Window()->CurrentMessage() : NULL;

    if (message)
        message->FindInt32("key", &key);

    fb_hHaikuHandleKeyUp(this, bytes, key);
}

void FBHaikuGLView::MouseMoved(BPoint where, uint32, const BMessage *)
{
    int x = (int)where.x;
    int y = (int)where.y;

    haiku_gl_map_point(this, where, &x, &y);
    fb_hHaikuHandleMouseMoved(this, x, y);
}

void FBHaikuGLView::MouseDown(BPoint where)
{
    uint32 buttons = 0;
    int x;
    int y;

    GetMouse(&where, &buttons);
    x = (int)where.x;
    y = (int)where.y;
    haiku_gl_map_point(this, where, &x, &y);
    fb_hHaikuHandleMouseDown(this, x, y, (int)(buttons & 7));
}

void FBHaikuGLView::MouseUp(BPoint where)
{
    uint32 buttons = 0;
    int x;
    int y;

    GetMouse(&where, &buttons);
    x = (int)where.x;
    y = (int)where.y;
    haiku_gl_map_point(this, where, &x, &y);
    fb_hHaikuHandleMouseUp(this, x, y, (int)(buttons & 7));
}

#endif

/* end of haiku_gl_view.cpp */
