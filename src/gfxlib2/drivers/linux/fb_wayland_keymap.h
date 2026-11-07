/*
 * Project: FreeBASIC Linux Wayland support
 * File: fb_wayland_keymap.h
 * Purpose: Map XKB control symbols to FreeBASIC scan codes.
 * Responsibilities: Share special-key handling between both gfx libraries.
 * This file intentionally does NOT load a keymap or deliver input events.
 */
#ifndef FB_WAYLAND_KEYMAP_H
#define FB_WAYLAND_KEYMAP_H

#include "../../../rtlib/fb.h"

/*
 * Virtual keyboards may assign Return or another control symbol to any
 * evdev code. Use the compositor's XKB symbol for these keys, while retaining
 * physical scan codes for ordinary character and modifier keys. The values
 * below are the stable XKB keysyms from keysymdef.h.
 */
static inline int fb_hWaylandControlScancode(uint32_t symbol, int fallback)
{
	switch (symbol) {
	case 0xFF08: return SC_BACKSPACE;
	case 0xFF09: case 0xFE20: return SC_TAB;
	case 0xFF0D: case 0xFF8D: return SC_ENTER;
	case 0xFF1B: return SC_ESCAPE;
	case 0xFF50: case 0xFF95: return SC_HOME;
	case 0xFF51: case 0xFF96: return SC_LEFT;
	case 0xFF52: case 0xFF97: return SC_UP;
	case 0xFF53: case 0xFF98: return SC_RIGHT;
	case 0xFF54: case 0xFF99: return SC_DOWN;
	case 0xFF55: case 0xFF9A: return SC_PAGEUP;
	case 0xFF56: case 0xFF9B: return SC_PAGEDOWN;
	case 0xFF57: case 0xFF9C: return SC_END;
	case 0xFF63: case 0xFF9E: return SC_INSERT;
	case 0xFFFF: case 0xFF9F: return SC_DELETE;
	default: return fallback;
	}
}

#endif
/* end of fb_wayland_keymap.h */
