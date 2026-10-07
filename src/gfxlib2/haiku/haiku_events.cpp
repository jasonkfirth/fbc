/*
    FreeBASIC gfxlib2 Haiku backend
    File: haiku_events.cpp
    Purpose: Bridge native input to ScreenEvent and legacy keyboard polling.
    Responsibilities: Translate keys, pointer transitions, wheels and focus.
    This file contains no native window ownership or framebuffer presentation.
*/

#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"
#include <limits.h>
#include <math.h>

/* ------------------------------------------------------------------------- */
/* Keyboard translation and synchronization                                  */
/* ------------------------------------------------------------------------- */

static int haiku_ascii(const char *bytes, int scancode)
{
    unsigned char first = bytes ? (unsigned char)bytes[0] : 0;

    if (scancode == SC_ENTER)
        return 13; /* Haiku sends LF; BASIC's Enter character is CR. */
    if ((scancode >= SC_F1 && scancode <= SC_F10) ||
        scancode == SC_F11 || scancode == SC_F12 ||
        (scancode >= SC_HOME && scancode <= SC_DELETE))
        return 0;
    if (first < 128)
        return first;
    /* ScreenEvent's historical character contract is one Latin-1 byte.
       Do not expose the first byte of a longer UTF-8 sequence as a character. */
    if ((first == 0xc2 || first == 0xc3) &&
        (((unsigned char)bytes[1] & 0xc0) == 0x80))
        return ((first & 0x1f) << 6) | ((unsigned char)bytes[1] & 0x3f);
    return 0;
}

static int haiku_legacy_key(int scancode, int ascii)
{
    if (ascii)
        return ascii;
    switch (scancode)
    {
        case SC_F1: return KEY_F1;
        case SC_F2: return KEY_F2;
        case SC_F3: return KEY_F3;
        case SC_F4: return KEY_F4;
        case SC_F5: return KEY_F5;
        case SC_F6: return KEY_F6;
        case SC_F7: return KEY_F7;
        case SC_F8: return KEY_F8;
        case SC_F9: return KEY_F9;
        case SC_F10: return KEY_F10;
        case SC_F11: return KEY_F11;
        case SC_F12: return KEY_F12;
        case SC_HOME: return KEY_HOME;
        case SC_UP: return KEY_UP;
        case SC_PAGEUP: return KEY_PAGE_UP;
        case SC_LEFT: return KEY_LEFT;
        case SC_RIGHT: return KEY_RIGHT;
        case SC_END: return KEY_END;
        case SC_DOWN: return KEY_DOWN;
        case SC_PAGEDOWN: return KEY_PAGE_DOWN;
        case SC_INSERT: return KEY_INS;
        case SC_DELETE: return KEY_DEL;
    }
    return 0;
}

/* Caller holds backend_lock. Mirror the bounded runtime keyboard buffer:
   when full, discard the oldest key rather than allocating in a GUI hook. */
static void haiku_queue_key(int key)
{
    if (!key)
        return;
    fb_haiku.pending_keys[fb_haiku.key_tail] = key;
    fb_haiku.key_tail = (fb_haiku.key_tail + 1) % MAX_EVENTS;
    if (fb_haiku.key_tail == fb_haiku.key_head)
        fb_haiku.key_head = (fb_haiku.key_head + 1) % MAX_EVENTS;
}

static void haiku_key(const char *bytes, int32_t keycode, int down)
{
    EVENT event = {};
    int scancode;

    /* Multimedia key codes exceed 255; truncating them can synthesize Escape. */
    if (keycode <= 0 || keycode >= 256)
        return;
    scancode = fb_hHaikuTranslateScancode((unsigned char)keycode);
    event.scancode = scancode;
    event.ascii = haiku_ascii(bytes, scancode);
    if (!scancode && !event.ascii)
        return;

    fb_hHaikuLockState();
    if (down)
    {
        event.type = scancode && fb_haiku.key_state[scancode] ?
            EVENT_KEY_REPEAT : EVENT_KEY_PRESS;
        if (scancode)
        {
            fb_haiku.key_state[scancode] = TRUE;
            fb_haiku.key_ascii[scancode] = event.ascii;
        }
        haiku_queue_key(haiku_legacy_key(scancode, event.ascii));
    }
    else
    {
        event.type = EVENT_KEY_RELEASE;
        if (scancode)
        {
            /* A Shift release can change the key-up bytes. Release the symbol
               that was pressed so remote viewers do not leave it held. */
            if (fb_haiku.key_state[scancode])
                event.ascii = fb_haiku.key_ascii[scancode];
            fb_haiku.key_state[scancode] = FALSE;
            fb_haiku.key_ascii[scancode] = 0;
        }
    }
    fb_hHaikuUnlockState();
    /* Native hooks run with the BWindow locked. Never take DRIVER_LOCK here:
       presentation takes the driver mutex before locking that BWindow.
       ScreenEvent has its own queue lock. Window shutdown joins native hooks
       before freeing the graphics context used by this event queue. */
    if (__fb_gfx)
        fb_hPostEvent(&event);
}

void fb_hHaikuHandleKeyDown(void *, const char *bytes, int32_t keycode)
{
    haiku_key(bytes, keycode, TRUE);
}

void fb_hHaikuHandleKeyUp(void *, const char *bytes, int32_t keycode)
{
    haiku_key(bytes, keycode, FALSE);
}

/* ------------------------------------------------------------------------- */
/* Pointer transitions                                                       */
/* ------------------------------------------------------------------------- */

void fb_hHaikuHandleMouseMoved(void *, int x, int y)
{
    EVENT event = {};
    int scanline;

    fb_hHaikuLockState();
    scanline = fb_haiku.scanline_size > 0 ? fb_haiku.scanline_size : 1;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (fb_haiku.width > 0 && x >= fb_haiku.width) x = fb_haiku.width - 1;
    if (fb_haiku.height > 0 && y >= fb_haiku.height) y = fb_haiku.height - 1;
    event.type = EVENT_MOUSE_MOVE;
    event.x = x;
    event.y = y / scanline;
    event.dx = x - fb_haiku.mouse_x;
    event.dy = event.y - fb_haiku.mouse_y / scanline;
    fb_haiku.mouse_x = x;
    fb_haiku.mouse_y = y;
    fb_hHaikuUnlockState();
    if (__fb_gfx && (event.dx || event.dy))
        fb_hPostEvent(&event);
}

static void haiku_buttons(void *view, int x, int y, int buttons)
{
    EVENT event = {};
    int previous;
    int changed;

    fb_hHaikuHandleMouseMoved(view, x, y);
    buttons &= BUTTON_LEFT | BUTTON_RIGHT | BUTTON_MIDDLE;
    fb_hHaikuLockState();
    previous = fb_haiku.mouse_buttons;
    changed = previous ^ buttons;
    fb_haiku.mouse_buttons = buttons;
    fb_haiku.mouse_latched_buttons |= buttons & ~previous;
    fb_hHaikuUnlockState();
    /* The native message contains the whole button mask. ScreenEvent reports
       one changed button per event, including multi-button releases. */
    for (int button = BUTTON_LEFT; button <= BUTTON_MIDDLE; button *= 2)
    {
        if (!(changed & button))
            continue;
        event.type = buttons & button ? EVENT_MOUSE_BUTTON_PRESS :
            EVENT_MOUSE_BUTTON_RELEASE;
        event.button = button;
        if (__fb_gfx)
            fb_hPostEvent(&event);
    }
}

void fb_hHaikuHandleMouseDown(void *view, int x, int y, int buttons)
{
    haiku_buttons(view, x, y, buttons);
}

void fb_hHaikuHandleMouseUp(void *view, int x, int y, int buttons)
{
    haiku_buttons(view, x, y, buttons);
}

static void haiku_wheel(float delta, int horizontal)
{
    EVENT event = {};
    double *fraction;
    int *position;
    int steps;
    long long next;

    if (!isfinite(delta) || fabs((double)delta) >= INT_MAX)
        return;
    fb_hHaikuLockState();
    fraction = horizontal ? &fb_haiku.wheel_x : &fb_haiku.wheel_y;
    position = horizontal ? &fb_haiku.mouse_w : &fb_haiku.mouse_z;
    *fraction += delta;
    steps = (int)*fraction;
    *fraction -= steps;
    /* Haiku positive vertical deltas scroll down; BASIC's Z increases up. */
    next = (long long)*position + (horizontal ? steps : -steps);
    if (next > INT_MAX) next = INT_MAX;
    if (next < INT_MIN) next = INT_MIN;
    *position = (int)next;
    event.type = horizontal ? EVENT_MOUSE_HWHEEL : EVENT_MOUSE_WHEEL;
    event.z = *position; /* z and w occupy the same EVENT union member. */
    fb_hHaikuUnlockState();
    if (__fb_gfx && steps)
        fb_hPostEvent(&event);
}

void fb_hHaikuHandleMouseWheel(float x, float y)
{
    haiku_wheel(x, TRUE);
    haiku_wheel(y, FALSE);
}

/* ------------------------------------------------------------------------- */
/* Focus and close                                                           */
/* ------------------------------------------------------------------------- */

void fb_hHaikuHandleFocus(int active)
{
    EVENT event = {};
    int x;
    int y;

    if (!active)
    {
        fb_hHaikuLockState();
        for (int scancode = 1; scancode < 128; scancode++)
        {
            if (!fb_haiku.key_state[scancode])
                continue;
            event.type = EVENT_KEY_RELEASE;
            event.scancode = scancode;
            event.ascii = fb_haiku.key_ascii[scancode];
            fb_haiku.key_state[scancode] = FALSE;
            fb_haiku.key_ascii[scancode] = 0;
            if (__fb_gfx)
                fb_hPostEvent(&event);
        }
        x = fb_haiku.mouse_x;
        y = fb_haiku.mouse_y;
        fb_hHaikuUnlockState();
        haiku_buttons(NULL, x, y, 0);
    }
    event.type = active ? EVENT_WINDOW_GOT_FOCUS : EVENT_WINDOW_LOST_FOCUS;
    if (__fb_gfx)
        fb_hPostEvent(&event);
}

void fb_hHaikuPostQuitEvent(void)
{
    EVENT event = {};
    fb_hHaikuLockState();
    fb_haiku.quitting = 1;
    haiku_queue_key(KEY_QUIT);
    fb_hHaikuUnlockState();
    event.type = EVENT_WINDOW_CLOSE;
    if (__fb_gfx)
        fb_hPostEvent(&event);
}

#endif

/* end of haiku_events.cpp */
