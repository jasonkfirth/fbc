/*
    FreeBASIC gfxlib2 support for RISC OS
    -------------------------------------

    File: gfx_input.c

    Purpose:

        Translate native RISC OS keyboard and pointer state for gfxlib2.

    Responsibilities:

        - scan live internal key numbers for MULTIKEY
        - consume translated keyboard-buffer characters for INKEY and GETKEY
        - preserve and control the native RISC OS pointer selection
        - report pointer coordinates and RISC OS's three mouse buttons
        - post keyboard and mouse events to the gfxlib2 event queue

    This file intentionally does NOT contain:

        - screen-memory access
        - graphics mode selection
        - Wimp event handling

    Input model:

        RISC OS exposes physical held-key state and translated character input
        through different APIs.  OS_Byte 121/129 supplies layout-independent
        physical state; a zero-timeout OS_Byte 129 read supplies layout-aware
        characters and function-key buffer codes.  Keeping both paths avoids
        making games choose between MULTIKEY and international text input.
*/

#include "fb_gfx_riscos.h"

#include <kernel.h>
#include <string.h>
#include <swis.h>

/* ------------------------------------------------------------------------- */
/* Physical RISC OS key mapping                                              */
/* ------------------------------------------------------------------------- */

static const unsigned char riscos_to_fb_scancode[128] =
{
    0,              SC_CONTROL,     SC_ALT,         SC_LSHIFT,
    SC_CONTROL,     SC_ALT,         SC_RSHIFT,      SC_CONTROL,
    SC_ALTGR,       0,              0,              0,
    0,              0,              0,              0,
    SC_Q,           SC_3,           SC_4,           SC_5,
    SC_F4,          SC_8,           SC_F7,          SC_MINUS,
    0,              SC_LEFT,        0,              0,
    SC_F11,         SC_F12,         SC_F10,         SC_SCROLLLOCK,
    0,              SC_W,           SC_E,           SC_T,
    SC_7,           SC_I,           SC_9,           SC_0,
    0,              SC_DOWN,        0,              0,
    0,              SC_TILDE,       0,              SC_BACKSPACE,
    SC_1,           SC_2,           SC_D,           SC_R,
    SC_6,           SC_U,           SC_O,           SC_P,
    SC_LEFTBRACKET, SC_UP,          SC_PLUS,        SC_MINUS,
    SC_ENTER,       SC_INSERT,      SC_HOME,        SC_PAGEUP,
    SC_CAPSLOCK,    SC_A,           SC_X,           SC_F,
    SC_Y,           SC_J,           SC_K,           0,
    SC_SEMICOLON,   SC_ENTER,       SC_SLASH,       0,
    SC_PERIOD,      SC_NUMLOCK,     SC_PAGEDOWN,    SC_QUOTE,
    0,              SC_S,           SC_C,           SC_G,
    SC_H,           SC_N,           SC_L,           SC_SEMICOLON,
    SC_RIGHTBRACKET,SC_DELETE,      0,              SC_MULTIPLY,
    0,              SC_EQUALS,      SC_BACKSLASH,   0,
    SC_TAB,         SC_Z,           SC_SPACE,       SC_V,
    SC_B,           SC_M,           SC_COMMA,       SC_PERIOD,
    SC_SLASH,       SC_END,         SC_INSERT,      SC_END,
    SC_PAGEDOWN,    0,              0,              0,
    SC_ESCAPE,      SC_F1,          SC_F2,          SC_F3,
    SC_F5,          SC_F6,          SC_F8,          SC_F9,
    SC_BACKSLASH,   SC_RIGHT,       SC_LEFT,        SC_CLEAR,
    SC_DOWN,        SC_LWIN,        SC_RWIN,        SC_MENU
};

/*
    Some keys have duplicate internal numbers for old keyboard compatibility.
    Ignore the duplicates when enumerating held keys or one physical press can
    appear twice in gfxlib2's state table.
*/

static int riscos_duplicate_key_number(int key_number)
{
    return key_number == 24 || key_number == 40 ||
        key_number == 71 || key_number == 87;
}

static int riscos_key_is_down(int key_number)
{
    int result;

    result = _kernel_osbyte(129, key_number ^ 255, 255);
    return ((result & 255) == 255);
}

static void riscos_post_physical_event(int scancode, int pressed)
{
    EVENT event;

    memset(&event, 0, sizeof(event));
    event.type = pressed ? EVENT_KEY_PRESS : EVENT_KEY_RELEASE;
    event.scancode = scancode;
    event.ascii = 0;
    fb_hPostEvent(&event);
}

static void riscos_poll_physical_keys(void)
{
    unsigned char current[128];
    int either_shift;
    int fb_scancode;
    int key_number;
    int left_shift;
    int previous;
    int right_shift;

    memset(current, 0, sizeof(current));
    key_number = 2;

    while (key_number < 255)
    {
        key_number = _kernel_osbyte(121, key_number + 1, 0) & 255;
        if (key_number >= 128)
            break;

        if (riscos_duplicate_key_number(key_number))
            continue;

        fb_scancode = riscos_to_fb_scancode[key_number];
        if (fb_scancode > 0 && fb_scancode < 128)
            current[fb_scancode] = TRUE;
    }

    /* Modifier keys sit below OS_Byte 121's normal enumeration threshold. */

    either_shift = riscos_key_is_down(0);
    left_shift = riscos_key_is_down(3);
    right_shift = riscos_key_is_down(6);

    if (left_shift)
        current[SC_LSHIFT] = TRUE;

    if (right_shift)
        current[SC_RSHIFT] = TRUE;

    /* Older keyboards may expose only the common Shift key number. */

    if (either_shift && !left_shift && !right_shift)
        current[SC_LSHIFT] = TRUE;

    if (riscos_key_is_down(1) || riscos_key_is_down(4) ||
        riscos_key_is_down(7))
    {
        current[SC_CONTROL] = TRUE;
    }

    if (riscos_key_is_down(2) || riscos_key_is_down(5))
        current[SC_ALT] = TRUE;

    if (riscos_key_is_down(8))
        current[SC_ALTGR] = TRUE;

    for (fb_scancode = 1; fb_scancode < 128; ++fb_scancode)
    {
        previous = (__fb_gfx->key[fb_scancode] != FALSE);
        __fb_gfx->key[fb_scancode] = current[fb_scancode];

        if (previous != (current[fb_scancode] != FALSE))
        {
            riscos_post_physical_event(fb_scancode,
                current[fb_scancode] != FALSE);
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Translated keyboard buffer                                                */
/* ------------------------------------------------------------------------- */

int fb_riscosGfxTranslateCharacter(int character)
{
    int function_key;

    /*
        Wimp_KeyPressed uses the same function-key families as the keyboard
        buffer, with modifier state represented by the upper hexadecimal
        digit.  gfxlib2 reports the corresponding extended key while its
        physical-key path retains MULTIKEY modifier state separately.
    */

    if (character >= 0x181 && character <= 0x1B9 &&
        (character & 15) >= 1 && (character & 15) <= 9)
    {
        function_key = character & 15;
        return fb_hScancodeToExtendedKey(SC_F1 + function_key - 1);
    }

    if (character >= 0x1CA && character <= 0x1FC &&
        (character & 15) >= 10 && (character & 15) <= 12)
    {
        function_key = character & 15;
        return fb_hScancodeToExtendedKey(SC_F10 + function_key - 10);
    }

    if (character >= 0x81 && character <= 0x89)
        return fb_hScancodeToExtendedKey(SC_F1 + character - 0x81);

    switch (character)
    {
        case 0x8c: return KEY_LEFT;
        case 0x8d: return KEY_RIGHT;
        case 0x8e: return KEY_DOWN;
        case 0x8f: return KEY_UP;
        case 0x18a: case 0x19a: case 0x1aa: case 0x1ba:
            return KEY_TAB;
        case 0x18b: case 0x19b: case 0x1ab: case 0x1bb:
            return KEY_DEL;
        case 0x18c: case 0x19c: case 0x1ac: case 0x1bc:
            return KEY_LEFT;
        case 0x18d: case 0x19d: case 0x1ad: case 0x1bd:
            return KEY_RIGHT;
        case 0x18e: case 0x19e: case 0x1ae: case 0x1be:
            return KEY_DOWN;
        case 0x18f: case 0x19f: case 0x1af: case 0x1bf:
            return KEY_UP;
        case 0x1cd: case 0x1dd: case 0x1ed: case 0x1fd:
            return KEY_INS;
        case 127:  return KEY_BACKSPACE;
        default:   return character;
    }
}

static void riscos_poll_characters(void)
{
    int character;
    int reads_left;
    int result;

    reads_left = 32;

    while (reads_left-- > 0)
    {
        result = _kernel_osbyte(129, 0, 0);
        if (((result >> 8) & 255) != 0)
            break;

        character = fb_riscosGfxTranslateCharacter(result & 255);
        if (character != 0)
            fb_hPostKey(character);
    }
}

/* ------------------------------------------------------------------------- */
/* Pointer handling                                                          */
/* ------------------------------------------------------------------------- */

static int riscos_clamp(int value, int lower, int upper)
{
    if (value < lower)
        return lower;
    if (value > upper)
        return upper;
    return value;
}

static void riscos_poll_mouse(void)
{
    _kernel_swi_regs registers;
    EVENT event;
    int buttons;
    int old_buttons;
    int old_x;
    int old_y;
    int screen_x;
    int screen_y;

    memset(&registers, 0, sizeof(registers));
    if (_kernel_swi(OS_Mouse, &registers, &registers) != NULL)
        return;

    screen_x = registers.r[0] >> fb_riscos_gfx.x_eigen;
    screen_y = fb_riscos_gfx.screen_height - 1 -
        (registers.r[1] >> fb_riscos_gfx.y_eigen);

    old_x = fb_riscos_gfx.mouse_x;
    old_y = fb_riscos_gfx.mouse_y;
    old_buttons = fb_riscos_gfx.mouse_buttons;

    fb_riscos_gfx.mouse_x = riscos_clamp(
        screen_x - fb_riscos_gfx.viewport_x,
        0, fb_riscos_gfx.viewport_width - 1);
    fb_riscos_gfx.mouse_y = riscos_clamp(
        screen_y - fb_riscos_gfx.viewport_y,
        0, fb_riscos_gfx.viewport_height - 1);

    buttons = registers.r[2];
    fb_riscos_gfx.mouse_buttons =
        ((buttons & 4) ? BUTTON_LEFT : 0) |
        ((buttons & 1) ? BUTTON_RIGHT : 0) |
        ((buttons & 2) ? BUTTON_MIDDLE : 0);

    if (old_x != fb_riscos_gfx.mouse_x || old_y != fb_riscos_gfx.mouse_y)
    {
        memset(&event, 0, sizeof(event));
        event.type = EVENT_MOUSE_MOVE;
        event.x = fb_riscos_gfx.mouse_x;
        event.y = fb_riscos_gfx.mouse_y;
        event.dx = fb_riscos_gfx.mouse_x - old_x;
        event.dy = fb_riscos_gfx.mouse_y - old_y;
        fb_hPostEvent(&event);
    }

    buttons = old_buttons ^ fb_riscos_gfx.mouse_buttons;
    for (event.button = BUTTON_LEFT; event.button <= BUTTON_MIDDLE;
        event.button <<= 1)
    {
        if (buttons & event.button)
        {
            int button;

            button = event.button;
            memset(&event, 0, sizeof(event));
            event.button = button;
            event.type = (fb_riscos_gfx.mouse_buttons & event.button)
                ? EVENT_MOUSE_BUTTON_PRESS
                : EVENT_MOUSE_BUTTON_RELEASE;
            fb_hPostEvent(&event);
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Public input lifecycle                                                    */
/* ------------------------------------------------------------------------- */

void fb_riscosGfxInputInit(void)
{
    int pointer_result;

    fb_riscos_gfx.mouse_clip = FALSE;

    if (fb_riscos_gfx.windowed)
    {
        fb_riscosGfxWindowPollEvents();
        return;
    }

    /* Let Escape enter the keyboard buffer instead of raising SIGINT. */

    (void)_kernel_osbyte(229, 1, 0);

    /*
        Graphics drivers show their pointer by default.  OS_Byte 106 returns
        the setting it replaces, so retain the user's exact pointer shape and
        linkage flag for teardown rather than assuming pointer one was active.
    */

    pointer_result = _kernel_osbyte(106, 1, 0);
    if (pointer_result >= 0)
    {
        fb_riscos_gfx.original_pointer_setting = pointer_result & 255;
        fb_riscos_gfx.current_pointer_setting = 1;
        fb_riscos_gfx.pointer_setting_saved = 1;
    }
    else
    {
        fb_riscosGfxDebug("OS_Byte 106 failed while enabling the pointer");
    }

    riscos_poll_mouse();
}

void fb_riscosGfxInputExit(void)
{
    if (fb_riscos_gfx.windowed)
        return;

    if (fb_riscos_gfx.pointer_setting_saved)
    {
        (void)_kernel_osbyte(106,
            fb_riscos_gfx.original_pointer_setting, 0);
    }

    (void)_kernel_osbyte(229, 0, 0);
}

void fb_riscosGfxPollEvents(void)
{
    if (!fb_riscos_gfx.active || __fb_gfx == NULL || __fb_gfx->key == NULL)
        return;

    if (fb_riscos_gfx.windowed)
    {
        fb_riscosGfxWindowPollEvents();
        if (!fb_riscos_gfx.active)
            return;

        if (fb_riscosGfxWindowHasInputFocus())
            riscos_poll_physical_keys();

        (void)fb_riscosGfxWindowGetMouse(NULL, NULL, NULL, NULL, NULL);
        return;
    }

    riscos_poll_physical_keys();
    riscos_poll_characters();
    riscos_poll_mouse();
}

int fb_riscosGfxGetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
    if (fb_riscos_gfx.windowed)
        return fb_riscosGfxWindowGetMouse(x, y, z, buttons, clip);

    if (!fb_riscos_gfx.active)
        return -1;

    riscos_poll_mouse();

    if (x != NULL)
        *x = fb_riscos_gfx.mouse_x;
    if (y != NULL)
        *y = fb_riscos_gfx.mouse_y;
    if (z != NULL)
        *z = fb_riscos_gfx.mouse_z;
    if (buttons != NULL)
        *buttons = fb_riscos_gfx.mouse_buttons;
    if (clip != NULL)
        *clip = fb_riscos_gfx.mouse_clip;

    return 0;
}

void fb_riscosGfxSetMouse(int x, int y, int cursor, int clip)
{
    union
    {
        int aligned[2];
        unsigned char bytes[8];
    } parameters;
    int screen_x;
    int screen_y;

    if (fb_riscos_gfx.windowed)
    {
        fb_riscosGfxWindowSetMouse(x, y, cursor, clip);
        return;
    }

    if (!fb_riscos_gfx.active)
        return;

    if (clip >= 0)
        fb_riscos_gfx.mouse_clip = (clip != 0);

    if (cursor >= 0)
    {
        int pointer_setting;

        pointer_setting = cursor ? 1 : 0;
        if (_kernel_osbyte(106, pointer_setting, 0) >= 0)
            fb_riscos_gfx.current_pointer_setting = pointer_setting;
    }

    if (x < 0 || y < 0)
        return;

    x = riscos_clamp(x, 0, fb_riscos_gfx.viewport_width - 1);
    y = riscos_clamp(y, 0, fb_riscos_gfx.viewport_height - 1);
    screen_x = (fb_riscos_gfx.viewport_x + x) << fb_riscos_gfx.x_eigen;
    screen_y = (fb_riscos_gfx.screen_height - 1 -
        (fb_riscos_gfx.viewport_y + y)) << fb_riscos_gfx.y_eigen;

    memset(&parameters, 0, sizeof(parameters));
    parameters.bytes[0] = 3;
    parameters.bytes[1] = (unsigned char)(screen_x & 255);
    parameters.bytes[2] = (unsigned char)((screen_x >> 8) & 255);
    parameters.bytes[3] = (unsigned char)(screen_y & 255);
    parameters.bytes[4] = (unsigned char)((screen_y >> 8) & 255);
    (void)_kernel_osword(21, parameters.aligned);

    parameters.bytes[0] = 5;
    (void)_kernel_osword(21, parameters.aligned);

    fb_riscos_gfx.mouse_x = x;
    fb_riscos_gfx.mouse_y = y;
}

int fb_riscosGfxPointerIsVisible(void)
{
    int pointer_result;

    if (fb_riscos_gfx.windowed)
        return fb_riscos_gfx.active;

    if (!fb_riscos_gfx.active || fb_riscos_gfx.cursors_removed ||
        fb_riscos_gfx.current_pointer_setting == 0)
    {
        return FALSE;
    }

    /* Setting the current value again returns the actual previous setting. */

    pointer_result = _kernel_osbyte(106,
        fb_riscos_gfx.current_pointer_setting, 0);
    if (pointer_result < 0)
        return FALSE;

    return ((pointer_result & 127) != 0);
}

/* end of gfx_input.c */
