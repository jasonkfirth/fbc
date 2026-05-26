/* console INKEY() function */

#include "../fb.h"
#include "fb_private_console.h"

/* Caller is expected to hold FB_LOCK() */
FBSTRING *fb_ConsoleInkey( void )
{
	FBSTRING *res = &__fb_ctx.null_desc;

	/*
	 * Browser-hosted programs often poll INKEY in loops that relied on the
	 * native OS scheduler for preemption.  Yield here so pending browser
	 * keyboard, timer, and canvas callbacks can run before the poll result is
	 * reported.
	 */
	fb_Delay( 0 );

	if( fb_ConsoleKeyHit( ) != 0 )
	{
        res = fb_hMakeInkeyStr( fb_ConsoleGetkey( ) );
	}

	return res;
}

int fb_ConsoleGetkey( void )
{
    /*
    	Do not block the JavaScript event loop.  Keyboard events are delivered
    	asynchronously, so an empty buffer means no key is currently available.
    */
	if( __fb_con.key_head == __fb_con.key_tail)
        return 0;

	int key = __fb_con.key_buffer[__fb_con.key_head];
	__fb_con.key_head = (__fb_con.key_head + 1) % KEY_BUFFER_LEN;

	return key;
}

/* Caller is expected to hold FB_LOCK() */
int fb_ConsoleKeyHit( void )
{
	return __fb_con.key_head != __fb_con.key_tail;
}
