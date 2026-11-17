/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/scancodes_x11.c
 * Purpose: Provide scancodes x11 runtime support.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_scancodes_x11.h"

#ifndef DISABLE_X11

typedef struct KeysymToScancode {
	KeySym keysym;
	int scancode;
#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8)
	/* Keep the native KeySym-aligned table stride. */
	unsigned char tail_padding[4];
#endif
} KeysymToScancode;

static const KeysymToScancode keysym_to_scancode[] =
{
	{ .keysym = XK_Escape, .scancode = SC_ESCAPE },
	{ .keysym = XK_F1, .scancode = SC_F1 },
	{ .keysym = XK_F2, .scancode = SC_F2 },
	{ .keysym = XK_F3, .scancode = SC_F3 },
	{ .keysym = XK_F4, .scancode = SC_F4 },
	{ .keysym = XK_F5, .scancode = SC_F5 },
	{ .keysym = XK_F6, .scancode = SC_F6 },
	{ .keysym = XK_F7, .scancode = SC_F7 },
	{ .keysym = XK_F8, .scancode = SC_F8 },
	{ .keysym = XK_F9, .scancode = SC_F9 },
	{ .keysym = XK_F10, .scancode = SC_F10 },
	{ .keysym = XK_F11, .scancode = SC_F11 },
	{ .keysym = XK_F12, .scancode = SC_F12 },
	{ .keysym = XK_Scroll_Lock, .scancode = SC_SCROLLLOCK },
	{ .keysym = XK_grave, .scancode = SC_TILDE },
	{ .keysym = XK_quoteleft, .scancode = SC_TILDE },
	{ .keysym = XK_asciitilde, .scancode = SC_TILDE },
	{ .keysym = XK_1, .scancode = SC_1 },
	{ .keysym = XK_2, .scancode = SC_2 },
	{ .keysym = XK_3, .scancode = SC_3 },
	{ .keysym = XK_4, .scancode = SC_4 },
	{ .keysym = XK_5, .scancode = SC_5 },
	{ .keysym = XK_6, .scancode = SC_6 },
	{ .keysym = XK_7, .scancode = SC_7 },
	{ .keysym = XK_8, .scancode = SC_8 },
	{ .keysym = XK_9, .scancode = SC_9 },
	{ .keysym = XK_0, .scancode = SC_0 },
	{ .keysym = XK_minus, .scancode = SC_MINUS },
	{ .keysym = XK_equal, .scancode = SC_EQUALS },
	{ .keysym = XK_backslash, .scancode = SC_BACKSLASH },
	{ .keysym = XK_BackSpace, .scancode = SC_BACKSPACE },
	{ .keysym = XK_Tab, .scancode = SC_TAB },
	{ .keysym = XK_q, .scancode = SC_Q },
	{ .keysym = XK_w, .scancode = SC_W },
	{ .keysym = XK_e, .scancode = SC_E },
	{ .keysym = XK_r, .scancode = SC_R },
	{ .keysym = XK_t, .scancode = SC_T },
	{ .keysym = XK_y, .scancode = SC_Y },
	{ .keysym = XK_u, .scancode = SC_U },
	{ .keysym = XK_i, .scancode = SC_I },
	{ .keysym = XK_o, .scancode = SC_O },
	{ .keysym = XK_p, .scancode = SC_P },
	{ .keysym = XK_bracketleft, .scancode = SC_LEFTBRACKET },
	{ .keysym = XK_bracketright, .scancode = SC_RIGHTBRACKET },
	{ .keysym = XK_Return, .scancode = SC_ENTER },
	{ .keysym = XK_Caps_Lock, .scancode = SC_CAPSLOCK },
	{ .keysym = XK_a, .scancode = SC_A },
	{ .keysym = XK_s, .scancode = SC_S },
	{ .keysym = XK_d, .scancode = SC_D },
	{ .keysym = XK_f, .scancode = SC_F },
	{ .keysym = XK_g, .scancode = SC_G },
	{ .keysym = XK_h, .scancode = SC_H },
	{ .keysym = XK_j, .scancode = SC_J },
	{ .keysym = XK_k, .scancode = SC_K },
	{ .keysym = XK_l, .scancode = SC_L },
	{ .keysym = XK_semicolon, .scancode = SC_SEMICOLON },
	{ .keysym = XK_apostrophe, .scancode = SC_QUOTE },
	{ .keysym = XK_Shift_L, .scancode = SC_LSHIFT },
	{ .keysym = XK_z, .scancode = SC_Z },
	{ .keysym = XK_x, .scancode = SC_X },
	{ .keysym = XK_c, .scancode = SC_C },
	{ .keysym = XK_v, .scancode = SC_V },
	{ .keysym = XK_b, .scancode = SC_B },
	{ .keysym = XK_n, .scancode = SC_N },
	{ .keysym = XK_m, .scancode = SC_M },
	{ .keysym = XK_comma, .scancode = SC_COMMA },
	{ .keysym = XK_period, .scancode = SC_PERIOD },
	{ .keysym = XK_slash, .scancode = SC_SLASH },
	{ .keysym = XK_Shift_R, .scancode = SC_RSHIFT },
	{ .keysym = XK_Control_L, .scancode = SC_CONTROL },
	{ .keysym = XK_Meta_L, .scancode = SC_LWIN },
	{ .keysym = XK_Alt_L, .scancode = SC_ALT },
	{ .keysym = XK_space, .scancode = SC_SPACE },
	{ .keysym = XK_Alt_R, .scancode = SC_ALT },
	{ .keysym = XK_Meta_R, .scancode = SC_RWIN },
	{ .keysym = XK_Menu, .scancode = SC_MENU },
	{ .keysym = XK_Control_R, .scancode = SC_CONTROL },
	{ .keysym = XK_Insert, .scancode = SC_INSERT },
	{ .keysym = XK_Home, .scancode = SC_HOME },
	{ .keysym = XK_Prior, .scancode = SC_PAGEUP },
	{ .keysym = XK_Delete, .scancode = SC_DELETE },
	{ .keysym = XK_End, .scancode = SC_END },
	{ .keysym = XK_Next, .scancode = SC_PAGEDOWN },
	{ .keysym = XK_Up, .scancode = SC_UP },
	{ .keysym = XK_Left, .scancode = SC_LEFT },
	{ .keysym = XK_Down, .scancode = SC_DOWN },
	{ .keysym = XK_Right, .scancode = SC_RIGHT },
	{ .keysym = XK_Num_Lock, .scancode = SC_NUMLOCK },
	{ .keysym = XK_KP_Divide, .scancode = SC_SLASH },
	{ .keysym = XK_KP_Multiply, .scancode = SC_MULTIPLY },
	{ .keysym = XK_KP_Subtract, .scancode = SC_MINUS },
	{ .keysym = XK_KP_Home, .scancode = SC_HOME },
	{ .keysym = XK_KP_Up, .scancode = SC_UP },
	{ .keysym = XK_KP_Prior, .scancode = SC_PAGEUP },
	{ .keysym = XK_KP_Add, .scancode = SC_PLUS },
	{ .keysym = XK_KP_Left, .scancode = SC_LEFT },
	{ .keysym = XK_KP_Begin, .scancode = SC_CLEAR },
	{ .keysym = XK_KP_Right, .scancode = SC_RIGHT },
	{ .keysym = XK_KP_End, .scancode = SC_END },
	{ .keysym = XK_KP_Down, .scancode = SC_DOWN },
	{ .keysym = XK_KP_Next, .scancode = SC_PAGEDOWN },
	{ .keysym = XK_KP_Enter, .scancode = SC_ENTER },
	{ .keysym = XK_KP_Insert, .scancode = SC_INSERT },
	{ .keysym = XK_KP_Delete, .scancode = SC_DELETE },
	{ .keysym = NoSymbol, .scancode = 0 }
};

unsigned char fb_x11keycode_to_scancode[256];

void fb_hInitX11KeycodeToScancodeTb
	(
		Display *display,
		XDISPLAYKEYCODES DisplayKeycodes,
		XGETKEYBOARDMAPPING GetKeyboardMapping,
		XFREE Free
	)
{
	int keycode_min = 0, keycode_max = -1, i, j;
	int keysyms_per_keycode_return;

	/* A rebuilt map starts without entries from an earlier keyboard layout. */
	memset( fb_x11keycode_to_scancode, 0, sizeof(fb_x11keycode_to_scancode) );
	DisplayKeycodes( display, &keycode_min, &keycode_max );
	if( keycode_min < 0   ) keycode_min = 0;
	if( keycode_max > 255 ) keycode_max = 255;

	for( i = keycode_min; i <= keycode_max; i++ ) {
		KeySym *keysyms;
		KeySym keysym;
		keysyms_per_keycode_return = 0;
		/* The clamped loop index fits Xlib's one-byte KeyCode. */
		keysyms = GetKeyboardMapping( display, (KeyCode)i, 1, &keysyms_per_keycode_return );
		if( keysyms == NULL )
			continue;
		if( keysyms_per_keycode_return <= 0 ) {
			Free( keysyms );
			continue;
		}

		keysym = keysyms[0];
		if( keysym != NoSymbol ) {
			for( j = 0;
			     keysym_to_scancode[j].scancode &&
			         (keysym_to_scancode[j].keysym != keysym);
			     j++ )
				;
			/* The native scan-code table contains values from 0 through 127. */
			fb_x11keycode_to_scancode[i] = (unsigned char)keysym_to_scancode[j].scancode;
		}

		Free( keysyms );
	}
}

#endif

/* end of drivers/unix/scancodes_x11.c */
