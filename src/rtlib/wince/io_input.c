/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_input.c

    Purpose:

        Provide keyboard input for the Windows CE logical console.

    Responsibilities:

        - pump the current thread's Windows CE message queue
        - detect newly pressed virtual keys without desktop console events
        - translate common keys to FreeBASIC/QB-compatible key codes
        - retain pending input in a bounded process-local queue

    This file intentionally does NOT contain:

        - desktop console input records
        - text-editor or input-method integration
        - graphics-window mouse handling
*/

#include "../fb.h"
#include "fb_private_console.h"

#include <ctype.h>
#include <windows.h>

#define FB_WINCE_KEY_COUNT 256
#define FB_WINCE_KEY_QUEUE_LENGTH 64

/*
    CeGCC exposes MapVirtualKeyW but its Windows CE headers omit the operation
    constants.  Operation 2 asks Windows to translate a virtual-key code to a
    character value, matching MAPVK_VK_TO_CHAR in the desktop SDK.
*/
#define FB_WINCE_MAPVK_VK_TO_CHAR 2u

static unsigned char key_was_down[FB_WINCE_KEY_COUNT];
static int key_queue[FB_WINCE_KEY_QUEUE_LENGTH];
static unsigned int key_head;
static unsigned int key_tail;

static void hQueueKey( int key )
{
	unsigned int next_tail;

	if( key < 0 )
		return;

	next_tail = (key_tail + 1) % FB_WINCE_KEY_QUEUE_LENGTH;
	if( next_tail == key_head )
		key_head = (key_head + 1) % FB_WINCE_KEY_QUEUE_LENGTH;

	key_queue[key_tail] = key;
	key_tail = next_tail;
}

static int hExtendedKey( int virtual_key )
{
	switch( virtual_key ) {
	case VK_F1:     return FB_MAKE_EXT_KEY( SC_F1 );
	case VK_F2:     return FB_MAKE_EXT_KEY( SC_F2 );
	case VK_F3:     return FB_MAKE_EXT_KEY( SC_F3 );
	case VK_F4:     return FB_MAKE_EXT_KEY( SC_F4 );
	case VK_F5:     return FB_MAKE_EXT_KEY( SC_F5 );
	case VK_F6:     return FB_MAKE_EXT_KEY( SC_F6 );
	case VK_F7:     return FB_MAKE_EXT_KEY( SC_F7 );
	case VK_F8:     return FB_MAKE_EXT_KEY( SC_F8 );
	case VK_F9:     return FB_MAKE_EXT_KEY( SC_F9 );
	case VK_F10:    return FB_MAKE_EXT_KEY( SC_F10 );
	case VK_F11:    return FB_MAKE_EXT_KEY( SC_F11 );
	case VK_F12:    return FB_MAKE_EXT_KEY( SC_F12 );
	case VK_HOME:   return FB_MAKE_EXT_KEY( SC_HOME );
	case VK_UP:     return FB_MAKE_EXT_KEY( SC_UP );
	case VK_PRIOR:  return FB_MAKE_EXT_KEY( SC_PAGEUP );
	case VK_LEFT:   return FB_MAKE_EXT_KEY( SC_LEFT );
	case VK_RIGHT:  return FB_MAKE_EXT_KEY( SC_RIGHT );
	case VK_END:    return FB_MAKE_EXT_KEY( SC_END );
	case VK_DOWN:   return FB_MAKE_EXT_KEY( SC_DOWN );
	case VK_NEXT:   return FB_MAKE_EXT_KEY( SC_PAGEDOWN );
	case VK_INSERT: return FB_MAKE_EXT_KEY( SC_INSERT );
	case VK_DELETE: return FB_MAKE_EXT_KEY( SC_DELETE );
	default:        return -1;
	}
}

static int hVirtualKeyToKey( int virtual_key )
{
	static const char shifted_digits[] = ")!@#$%^&*(";
	int shift_down;
	int caps_enabled;
	unsigned int character;

	switch( virtual_key ) {
	case VK_BACK:   return '\b';
	case VK_TAB:    return '\t';
	case VK_RETURN: return '\r';
	case VK_ESCAPE: return 27;
	case VK_SPACE:  return ' ';
	default:
		break;
	}

	character = MapVirtualKeyW( (UINT)virtual_key,
	                            FB_WINCE_MAPVK_VK_TO_CHAR );
	character &= 0xFFFFu;
	if( character == 0 )
		return hExtendedKey( virtual_key );

	shift_down = ((GetAsyncKeyState( VK_SHIFT ) & 0x8000) != 0);
	caps_enabled = ((GetKeyState( VK_CAPITAL ) & 1) != 0);

	if( (virtual_key >= 'A') && (virtual_key <= 'Z') ) {
		if( !(shift_down ^ caps_enabled) )
			character = (unsigned int)tolower( (int)character );
	} else if( shift_down && (virtual_key >= '0') &&
	           (virtual_key <= '9') ) {
		character = (unsigned int)shifted_digits[virtual_key - '0'];
	}

	return (character <= 255u) ? (int)character : -1;
}

static void hPollKeys( void )
{
	int virtual_key;

	for( virtual_key = 1; virtual_key < FB_WINCE_KEY_COUNT;
	     ++virtual_key ) {
		int is_down =
			((GetAsyncKeyState( virtual_key ) & 0x8000) != 0);

		if( is_down && !key_was_down[virtual_key] )
			hQueueKey( hVirtualKeyToKey( virtual_key ) );

		key_was_down[virtual_key] = (unsigned char)is_down;
	}
}

int fb_ConsoleProcessEvents( void )
{
	MSG message;
	int processed = FALSE;

	while( PeekMessageW( &message, NULL, 0, 0, PM_REMOVE ) ) {
		if( message.message == WM_QUIT )
			hQueueKey( KEY_QUIT );
		else {
			TranslateMessage( &message );
			DispatchMessageW( &message );
		}
		processed = TRUE;
	}

	hPollKeys();
	return processed;
}

int fb_hConsoleInputBufferChanged( void )
{
	fb_ConsoleProcessEvents();
	return (key_head != key_tail);
}

static int hGetQueuedKey( int full, int remove )
{
	int key;

	fb_ConsoleProcessEvents();
	if( key_head == key_tail )
		return -1;

	key = key_queue[key_head];
	if( !full && (key > 255) ) {
		key_queue[key_head] = (key >> 8) & 0xFF;
		return (unsigned char)FB_EXT_CHAR;
	}

	if( remove )
		key_head = (key_head + 1) % FB_WINCE_KEY_QUEUE_LENGTH;

	return key;
}

int fb_hConsoleGetKey( int full )
{
	return hGetQueuedKey( full, TRUE );
}

int fb_hConsolePeekKey( int full )
{
	return hGetQueuedKey( full, FALSE );
}

void fb_hConsolePutBackEvents( void )
{
}

int fb_ConsoleHasFocus( void )
{
	return TRUE;
}

int fb_hConsoleTranslateKey( char ascii_character, WORD scan_code,
	                         WORD virtual_key, DWORD control_state,
	                         int enhanced_only )
{
	(void)scan_code;
	(void)control_state;

	if( !enhanced_only && (ascii_character != '\0') )
		return (unsigned char)ascii_character;

	return hExtendedKey( virtual_key );
}

/* end of wince/io_input.c */
