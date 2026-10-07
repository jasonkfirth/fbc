/*
    FreeBASIC graphics runtime tests
    File: haiku-mode-client.cpp
    Purpose: Check native geometry and application routing after mode changes.
    Responsibilities: Inspect window flags, desktop bounds and public scripting.
    This file contains no framebuffer allocation or application input injection.
*/
#include <Application.h>
#include <Message.h>
#include <Messenger.h>
#include <Screen.h>
#include <Window.h>
#include <stdint.h>

extern "C" int fb_test_haiku_mode(int64_t handle, int fullscreen,
    int width, int height)
{
    BWindow *window = reinterpret_cast<BWindow *>(static_cast<intptr_t>(handle));
    if (!window || !be_app)
        return 1;

    /* A live window alone is insufficient: the recreated BApplication must
       still dispatch requests, as native tools and desktop integration do. */
    BMessenger app(be_app), native_window;
    BMessage request(B_GET_PROPERTY), reply;
    request.AddSpecifier("Window", (int32)0);
    if (app.SendMessage(&request, &reply, 2000000, 2000000) != B_OK ||
        reply.FindMessenger("result", &native_window) != B_OK)
        return 2;

    if (!window->Lock())
        return 3;
    BScreen screen;
    BRect desktop = screen.Frame();
    BRect frame = window->Frame();
    int result = 0;
    if (fullscreen)
    {
        if (window->Look() != B_NO_BORDER_WINDOW_LOOK || frame != desktop)
            result = 4;
        if (width != desktop.IntegerWidth() + 1 ||
            height != desktop.IntegerHeight() + 1)
            result = 5;
    }
    else if ((window->Flags() & B_NOT_RESIZABLE) ||
        window->Look() == B_NO_BORDER_WINDOW_LOOK ||
        frame.IntegerWidth() + 1 != width ||
        frame.IntegerHeight() + 1 != height)
        result = 6;
    window->Unlock();
    return result;
}

/* end of haiku-mode-client.cpp */
