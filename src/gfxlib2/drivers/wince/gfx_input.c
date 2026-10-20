/*
    FreeBASIC gfxlib2 support for Windows CE
    ----------------------------------------

    File: gfx_input.c

    Purpose:

        Translate Windows CE window messages into FreeBASIC graphics input.

    Responsibilities:

        - map virtual keys to FreeBASIC scancodes and key events
        - track mouse position, buttons, wheel, focus, and cursor state
        - pump the owning thread's non-blocking message queue
        - repaint invalidated client regions through the display service

    This file intentionally does NOT contain:

        - framebuffer conversion
        - window-class registration
        - desktop-only raw input, touch, or DirectInput support
*/

#include "fb_gfx_wince.h"

#include <string.h>

static int wince_virtual_to_scancode(unsigned int virtual_key)
{
    int index;

    for (index = 0; __fb_keytable[index][0] != 0; ++index)
    {
        if (__fb_keytable[index][1] == virtual_key ||
            __fb_keytable[index][2] == virtual_key)
        {
            return __fb_keytable[index][0];
        }
    }

    return 0;
}

static void wince_post_key(unsigned int message, WPARAM virtual_key,
    LPARAM key_data)
{
    EVENT event;
    int pressed;
    int repeated;
    int scancode;
    int extended;

    pressed = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);
    repeated = pressed && ((key_data & 0x40000000L) != 0);
    scancode = wince_virtual_to_scancode((unsigned int)virtual_key);

    if (scancode > 0 && scancode < 128)
        __fb_gfx->key[scancode] = pressed ? TRUE : FALSE;

    memset(&event, 0, sizeof(event));
    if (!pressed)
        event.type = EVENT_KEY_RELEASE;
    else if (repeated)
        event.type = EVENT_KEY_REPEAT;
    else
        event.type = EVENT_KEY_PRESS;
    event.scancode = scancode;
    fb_hPostEvent(&event);

    if (!pressed || repeated || scancode == 0)
        return;

    extended = fb_hScancodeToExtendedKey(scancode);
    if (extended != 0)
        fb_hPostKey(extended);
}

static void wince_client_to_framebuffer(int client_x, int client_y,
    int *framebuffer_x, int *framebuffer_y)
{
    RECT client;
    int client_width;
    int client_height;

    if (!GetClientRect(fb_wince_gfx.window, &client))
    {
        *framebuffer_x = fb_wince_gfx.mouse_x;
        *framebuffer_y = fb_wince_gfx.mouse_y;
        return;
    }

    client_width = client.right - client.left;
    client_height = client.bottom - client.top;
    if (client_width <= 0 || client_height <= 0)
        return;

    *framebuffer_x = (int)(((long long)client_x *
        fb_wince_gfx.width) / client_width);
    *framebuffer_y = (int)(((long long)client_y *
        fb_wince_gfx.height) / client_height);
    *framebuffer_x = MID(0, *framebuffer_x, fb_wince_gfx.width - 1);
    *framebuffer_y = MID(0, *framebuffer_y, fb_wince_gfx.height - 1);
}

static void wince_post_mouse_move(LPARAM position)
{
    EVENT event;
    int client_x;
    int client_y;
    int next_x;
    int next_y;

    client_x = (short)(position & 0xFFFF);
    client_y = (short)((position >> 16) & 0xFFFF);
    next_x = fb_wince_gfx.mouse_x;
    next_y = fb_wince_gfx.mouse_y;
    wince_client_to_framebuffer(client_x, client_y, &next_x, &next_y);

    if (next_x == fb_wince_gfx.mouse_x && next_y == fb_wince_gfx.mouse_y)
        return;

    memset(&event, 0, sizeof(event));
    event.type = EVENT_MOUSE_MOVE;
    event.x = next_x;
    event.y = next_y;
    event.dx = next_x - fb_wince_gfx.mouse_x;
    event.dy = next_y - fb_wince_gfx.mouse_y;
    fb_wince_gfx.mouse_x = next_x;
    fb_wince_gfx.mouse_y = next_y;
    fb_hPostEvent(&event);
}

static void wince_post_mouse_button(unsigned int message, int button)
{
    EVENT event;
    int pressed;
    int double_click;

    pressed = (message == WM_LBUTTONDOWN ||
        message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN ||
        message == WM_LBUTTONDBLCLK || message == WM_RBUTTONDBLCLK ||
        message == WM_MBUTTONDBLCLK);
    double_click = (message == WM_LBUTTONDBLCLK ||
        message == WM_RBUTTONDBLCLK || message == WM_MBUTTONDBLCLK);

    if (pressed)
    {
        fb_wince_gfx.mouse_buttons |= button;
        SetCapture(fb_wince_gfx.window);
    }
    else
    {
        fb_wince_gfx.mouse_buttons &= ~button;
        if (fb_wince_gfx.mouse_buttons == 0)
            ReleaseCapture();
    }

    memset(&event, 0, sizeof(event));
    if (double_click)
        event.type = EVENT_MOUSE_DOUBLE_CLICK;
    else
        event.type = pressed ? EVENT_MOUSE_BUTTON_PRESS :
            EVENT_MOUSE_BUTTON_RELEASE;
    event.button = button;
    fb_hPostEvent(&event);
}

LRESULT CALLBACK fb_winceGfxWindowProc(HWND window, UINT message,
    WPARAM wparam, LPARAM lparam)
{
    EVENT event;

    if (__fb_gfx == NULL)
        return DefWindowProcW(window, message, wparam, lparam);

    switch (message)
    {
        case WM_ACTIVATE:
            fb_wince_gfx.active = (LOWORD(wparam) != WA_INACTIVE);
            if (!fb_wince_gfx.active)
            {
                memset(__fb_gfx->key, FALSE, 128);
                fb_wince_gfx.mouse_buttons = 0;
            }
            memset(&event, 0, sizeof(event));
            event.type = fb_wince_gfx.active
                ? EVENT_WINDOW_GOT_FOCUS : EVENT_WINDOW_LOST_FOCUS;
            fb_hPostEvent(&event);
            return 0;

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            wince_post_key(message, wparam, lparam);
            return 0;

        case WM_CHAR:
            if (wparam > 0 && wparam < 256)
                fb_hPostKey((int)wparam);
            return 0;

        case WM_MOUSEMOVE:
            wince_post_mouse_move(lparam);
            return 0;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
            wince_post_mouse_button(message, BUTTON_LEFT);
            return 0;

        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
            wince_post_mouse_button(message, BUTTON_RIGHT);
            return 0;

        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:
            wince_post_mouse_button(message, BUTTON_MIDDLE);
            return 0;

        case WM_MOUSEWHEEL:
            fb_wince_gfx.mouse_z +=
                (GET_WHEEL_DELTA_WPARAM(wparam) >= 0) ? 1 : -1;
            memset(&event, 0, sizeof(event));
            event.type = EVENT_MOUSE_WHEEL;
            event.z = fb_wince_gfx.mouse_z;
            fb_hPostEvent(&event);
            return 0;

        case WM_CLOSE:
            memset(&event, 0, sizeof(event));
            event.type = EVENT_WINDOW_CLOSE;
            fb_hPostEvent(&event);
            fb_hPostKey(KEY_QUIT);
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device;

            device = BeginPaint(window, &paint);
            if (device != NULL)
                fb_winceGfxPresentToDevice(device);
            EndPaint(window, &paint);
            return 0;
        }
    }

    return DefWindowProcW(window, message, wparam, lparam);
}

void fb_winceGfxInputInit(void)
{
    fb_wince_gfx.mouse_x = 0;
    fb_wince_gfx.mouse_y = 0;
    fb_wince_gfx.mouse_z = 0;
    fb_wince_gfx.mouse_buttons = 0;
    fb_wince_gfx.mouse_clip = FALSE;
}

void fb_winceGfxInputExit(void)
{
    if (!fb_wince_gfx.cursor_visible)
        ShowCursor(TRUE);
    fb_wince_gfx.cursor_visible = TRUE;
}

void fb_winceGfxPollEvents(void)
{
    MSG message;

    while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
        {
            EVENT event;

            memset(&event, 0, sizeof(event));
            event.type = EVENT_WINDOW_CLOSE;
            fb_hPostEvent(&event);
            fb_hPostKey(KEY_QUIT);
            continue;
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

int fb_winceGfxGetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
    if (fb_wince_gfx.window == NULL)
        return -1;

    fb_winceGfxPollEvents();
    *x = fb_wince_gfx.mouse_x;
    *y = fb_wince_gfx.mouse_y;
    *z = fb_wince_gfx.mouse_z;
    *buttons = fb_wince_gfx.mouse_buttons;
    *clip = fb_wince_gfx.mouse_clip;
    return 0;
}

void fb_winceGfxSetMouse(int x, int y, int cursor, int clip)
{
    POINT point;
    RECT client;

    if (fb_wince_gfx.window == NULL)
        return;

    if (x >= 0)
        fb_wince_gfx.mouse_x = MID(0, x, fb_wince_gfx.width - 1);
    if (y >= 0)
        fb_wince_gfx.mouse_y = MID(0, y, fb_wince_gfx.height - 1);

    if ((x >= 0 || y >= 0) &&
        GetClientRect(fb_wince_gfx.window, &client))
    {
        point.x = (LONG)(((long long)fb_wince_gfx.mouse_x *
            (client.right - client.left)) / fb_wince_gfx.width);
        point.y = (LONG)(((long long)fb_wince_gfx.mouse_y *
            (client.bottom - client.top)) / fb_wince_gfx.height);
        if (ClientToScreen(fb_wince_gfx.window, &point))
            SetCursorPos(point.x, point.y);
    }

    if (cursor >= 0 && !!cursor != fb_wince_gfx.cursor_visible)
    {
        ShowCursor(cursor ? TRUE : FALSE);
        fb_wince_gfx.cursor_visible = !!cursor;
    }

    /* Windows CE has no portable ClipCursor contract.  Preserve the logical
       state so GETMOUSE remains deterministic for callers. */
    if (clip >= 0)
        fb_wince_gfx.mouse_clip = !!clip;
}

/* end of gfx_input.c */
