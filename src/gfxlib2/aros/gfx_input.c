/*
    FreeBASIC gfxlib2 support for AROS
    ----------------------------------

    File: gfx_input.c

    Purpose:

        Translate Intuition input messages into FreeBASIC graphics input.

    Responsibilities:

        - map AROS raw keys to FreeBASIC scancodes
        - use keymap.library for layout-aware text input
        - track mouse position, buttons, wheel, and focus
        - preserve the native Intuition cursor above graphics updates

    This file intentionally does NOT contain:

        - framebuffer conversion
        - display-window construction
        - global input.device hooks
*/

#include "fb_gfx_aros.h"

#include <devices/inputevent.h>
#include <devices/rawkeycodes.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/keymap.h>

#include <string.h>

static int aros_raw_to_scancode(unsigned int raw)
{
    static const unsigned char map[128] =
    {
        [RAWKEY_ESCAPE] = SC_ESCAPE,
        [RAWKEY_1] = SC_1, [RAWKEY_2] = SC_2, [RAWKEY_3] = SC_3,
        [RAWKEY_4] = SC_4, [RAWKEY_5] = SC_5, [RAWKEY_6] = SC_6,
        [RAWKEY_7] = SC_7, [RAWKEY_8] = SC_8, [RAWKEY_9] = SC_9,
        [RAWKEY_0] = SC_0, [RAWKEY_MINUS] = SC_MINUS,
        [RAWKEY_EQUAL] = SC_EQUALS, [RAWKEY_BACKSPACE] = SC_BACKSPACE,
        [RAWKEY_TAB] = SC_TAB, [RAWKEY_Q] = SC_Q, [RAWKEY_W] = SC_W,
        [RAWKEY_E] = SC_E, [RAWKEY_R] = SC_R, [RAWKEY_T] = SC_T,
        [RAWKEY_Y] = SC_Y, [RAWKEY_U] = SC_U, [RAWKEY_I] = SC_I,
        [RAWKEY_O] = SC_O, [RAWKEY_P] = SC_P,
        [RAWKEY_LBRACKET] = SC_LEFTBRACKET,
        [RAWKEY_RBRACKET] = SC_RIGHTBRACKET,
        [RAWKEY_RETURN] = SC_ENTER, [RAWKEY_CONTROL] = SC_CONTROL,
        [RAWKEY_A] = SC_A, [RAWKEY_S] = SC_S, [RAWKEY_D] = SC_D,
        [RAWKEY_F] = SC_F, [RAWKEY_G] = SC_G, [RAWKEY_H] = SC_H,
        [RAWKEY_J] = SC_J, [RAWKEY_K] = SC_K, [RAWKEY_L] = SC_L,
        [RAWKEY_SEMICOLON] = SC_SEMICOLON, [RAWKEY_QUOTE] = SC_QUOTE,
        [RAWKEY_TILDE] = SC_TILDE, [RAWKEY_LSHIFT] = SC_LSHIFT,
        [RAWKEY_BACKSLASH] = SC_BACKSLASH, [RAWKEY_Z] = SC_Z,
        [RAWKEY_X] = SC_X, [RAWKEY_C] = SC_C, [RAWKEY_V] = SC_V,
        [RAWKEY_B] = SC_B, [RAWKEY_N] = SC_N, [RAWKEY_M] = SC_M,
        [RAWKEY_COMMA] = SC_COMMA, [RAWKEY_PERIOD] = SC_PERIOD,
        [RAWKEY_SLASH] = SC_SLASH, [RAWKEY_RSHIFT] = SC_RSHIFT,
        [RAWKEY_LALT] = SC_ALT, [RAWKEY_RALT] = SC_ALTGR,
        [RAWKEY_SPACE] = SC_SPACE, [RAWKEY_CAPSLOCK] = SC_CAPSLOCK,
        [RAWKEY_F1] = SC_F1, [RAWKEY_F2] = SC_F2, [RAWKEY_F3] = SC_F3,
        [RAWKEY_F4] = SC_F4, [RAWKEY_F5] = SC_F5, [RAWKEY_F6] = SC_F6,
        [RAWKEY_F7] = SC_F7, [RAWKEY_F8] = SC_F8, [RAWKEY_F9] = SC_F9,
        [RAWKEY_F10] = SC_F10, [RAWKEY_F11] = SC_F11,
        [RAWKEY_F12] = SC_F12, [RAWKEY_HOME] = SC_HOME,
        [RAWKEY_UP] = SC_UP, [RAWKEY_PAGEUP] = SC_PAGEUP,
        [RAWKEY_LEFT] = SC_LEFT, [RAWKEY_RIGHT] = SC_RIGHT,
        [RAWKEY_END] = SC_END, [RAWKEY_DOWN] = SC_DOWN,
        [RAWKEY_PAGEDOWN] = SC_PAGEDOWN, [RAWKEY_INSERT] = SC_INSERT,
        [RAWKEY_DELETE] = SC_DELETE, [RAWKEY_LAMIGA] = SC_LWIN,
        [RAWKEY_RAMIGA] = SC_RWIN, [RAWKEY_NUMLOCK] = SC_NUMLOCK,
        [RAWKEY_SCRLOCK] = SC_SCROLLLOCK,
        [RAWKEY_KP_MULTIPLY] = SC_MULTIPLY,
        [RAWKEY_KP_PLUS] = SC_PLUS, [RAWKEY_KP_MINUS] = SC_MINUS
    };

    return (raw < sizeof(map)) ? map[raw] : 0;
}

static int aros_extended_key(int scancode)
{
    if ((scancode >= SC_F1 && scancode <= SC_F12) ||
        scancode == SC_HOME || scancode == SC_UP ||
        scancode == SC_PAGEUP || scancode == SC_LEFT ||
        scancode == SC_CLEAR || scancode == SC_RIGHT ||
        scancode == SC_END || scancode == SC_DOWN ||
        scancode == SC_PAGEDOWN || scancode == SC_INSERT ||
        scancode == SC_DELETE)
    {
        return fb_hScancodeToExtendedKey(scancode);
    }

    return 0;
}

static void aros_post_key(struct IntuiMessage *message)
{
    struct InputEvent input_event;
    unsigned char text[8];
    EVENT event;
    unsigned int raw;
    int pressed;
    int scancode;
    int key;
    int count;
    int index;

    raw = message->Code & ~IECODE_UP_PREFIX;
    pressed = ((message->Code & IECODE_UP_PREFIX) == 0);

    if (raw == RAWKEY_NM_WHEEL_UP || raw == RAWKEY_NM_WHEEL_DOWN ||
        raw == RAWKEY_NM_WHEEL_LEFT || raw == RAWKEY_NM_WHEEL_RIGHT)
    {
        if (pressed)
        {
            memset(&event, 0, sizeof(event));
            if (raw == RAWKEY_NM_WHEEL_LEFT || raw == RAWKEY_NM_WHEEL_RIGHT)
            {
                event.type = EVENT_MOUSE_HWHEEL;
                event.w = (raw == RAWKEY_NM_WHEEL_LEFT) ? 1 : -1;
            }
            else
            {
                event.type = EVENT_MOUSE_WHEEL;
                event.z = (raw == RAWKEY_NM_WHEEL_UP) ? 1 : -1;
                fb_aros_gfx.mouse_z += event.z;
            }
            fb_hPostEvent(&event);
        }
        return;
    }

    scancode = aros_raw_to_scancode(raw);
    if (scancode > 0 && scancode < 128)
        __fb_gfx->key[scancode] = pressed ? TRUE : FALSE;

    memset(&event, 0, sizeof(event));
    event.type = pressed ? EVENT_KEY_PRESS : EVENT_KEY_RELEASE;
    event.scancode = scancode;

    memset(&input_event, 0, sizeof(input_event));
    input_event.ie_Class = IECLASS_RAWKEY;
    input_event.ie_Code = message->Code;
    input_event.ie_Qualifier = message->Qualifier;
    input_event.ie_EventAddress = message->IAddress;
    count = MapRawKey(&input_event, text, (LONG)sizeof(text), NULL);
    if (count > 0)
        event.ascii = text[0];
    fb_hPostEvent(&event);

    if (!pressed)
        return;

    key = aros_extended_key(scancode);
    if (key != 0)
    {
        fb_hPostKey(key);
        return;
    }

    for (index = 0; index < count; ++index)
        fb_hPostKey(text[index]);
}

static void aros_post_mouse_move(struct IntuiMessage *message)
{
    EVENT event;
    int next_x;
    int next_y;

    next_x = message->MouseX - fb_aros_gfx.window->BorderLeft;
    next_y = message->MouseY - fb_aros_gfx.window->BorderTop;
    next_x = MID(0, next_x, fb_aros_gfx.width - 1);
    next_y = MID(0, next_y, fb_aros_gfx.height - 1);

    if (next_x == fb_aros_gfx.mouse_x && next_y == fb_aros_gfx.mouse_y)
        return;

    memset(&event, 0, sizeof(event));
    event.type = EVENT_MOUSE_MOVE;
    event.x = next_x;
    event.y = next_y;
    event.dx = next_x - fb_aros_gfx.mouse_x;
    event.dy = next_y - fb_aros_gfx.mouse_y;
    fb_aros_gfx.mouse_x = next_x;
    fb_aros_gfx.mouse_y = next_y;
    fb_hPostEvent(&event);
}

static void aros_post_mouse_button(struct IntuiMessage *message)
{
    EVENT event;
    int button;
    int pressed;

    button = 0;
    pressed = FALSE;
    switch (message->Code)
    {
        case SELECTDOWN: button = BUTTON_LEFT; pressed = TRUE; break;
        case SELECTUP: button = BUTTON_LEFT; break;
        case MENUDOWN: button = BUTTON_RIGHT; pressed = TRUE; break;
        case MENUUP: button = BUTTON_RIGHT; break;
        case MIDDLEDOWN: button = BUTTON_MIDDLE; pressed = TRUE; break;
        case MIDDLEUP: button = BUTTON_MIDDLE; break;
        default: return;
    }

    if (pressed)
        fb_aros_gfx.mouse_buttons |= button;
    else
        fb_aros_gfx.mouse_buttons &= ~button;

    aros_post_mouse_move(message);
    memset(&event, 0, sizeof(event));
    event.type = pressed ? EVENT_MOUSE_BUTTON_PRESS :
        EVENT_MOUSE_BUTTON_RELEASE;
    event.button = button;
    fb_hPostEvent(&event);
}

void fb_arosGfxInputInit(void)
{
    fb_aros_gfx.mouse_x = 0;
    fb_aros_gfx.mouse_y = 0;
    fb_aros_gfx.mouse_z = 0;
    fb_aros_gfx.mouse_buttons = 0;
    fb_aros_gfx.mouse_clip = FALSE;
}

void fb_arosGfxInputExit(void)
{
}

void fb_arosGfxPollEvents(void)
{
    struct IntuiMessage *message;
    int refresh;

    if (!fb_aros_gfx.active || fb_aros_gfx.window == NULL ||
        __fb_gfx == NULL || __fb_gfx->key == NULL)
    {
        return;
    }

    refresh = FALSE;
    while ((message = (struct IntuiMessage *)GetMsg(
        fb_aros_gfx.window->UserPort)) != NULL)
    {
        switch (message->Class)
        {
            case IDCMP_RAWKEY:
                aros_post_key(message);
                break;
            case IDCMP_MOUSEMOVE:
                aros_post_mouse_move(message);
                break;
            case IDCMP_MOUSEBUTTONS:
                aros_post_mouse_button(message);
                break;
            case IDCMP_ACTIVEWINDOW:
            case IDCMP_INACTIVEWINDOW:
            {
                EVENT event;

                memset(&event, 0, sizeof(event));
                event.type = (message->Class == IDCMP_ACTIVEWINDOW)
                    ? EVENT_WINDOW_GOT_FOCUS
                    : EVENT_WINDOW_LOST_FOCUS;
                fb_hPostEvent(&event);
                break;
            }
            case IDCMP_CLOSEWINDOW:
            {
                EVENT event;

                memset(&event, 0, sizeof(event));
                event.type = EVENT_WINDOW_CLOSE;
                fb_hPostEvent(&event);
                fb_hPostKey(KEY_QUIT);
                break;
            }
            case IDCMP_REFRESHWINDOW:
                BeginRefresh(fb_aros_gfx.window);
                EndRefresh(fb_aros_gfx.window, TRUE);
                refresh = TRUE;
                break;
        }

        ReplyMsg((struct Message *)message);
    }

    if (refresh)
    {
        if (__fb_gfx->dirty != NULL)
            memset(__fb_gfx->dirty, TRUE, (size_t)fb_aros_gfx.height);
        fb_arosGfxPresent();
    }
}

int fb_arosGfxGetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
    if (!fb_aros_gfx.active)
        return -1;

    fb_arosGfxPollEvents();
    *x = fb_aros_gfx.mouse_x;
    *y = fb_aros_gfx.mouse_y;
    *z = fb_aros_gfx.mouse_z;
    *buttons = fb_aros_gfx.mouse_buttons;
    *clip = fb_aros_gfx.mouse_clip;
    return 0;
}

void fb_arosGfxSetMouse(int x, int y, int cursor, int clip)
{
    if (!fb_aros_gfx.active)
        return;

    if (x >= 0)
        fb_aros_gfx.mouse_x = MID(0, x, fb_aros_gfx.width - 1);
    if (y >= 0)
        fb_aros_gfx.mouse_y = MID(0, y, fb_aros_gfx.height - 1);
    if (clip >= 0)
        fb_aros_gfx.mouse_clip = (clip != 0);

    /*
        Intuition owns the visible pointer.  Keeping that pointer selected is
        deliberate: it guarantees cursor pixels stay above CyberGraphX blits.
        The requested state is retained for GETMOUSE compatibility.
    */
    if (cursor >= 0)
        fb_aros_gfx.cursor_visible = (cursor != 0);
}

/* end of gfx_input.c */
