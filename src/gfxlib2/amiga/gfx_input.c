/*
    FreeBASIC gfxlib2 support for AmigaOS
    ----------------------------------

    File: gfx_input.c

    Purpose:

        Translate Intuition input messages into FreeBASIC graphics input.

    Responsibilities:

        - map AMIGA raw keys to FreeBASIC scancodes
        - use keymap.library for layout-aware text input
        - track mouse position, buttons, wheel, and focus
        - preserve the native Intuition cursor above graphics updates

    This file intentionally does NOT contain:

        - framebuffer conversion
        - display-window construction
        - global input.device hooks
*/

#include "fb_gfx_amiga.h"

#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/keymap.h>

#include <string.h>

/* Classic input.device raw codes reserve 0x7a..0x7d for wheel events. */
#define RAWKEY_NM_WHEEL_UP 0x7a
#define RAWKEY_NM_WHEEL_DOWN 0x7b
#define RAWKEY_NM_WHEEL_LEFT 0x7c
#define RAWKEY_NM_WHEEL_RIGHT 0x7d

struct Library *KeymapBase;

/* Amiga scancode to FreeBASIC (PC Set 1) scancode mapping */
static const unsigned char amiga_to_fb_scancode[128] = {
    /* 0x00-0x0F: ` 1 2 3 4 5 6 7 8 9 0 - = \ x Del */
    SC_TILDE, SC_1, SC_2, SC_3, SC_4, SC_5, SC_6, SC_7,
    SC_8, SC_9, SC_0, SC_MINUS, SC_EQUALS, SC_BACKSLASH, 0, SC_DELETE,
    /* 0x10-0x1F: Q W E R T Y U I O P [ ] x x x KP0 */
    SC_Q, SC_W, SC_E, SC_R, SC_T, SC_Y, SC_U, SC_I,
    SC_O, SC_P, SC_LEFTBRACKET, SC_RIGHTBRACKET, 0, 0, 0, SC_INSERT,
    /* 0x20-0x2F: A S D F G H J K L ; ' x x x x KP. */
    SC_A, SC_S, SC_D, SC_F, SC_G, SC_H, SC_J, SC_K,
    SC_L, SC_SEMICOLON, SC_QUOTE, 0, 0, 0, 0, SC_DELETE,
    /* 0x30-0x3F: x Z X C V B N M , . / x x x x x */
    0, SC_Z, SC_X, SC_C, SC_V, SC_B, SC_N, SC_M,
    SC_COMMA, SC_PERIOD, SC_SLASH, 0, 0, 0, 0, 0,
    /* 0x40-0x4F: Space BS Tab Enter Ret Esc x x x x KP- x Up Down Right Left */
    SC_SPACE, SC_BACKSPACE, SC_TAB, SC_ENTER, SC_ENTER, SC_ESCAPE, 0, 0,
    0, 0, 0x4A, 0, SC_UP, SC_DOWN, SC_RIGHT, SC_LEFT,
    /* 0x50-0x5F: F1-F10 x x */
    SC_F1, SC_F2, SC_F3, SC_F4, SC_F5, SC_F6, SC_F7, SC_F8,
    SC_F9, SC_F10, 0, 0, 0, 0, 0, 0,
    /* 0x60-0x6F: LShift RShift CapsLk Ctrl LAlt RAlt LAmiga RAmiga ... */
    SC_LSHIFT, SC_RSHIFT, SC_CAPSLOCK, SC_CONTROL, SC_ALT, SC_ALTGR, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x70-0x7F: unused */
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};
static int amiga_raw_to_scancode(unsigned int raw)
{
    return raw < sizeof(amiga_to_fb_scancode) ? amiga_to_fb_scancode[raw] : 0;
}

static int amiga_extended_key(int scancode)
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

static void amiga_post_key(struct IntuiMessage *message)
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
                fb_amiga_gfx.mouse_z += event.z;
            }
            fb_hPostEvent(&event);
        }
        return;
    }

    scancode = amiga_raw_to_scancode(raw);
    if (scancode > 0 && scancode < 128)
        __fb_gfx->key[scancode] = pressed ? TRUE : FALSE;

    memset(&event, 0, sizeof(event));
    event.type = pressed ? EVENT_KEY_PRESS : EVENT_KEY_RELEASE;
    event.scancode = scancode;

    memset(&input_event, 0, sizeof(input_event));
    input_event.ie_Class = IECLASS_RAWKEY;
    input_event.ie_Code = message->Code;
    input_event.ie_Qualifier = message->Qualifier;
    /* IDCMP_RAWKEY stores the dead-key state behind IAddress. MapRawKey
       expects the stored address, rather than the Intuition wrapper. */
    if (message->IAddress != NULL)
        input_event.ie_EventAddress = *(APTR *)message->IAddress;
    count = KeymapBase != NULL
        ? MapRawKey(&input_event, (STRPTR)text, (LONG)sizeof(text), NULL) : 0;
    if (count < 0 || count > (int)sizeof(text))
        count = 0;
    if (count > 0)
        event.ascii = text[0];
    fb_hPostEvent(&event);

    if (!pressed)
        return;

    key = amiga_extended_key(scancode);
    if (key != 0)
    {
        fb_hPostKey(key);
        return;
    }

    for (index = 0; index < count; ++index)
        fb_hPostKey(text[index]);
}

static void amiga_post_mouse_move(struct IntuiMessage *message)
{
    EVENT event;
    int next_x;
    int next_y;

    next_x = message->MouseX - fb_amiga_gfx.window->BorderLeft;
    next_y = message->MouseY - fb_amiga_gfx.window->BorderTop;
    next_x = MID(0, next_x, fb_amiga_gfx.width - 1);
    next_y = MID(0, next_y, fb_amiga_gfx.height - 1);

    if (next_x == fb_amiga_gfx.mouse_x && next_y == fb_amiga_gfx.mouse_y)
        return;

    memset(&event, 0, sizeof(event));
    event.type = EVENT_MOUSE_MOVE;
    event.x = next_x;
    event.y = next_y;
    event.dx = next_x - fb_amiga_gfx.mouse_x;
    event.dy = next_y - fb_amiga_gfx.mouse_y;
    fb_amiga_gfx.mouse_x = next_x;
    fb_amiga_gfx.mouse_y = next_y;
    fb_hPostEvent(&event);
}

static void amiga_post_mouse_button(struct IntuiMessage *message)
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
        fb_amiga_gfx.mouse_buttons |= button;
    else
        fb_amiga_gfx.mouse_buttons &= ~button;

    amiga_post_mouse_move(message);
    memset(&event, 0, sizeof(event));
    event.type = pressed ? EVENT_MOUSE_BUTTON_PRESS :
        EVENT_MOUSE_BUTTON_RELEASE;
    event.button = button;
    fb_hPostEvent(&event);
}

void fb_amigaGfxInputInit(void)
{
    KeymapBase = OpenLibrary("keymap.library", 37);
    fb_amiga_gfx.mouse_x = 0;
    fb_amiga_gfx.mouse_y = 0;
    fb_amiga_gfx.mouse_z = 0;
    fb_amiga_gfx.mouse_buttons = 0;
    fb_amiga_gfx.mouse_clip = FALSE;
}

void fb_amigaGfxInputExit(void)
{
    if (KeymapBase != NULL)
        CloseLibrary(KeymapBase);
    KeymapBase = NULL;
}

void fb_amigaGfxPollEvents(void)
{
    struct IntuiMessage *message;
    int refresh;

    if (!fb_amiga_gfx.active || fb_amiga_gfx.window == NULL ||
        __fb_gfx == NULL || __fb_gfx->key == NULL)
    {
        return;
    }

    refresh = FALSE;
    while ((message = (struct IntuiMessage *)GetMsg(
        fb_amiga_gfx.window->UserPort)) != NULL)
    {
        switch (message->Class)
        {
            case IDCMP_RAWKEY:
                amiga_post_key(message);
                break;
            case IDCMP_MOUSEMOVE:
                amiga_post_mouse_move(message);
                break;
            case IDCMP_MOUSEBUTTONS:
                amiga_post_mouse_button(message);
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
                BeginRefresh(fb_amiga_gfx.window);
                EndRefresh(fb_amiga_gfx.window, TRUE);
                refresh = TRUE;
                break;
        }

        ReplyMsg((struct Message *)message);
    }

    if (refresh)
    {
        if (__fb_gfx->dirty != NULL)
            memset(__fb_gfx->dirty, TRUE, (size_t)fb_amiga_gfx.height);
        fb_amigaGfxPresent();
    }
}

int fb_amigaGfxGetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
    if (!fb_amiga_gfx.active)
        return -1;

    fb_amigaGfxPollEvents();
    *x = fb_amiga_gfx.mouse_x;
    *y = fb_amiga_gfx.mouse_y;
    *z = fb_amiga_gfx.mouse_z;
    *buttons = fb_amiga_gfx.mouse_buttons;
    *clip = fb_amiga_gfx.mouse_clip;
    return 0;
}

void fb_amigaGfxSetMouse(int x, int y, int cursor, int clip)
{
    if (!fb_amiga_gfx.active)
        return;

    if (x >= 0)
        fb_amiga_gfx.mouse_x = MID(0, x, fb_amiga_gfx.width - 1);
    if (y >= 0)
        fb_amiga_gfx.mouse_y = MID(0, y, fb_amiga_gfx.height - 1);
    if (clip >= 0)
        fb_amiga_gfx.mouse_clip = (clip != 0);

    /*
        Intuition owns the visible pointer.  Keeping that pointer selected is
        deliberate: it guarantees cursor pixels stay above CyberGraphX blits.
        The requested state is retained for GETMOUSE compatibility.
    */
    if (cursor >= 0)
        fb_amiga_gfx.cursor_visible = (cursor != 0);
}

/* end of gfx_input.c */
