/*
 * FreeBASIC gfxlib2: gfx_inkey.c
 * Purpose: Expose graphics keyboard input through runtime hooks.
 * Responsibilities: input state, event delivery and runtime input hooks.
 * This file intentionally does NOT contain pixel rasterization or mode allocation.
 */

/* inkey$ handling */

#include "../fb_gfx.h"

/* An IME commits a word in one callback, before BASIC can drain INKEY. */
#if defined(HOST_ANDROID) || defined(HOST_JS) || defined(HOST_LINUX)
#define KEY_BUFFER_LEN		256
#else
#define KEY_BUFFER_LEN		16
#endif

static int key_buffer[KEY_BUFFER_LEN], key_head = 0, key_tail = 0;
static int key_buffer_changed = FALSE;

static void lock_key_buffer(void)
{
	if ((__fb_gfx) && (__fb_gfx->event_mutex))
		fb_MutexLock(__fb_gfx->event_mutex);
}

static void unlock_key_buffer(void)
{
	if ((__fb_gfx) && (__fb_gfx->event_mutex))
		fb_MutexUnlock(__fb_gfx->event_mutex);
}

static void poll_events(void)
{
	if ((__fb_gfx) && (__fb_gfx->driver->poll_events))
		__fb_gfx->driver->poll_events();
}

int fb_hGfxInputBufferChanged( void )
{
	int res;

	FB_GRAPHICS_LOCK( );
	lock_key_buffer();

    res = key_buffer_changed;
    key_buffer_changed = FALSE;

	unlock_key_buffer();
	FB_GRAPHICS_UNLOCK( );

	return res;
}

/* Drivers post keys without waiting for the display to finish presenting. */
void fb_hPostKey(int key)
{
	lock_key_buffer();
	key_buffer[key_tail] = key;
	if (((key_tail + 1) & (KEY_BUFFER_LEN - 1)) == key_head)
		key_head = (key_head + 1) & (KEY_BUFFER_LEN - 1);
    key_tail = (key_tail + 1) & (KEY_BUFFER_LEN - 1);
    key_buffer_changed = TRUE;
	unlock_key_buffer();
}

static int get_key(void)
{
	int key = 0;

	lock_key_buffer();

	if (key_head != key_tail) {
		key = key_buffer[key_head];
		key_head = (key_head + 1) & (KEY_BUFFER_LEN - 1);
	}
	/* INKEY and INPUT SLEEP share the pending-key notification state. */
	key_buffer_changed = FALSE;
	unlock_key_buffer();

	return key;
}

/* Doing synchronization manually here because getkey() is blocking */
int fb_GfxGetkey(void)
{
	int key = 0;

	/* Poll until next key press */
	while( 1 ) {
		FB_GRAPHICS_LOCK( );

		/* Abort if gfx screen was closed */
		if (!__fb_gfx) {
			FB_GRAPHICS_UNLOCK( );
			break;
		}

		poll_events( );
		key = get_key( );

		FB_GRAPHICS_UNLOCK( );

		if( key != 0 ) {
			break;
		}

		fb_Sleep(20);
	}

	return key;
}

/* Caller is expected to hold FB_LOCK() */
int fb_GfxKeyHit(void)
{
	int res;

	FB_GRAPHICS_LOCK( );

	poll_events( );

	lock_key_buffer();

	res = (key_head != key_tail? 1: 0);

	unlock_key_buffer();
	FB_GRAPHICS_UNLOCK( );

	return res;
}

/* Caller is expected to hold FB_LOCK() */
FBSTRING_ACCESS *fb_GfxInkey(void)
{
	FBSTRING_ACCESS *res;
	int ch;

	FB_GRAPHICS_LOCK( );

	poll_events( );

	if (__fb_gfx && (ch = get_key())) {
		res = fb_hMakeInkeyStr( ch );
	} else {
		res = &fb_ctx.null_desc;
	}

	FB_GRAPHICS_UNLOCK( );

	return res;
}

int fb_GfxIsRedir(int is_input)
{
	(void)is_input;

	return FB_FALSE;
}

/* end of gfx_inkey.c */
