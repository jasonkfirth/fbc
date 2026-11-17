/*
 * Project: FreeBASIC Wii graphics input
 * File: gfx_wii_keyboard.c
 * Purpose: Translate Dolphin's IOS USB keyboard reports into FreeBASIC input.
 * Responsibilities: Bounded report polling, US ASCII, modifiers and key events.
 * This file does not render, replace controller mappings or generate input.
 *
 * Real IOS waits indefinitely for a keyboard event. Dolphin's implementation
 * returns immediately, so synchronous polling is enabled only after detecting
 * its private device. Physical USB keyboard support needs a qualified async
 * provider. The graphics owner serializes Init/Poll/Exit through its usual lock;
 * this module owns one descriptor and one complete aligned DMA cache line.
 */
#include "../../fb_gfx.h"
#include "gfx_wii_keyboard.h"
#include <ogc/ipc.h>

static s32 keyboard_descriptor = -1;
static unsigned char report[32] ATTRIBUTE_ALIGN(32);
static unsigned char held[128];
static int caps_lock;

static int is_modifier( int scancode )
{
	return scancode == SC_CONTROL || scancode == SC_LSHIFT ||
	       scancode == SC_RSHIFT || scancode == SC_ALT ||
	       scancode == SC_ALTGR || scancode == SC_LWIN || scancode == SC_RWIN;
}

static int key_scancode( unsigned char key )
{
	static const unsigned char letters[26] = {
		SC_A,SC_B,SC_C,SC_D,SC_E,SC_F,SC_G,SC_H,SC_I,SC_J,SC_K,SC_L,SC_M,
		SC_N,SC_O,SC_P,SC_Q,SC_R,SC_S,SC_T,SC_U,SC_V,SC_W,SC_X,SC_Y,SC_Z
	};
	if( key >= 4 && key <= 29 ) return letters[key - 4];
	if( key >= 0x1e && key <= 0x27 ) return SC_1 + key - 0x1e;
	if( key >= 0x3a && key <= 0x43 ) return SC_F1 + key - 0x3a;
	switch( key ) {
	case 0x28: case 0x58: return SC_ENTER;
	case 0x29: return SC_ESCAPE;
	case 0x2a: return SC_BACKSPACE;
	case 0x2b: return SC_TAB;
	case 0x2c: return SC_SPACE;
	case 0x2d: return SC_MINUS;
	case 0x2e: return SC_EQUALS;
	case 0x2f: return SC_LEFTBRACKET;
	case 0x30: return SC_RIGHTBRACKET;
	case 0x31: case 0x32: return SC_BACKSLASH;
	case 0x33: return SC_SEMICOLON;
	case 0x34: return SC_QUOTE;
	case 0x35: return SC_TILDE;
	case 0x36: return SC_COMMA;
	case 0x37: return SC_PERIOD;
	case 0x38: return SC_SLASH;
	case 0x39: return SC_CAPSLOCK;
	case 0x44: return SC_F11;
	case 0x45: return SC_F12;
	case 0x49: return SC_INSERT;
	case 0x4a: return SC_HOME;
	case 0x4b: return SC_PAGEUP;
	case 0x4c: return SC_DELETE;
	case 0x4d: return SC_END;
	case 0x4e: return SC_PAGEDOWN;
	case 0x4f: return SC_RIGHT;
	case 0x50: return SC_LEFT;
	case 0x51: return SC_DOWN;
	case 0x52: return SC_UP;
	default: return 0;
	}
}

static int key_ascii( unsigned char key, int shifted )
{
	static const char digits[] = "1234567890";
	static const char shifted_digits[] = "!@#$%^&*()";
	if( key >= 4 && key <= 29 )
		return ((shifted != caps_lock) ? 'A' : 'a') + key - 4;
	if( key >= 0x1e && key <= 0x27 )
		return shifted ? shifted_digits[key - 0x1e] : digits[key - 0x1e];
	switch( key ) {
	case 0x28: case 0x58: return 13;
	case 0x29: return 27;
	case 0x2a: return 8;
	case 0x2b: return 9;
	case 0x2c: return ' ';
	case 0x2d: return shifted ? '_' : '-';
	case 0x2e: return shifted ? '+' : '=';
	case 0x2f: return shifted ? '{' : '[';
	case 0x30: return shifted ? '}' : ']';
	case 0x31: case 0x32: return shifted ? '|' : '\\';
	case 0x33: return shifted ? ':' : ';';
	case 0x34: return shifted ? '"' : '\'';
	case 0x35: return shifted ? '~' : 96; /* ASCII grave accent */
	case 0x36: return shifted ? '<' : ',';
	case 0x37: return shifted ? '>' : '.';
	case 0x38: return shifted ? '?' : '/';
	default: return 0;
	}
}

static void post_key( int scancode, int down, int ascii )
{
	EVENT event;
	int legacy;
	memset( &event, 0, sizeof( event ) );
	event.type = down ? EVENT_KEY_PRESS : EVENT_KEY_RELEASE;
	event.scancode = scancode;
	event.ascii = down ? ascii : 0;
	if( fb_gfx && fb_gfx->key )
		fb_gfx->key[scancode] = down ? TRUE : FALSE;
	fb_hPostEvent( &event );
	if( down && !is_modifier( scancode ) && scancode != SC_CAPSLOCK ) {
		legacy = ascii ? ascii : fb_hScancodeToExtendedKey( scancode );
		if( legacy ) fb_hPostKey( legacy );
	}
}

int fb_hWiiKeyboardDown( int scancode )
{
	return scancode > 0 && scancode < 128 ? held[scancode] : 0;
}

void fb_hWiiKeyboardExit( void )
{
	if( keyboard_descriptor >= 0 ) IOS_Close( keyboard_descriptor );
	keyboard_descriptor = -1;
	memset( held, 0, sizeof( held ) );
	caps_lock = 0;
}

void fb_hWiiKeyboardInit( void )
{
	s32 dolphin;
	fb_hWiiKeyboardExit();
	dolphin = IOS_Open( "/dev/dolphin", IPC_OPEN_NONE );
	if( dolphin < 0 ) return;
	IOS_Close( dolphin );
	keyboard_descriptor = IOS_Open( "/dev/usb/kbd", IPC_OPEN_NONE );
}

void fb_hWiiKeyboardPoll( void )
{
	int budget, index, scancode, phase;
	unsigned char next[128];
	int ascii[128];
	u32 type;
	unsigned char modifiers, key;
	if( keyboard_descriptor < 0 || !fb_gfx || !fb_gfx->key ) return;
	/* Packet: BE type at0, identifier at4, modifiers at8, reserved at9,
	 * six USB HID keyboard usages at10. Keep the DMA allocation32-byte aligned.
	 * At most eight queued reports are consumed before returning to drawing. */
	for( budget = 0; budget < 8; budget++ ) {
		memset( report, 0xff, sizeof( report ) );
		if( IOS_Ioctl( keyboard_descriptor, 0, NULL, 0, report, 16 ) < 0 ) {
			for( scancode = 1; scancode < 128; scancode++ )
				if( held[scancode] ) post_key( scancode, FALSE, 0 );
			fb_hWiiKeyboardExit();
			return;
		}
		type = ((u32)report[0] << 24) | ((u32)report[1] << 16) |
		       ((u32)report[2] << 8) | report[3];
		if( type > 2 ) break; /* No queued report leaves the sentinel intact. */
		memset( next, 0, sizeof( next ) );
		memset( ascii, 0, sizeof( ascii ) );
		if( type == 2 ) {
			modifiers = report[8];
			next[SC_CONTROL] = (modifiers & 0x11) != 0;
			next[SC_LSHIFT] = (modifiers & 0x02) != 0;
			next[SC_RSHIFT] = (modifiers & 0x20) != 0;
			next[SC_ALT] = (modifiers & 0x04) != 0;
			next[SC_ALTGR] = (modifiers & 0x40) != 0;
			next[SC_LWIN] = (modifiers & 0x08) != 0;
			next[SC_RWIN] = (modifiers & 0x80) != 0;
			for( index = 10; index < 16; index++ ) {
				key = report[index];
				if( key >= 1 && key <= 3 ) break; /* HID rollover/error. */
				scancode = key_scancode( key );
				if( scancode ) next[scancode] = 1;
			}
			if( index != 16 ) continue;
			if( next[SC_CAPSLOCK] && !held[SC_CAPSLOCK] ) caps_lock = !caps_lock;
			for( index = 10; index < 16; index++ ) {
				key = report[index];
				scancode = key_scancode( key );
				if( scancode && !(modifiers & 0x11) )
					ascii[scancode] = key_ascii( key, (modifiers & 0x22) != 0 );
			}
		}
		/* Release keys before modifiers, and press modifiers before keys. */
		for( phase = 0; phase < 4; phase++ )
			for( scancode = 1; scancode < 128; scancode++ ) {
				int modifier = is_modifier( scancode );
				if( phase == 0 && !modifier && held[scancode] && !next[scancode] )
					post_key( scancode, FALSE, 0 );
				if( phase == 1 && modifier && held[scancode] && !next[scancode] )
					post_key( scancode, FALSE, 0 );
				if( phase == 2 && modifier && !held[scancode] && next[scancode] )
					post_key( scancode, TRUE, 0 );
				if( phase == 3 && !modifier && !held[scancode] && next[scancode] )
					post_key( scancode, TRUE, ascii[scancode] );
			}
		memcpy( held, next, sizeof( held ) );
	}
}
/* end of gfx_wii_keyboard.c */
