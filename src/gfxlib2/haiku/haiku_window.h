
#ifndef FB_HAIKU_WINDOW_H
#define FB_HAIKU_WINDOW_H

#include <Window.h>
#include <View.h>
#include <Rect.h>

/* ------------------------------------------------------------------------- */

class FBHaikuWindow : public BWindow
{
public:

    FBHaikuWindow(BRect frame, const char *title);

    virtual bool QuitRequested();
};

/* ------------------------------------------------------------------------- */

class FBHaikuView : public BView
{
public:

    FBHaikuView(BRect frame);

    virtual void AttachedToWindow();   /* <-- added */

    virtual void Draw(BRect update);
    virtual void MessageReceived(BMessage *msg);

    virtual void KeyDown(const char *bytes, int32 numBytes);
    virtual void KeyUp(const char *bytes, int32 numBytes);

    virtual void MouseMoved(BPoint where, uint32 transit, const BMessage*);
    virtual void MouseDown(BPoint where);
    virtual void MouseUp(BPoint where);

    virtual void FrameResized(float width, float height);

private:

    /* cached scaling destination */
    BRect fDestRect;
};

/* ------------------------------------------------------------------------- */

#endif
