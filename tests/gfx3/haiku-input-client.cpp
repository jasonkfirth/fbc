/*
    FreeBASIC graphics runtime tests
    File: haiku-input-client.cpp
    Purpose: Send native input messages through the real Haiku window dispatcher.
    Responsibilities: Exercise mapped/unmapped keys, pointer masks and wheels.
    This file contains no application input injection or desktop automation.
*/
#include <Window.h>
#include <Message.h>
#include <InterfaceDefs.h>
#include <stdint.h>
#include <math.h>

static const char enter_bytes[] = {10, 0};
static const char control_c[] = {3, 0};
static const char function_bytes[] = {16, 0};

static void key(BWindow *window, uint32 what, int code, const char *bytes)
{
    BMessage message(what);
    message.AddInt32("key", code);
    if (bytes)
        message.AddString("bytes", bytes);
    window->DispatchMessage(&message, window);
}

static void pointer(BWindow *window, uint32 what, int buttons)
{
    BMessage message(what);
    message.AddInt32("fb:x", 40);
    message.AddInt32("fb:y", 50);
    message.AddInt32("fb:buttons", buttons);
    window->MessageReceived(&message);
}

static void wheel(BWindow *window, float x, float y)
{
    BMessage message(B_MOUSE_WHEEL_CHANGED);
    message.AddFloat("be:wheel_delta_x", x);
    message.AddFloat("be:wheel_delta_y", y);
    window->DispatchMessage(&message, window);
}

extern "C" int fb_test_haiku_input(int64_t handle, int stage)
{
    BWindow *window = reinterpret_cast<BWindow *>(static_cast<intptr_t>(handle));
    if (!window || !window->Lock())
        return -1;
    switch (stage)
    {
        case 1:
            key(window, B_KEY_DOWN, 0x3c, "a");
            key(window, B_KEY_DOWN, 0x3c, "a");
            key(window, B_KEY_UP, 0x3c, "a");
            break;
        case 2:
            key(window, B_KEY_DOWN, 0x47, enter_bytes);
            key(window, B_KEY_UP, 0x47, enter_bytes);
            key(window, B_KEY_DOWN, 0x32, "]");
            key(window, B_KEY_UP, 0x32, "]");
            break;
        case 3:
            key(window, B_UNMAPPED_KEY_DOWN, 0x5c, NULL);
            key(window, B_KEY_DOWN, 0x4e, control_c);
            key(window, B_KEY_UP, 0x4e, control_c);
            key(window, B_UNMAPPED_KEY_UP, 0x5c, NULL);
            key(window, B_UNMAPPED_KEY_DOWN, 0x5d, NULL);
            key(window, B_KEY_DOWN, 0x3c, "a");
            key(window, B_KEY_UP, 0x3c, "a");
            key(window, B_UNMAPPED_KEY_UP, 0x5d, NULL);
            break;
        case 4:
            pointer(window, B_MOUSE_MOVED, 0);
            pointer(window, B_MOUSE_DOWN, 1);
            pointer(window, B_MOUSE_DOWN, 3);
            pointer(window, B_MOUSE_UP, 2);
            pointer(window, B_MOUSE_UP, 0);
            break;
        case 5:
            wheel(window, 0, -0.5f);
            wheel(window, 0, -0.5f);
            wheel(window, 1, 0);
            wheel(window, NAN, NAN);
            key(window, B_UNMAPPED_KEY_DOWN, 0x10081, NULL);
            break;
        case 6:
            key(window, B_KEY_DOWN, 0x3c, "A");
            key(window, B_KEY_UP, 0x3c, "a");
            key(window, B_KEY_DOWN, 0x02, function_bytes);
            key(window, B_KEY_UP, 0x02, function_bytes);
            break;
    }
    window->Unlock();
    return 0;
}
/* end of haiku-input-client.cpp */
