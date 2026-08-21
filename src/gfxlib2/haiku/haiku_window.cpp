#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"
#include "haiku_window.h"
#include "../fb_gfx.h"

#include <Bitmap.h>
#include <View.h>
#include <Window.h>
#include <Message.h>
#include <Alert.h>
#include <OS.h>
#include <stdint.h>
#include <stdlib.h>

extern BBitmap *g_bmp;
extern BWindow *g_win;
extern BView   *g_view;

#define FB_HAIKU_REDRAW_MSG 'fbrd'
#define FB_HAIKU_FORCE_EXIT_DELAY_US 500000

static int32 fb_hHaikuForceExit(void*)
{
    snooze(FB_HAIKU_FORCE_EXIT_DELAY_US);
    exit(0);
    return 0;
}

FBHaikuWindow::FBHaikuWindow(BRect frame, const char *title)
    : BWindow(frame, title, B_TITLED_WINDOW, 0)
{
}

void FBHaikuWindow::MessageReceived(BMessage *msg)
{
    int32 key = 0;
    const char *bytes = "";
    int32 x = 0;
    int32 y = 0;
    int32 buttons = 0;

    switch (msg->what)
    {
        case B_KEY_DOWN:
        case B_KEY_UP:
            /*
                Normally Haiku delivers key events directly to the focused
                BView.  Remote smoke tests can only post flattened messages to
                the public window token, so accept that path too and feed the
                event into the same backend handlers used by FBHaikuView.
            */
            msg->FindInt32("key", &key);
            msg->FindString("bytes", &bytes);

            if (msg->what == B_KEY_DOWN)
                fb_hHaikuHandleKeyDown(g_view, bytes, key);
            else
                fb_hHaikuHandleKeyUp(g_view, bytes, key);
            return;

        case B_MOUSE_MOVED:
            if (msg->FindInt32("fb:x", &x) == B_OK &&
                msg->FindInt32("fb:y", &y) == B_OK)
            {
                fb_hHaikuHandleMouseMoved(g_view, x, y);
                return;
            }
            break;

        case B_MOUSE_DOWN:
            if (msg->FindInt32("fb:x", &x) == B_OK &&
                msg->FindInt32("fb:y", &y) == B_OK &&
                msg->FindInt32("fb:buttons", &buttons) == B_OK)
            {
                fb_hHaikuHandleMouseDown(g_view, x, y, buttons);
                return;
            }
            break;

        case B_MOUSE_UP:
            if (msg->FindInt32("fb:x", &x) == B_OK &&
                msg->FindInt32("fb:y", &y) == B_OK &&
                msg->FindInt32("fb:buttons", &buttons) == B_OK)
            {
                fb_hHaikuHandleMouseUp(g_view, x, y, buttons);
                return;
            }
            break;
    }

    BWindow::MessageReceived(msg);
}

bool FBHaikuWindow::QuitRequested()
{
    if (fb_haiku.quitting)
        return true;

    BAlert *alert = new BAlert(
        "quit",
        "Are you sure you want to quit?",
        "Cancel",
        "Quit",
        NULL,
        B_WIDTH_AS_USUAL,
        B_WARNING_ALERT
    );

    int32 result = alert->Go();

    if (result == 1)
    {
        thread_id killer;

        fb_haiku.quitting = 1;
        fb_hHaikuPostQuitEvent();

        if (__fb_gfx)
            __fb_gfx->key[KEY_QUIT] = TRUE;

        killer = spawn_thread(
            fb_hHaikuForceExit,
            "fb_haiku_force_exit",
            B_NORMAL_PRIORITY,
            NULL
        );

        if (killer >= B_OK)
            resume_thread(killer);

        return true;
    }

    return false;
}

FBHaikuView::FBHaikuView(BRect frame)
    : BView(frame, "fb_view", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
{
    SetViewColor(0,0,0);
    SetFlags(Flags() | B_NAVIGABLE);
    SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);

    fDestRect = Bounds();
}

void FBHaikuView::AttachedToWindow()
{
    BView::AttachedToWindow();
    MakeFocus(true);
    if (Window())
        Window()->Activate(true);
}

void FBHaikuView::Draw(BRect)
{
    if (!g_bmp)
        return;

    SetDrawingMode(B_OP_COPY);
    DrawBitmap(g_bmp, g_bmp->Bounds(), fDestRect);
}

void FBHaikuView::MessageReceived(BMessage *msg)
{
    switch (msg->what)
    {
        case FB_HAIKU_REDRAW_MSG:
            Invalidate();
            break;
        default:
            BView::MessageReceived(msg);
            break;
    }
}

void FBHaikuView::KeyDown(const char *bytes, int32)
{
    int32 key = 0;
    BMessage *msg = Window() ? Window()->CurrentMessage() : NULL;
    if (msg)
        msg->FindInt32("key", &key);

    fb_hHaikuHandleKeyDown(this, bytes, key);
}

void FBHaikuView::KeyUp(const char *bytes, int32)
{
    int32 key = 0;
    BMessage *msg = Window() ? Window()->CurrentMessage() : NULL;
    if (msg)
        msg->FindInt32("key", &key);

    fb_hHaikuHandleKeyUp(this, bytes, key);
}

void FBHaikuView::MouseMoved(BPoint where, uint32, const BMessage*)
{
    int fb_w, fb_h, draw_w, draw_h, fx, fy;
    float left, top, rel_x, rel_y;

    if (!g_bmp)
    {
        fb_hHaikuHandleMouseMoved(this, (int)where.x, (int)where.y);
        return;
    }

    fb_w = g_bmp->Bounds().IntegerWidth() + 1;
    fb_h = g_bmp->Bounds().IntegerHeight() + 1;
    draw_w = (int)fDestRect.Width() + 1;
    draw_h = (int)fDestRect.Height() + 1;

    left = fDestRect.left;
    top  = fDestRect.top;
    rel_x = where.x - left;
    rel_y = where.y - top;

    if (rel_x < 0) rel_x = 0;
    if (rel_y < 0) rel_y = 0;
    if (rel_x >= draw_w) rel_x = (float)(draw_w - 1);
    if (rel_y >= draw_h) rel_y = (float)(draw_h - 1);

    fx = (int)((rel_x * fb_w) / draw_w);
    fy = (int)((rel_y * fb_h) / draw_h);

    if (fx < 0) fx = 0;
    if (fy < 0) fy = 0;
    if (fx >= fb_w) fx = fb_w - 1;
    if (fy >= fb_h) fy = fb_h - 1;

    fb_hHaikuHandleMouseMoved(this, fx, fy);
}

void FBHaikuView::MouseDown(BPoint where)
{
    uint32 buttons = 0;
    int fb_w, fb_h, draw_w, draw_h, fx, fy;
    float left, top, rel_x, rel_y;

    GetMouse(&where, &buttons);

    if (!g_bmp)
    {
        fb_hHaikuHandleMouseDown(this, (int)where.x, (int)where.y, (int)(buttons & 7));
        return;
    }

    fb_w = g_bmp->Bounds().IntegerWidth() + 1;
    fb_h = g_bmp->Bounds().IntegerHeight() + 1;
    draw_w = (int)fDestRect.Width() + 1;
    draw_h = (int)fDestRect.Height() + 1;

    left = fDestRect.left;
    top  = fDestRect.top;
    rel_x = where.x - left;
    rel_y = where.y - top;

    if (rel_x < 0) rel_x = 0;
    if (rel_y < 0) rel_y = 0;
    if (rel_x >= draw_w) rel_x = (float)(draw_w - 1);
    if (rel_y >= draw_h) rel_y = (float)(draw_h - 1);

    fx = (int)((rel_x * fb_w) / draw_w);
    fy = (int)((rel_y * fb_h) / draw_h);

    if (fx < 0) fx = 0;
    if (fy < 0) fy = 0;
    if (fx >= fb_w) fx = fb_w - 1;
    if (fy >= fb_h) fy = fb_h - 1;

    fb_hHaikuHandleMouseDown(this, fx, fy, (int)(buttons & 7));
}

void FBHaikuView::MouseUp(BPoint where)
{
    uint32 buttons = 0;
    int fb_w, fb_h, draw_w, draw_h, fx, fy;
    float left, top, rel_x, rel_y;

    GetMouse(&where, &buttons);

    if (!g_bmp)
    {
        fb_hHaikuHandleMouseUp(this, (int)where.x, (int)where.y, (int)(buttons & 7));
        return;
    }

    fb_w = g_bmp->Bounds().IntegerWidth() + 1;
    fb_h = g_bmp->Bounds().IntegerHeight() + 1;
    draw_w = (int)fDestRect.Width() + 1;
    draw_h = (int)fDestRect.Height() + 1;

    left = fDestRect.left;
    top  = fDestRect.top;
    rel_x = where.x - left;
    rel_y = where.y - top;

    if (rel_x < 0) rel_x = 0;
    if (rel_y < 0) rel_y = 0;
    if (rel_x >= draw_w) rel_x = (float)(draw_w - 1);
    if (rel_y >= draw_h) rel_y = (float)(draw_h - 1);

    fx = (int)((rel_x * fb_w) / draw_w);
    fy = (int)((rel_y * fb_h) / draw_h);

    if (fx < 0) fx = 0;
    if (fy < 0) fy = 0;
    if (fx >= fb_w) fx = fb_w - 1;
    if (fy >= fb_h) fy = fb_h - 1;

    fb_hHaikuHandleMouseUp(this, fx, fy, (int)(buttons & 7));
}

void FBHaikuView::FrameResized(float width, float height)
{
    BView::FrameResized(width, height);

    if (!g_bmp)
        return;

    int fb_w = g_bmp->Bounds().IntegerWidth() + 1;
    int fb_h = g_bmp->Bounds().IntegerHeight() + 1;
    int win_w = (int)width + 1;
    int win_h = (int)height + 1;
#ifdef GFXLIB_NEVERSCALE
    int scale = 1;
#else
    int scale_x = win_w / fb_w;
    int scale_y = win_h / fb_h;
    int scale = scale_x < scale_y ? scale_x : scale_y;

    if (scale < 1)
        scale = 1;
#endif

    int draw_w = fb_w * scale;
    int draw_h = fb_h * scale;
    int offset_x = (win_w - draw_w) / 2;
    int offset_y = (win_h - draw_h) / 2;

    fDestRect = BRect(
        offset_x,
        offset_y,
        offset_x + draw_w - 1,
        offset_y + draw_h - 1
    );

    Invalidate();
}

void fb_hHaikuSetWindowTitle(char *title)
{
    if (!g_win)
        return;

    if (g_win->Lock())
    {
        g_win->SetTitle(title ? title : "FreeBASIC");
        g_win->Unlock();
    }
}

int fb_hHaikuSetWindowPos(int x, int y)
{
    if (!g_win)
        return -1;

    if (g_win->Lock())
    {
        g_win->MoveTo(x, y);
        g_win->Unlock();
    }

    return 0;
}

/*
    ScreenControl handle compatibility

    The shared gfx control layer asks every backend for a window handle and a
    display handle.  Haiku has a real native window object, so expose the active
    BWindow pointer through the integer-sized API slot.  Haiku does not expose
    an X11-style display connection to applications, so the display handle is
    intentionally reported as zero.
*/

extern "C" ssize_t fb_hGetWindowHandle(void)
{
    if (!g_win)
        return 0;

    return (ssize_t)(intptr_t)g_win;
}

extern "C" ssize_t fb_hGetDisplayHandle(void)
{
    return 0;
}

#endif
