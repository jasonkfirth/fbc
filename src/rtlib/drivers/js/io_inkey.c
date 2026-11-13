/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_inkey.c
 * Purpose: console INKEY() function.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

/* Caller is expected to hold FB_LOCK() */
FBSTRING_ACCESS *fb_ConsoleInkey( void )
{
	FBSTRING_ACCESS *res = &fb_ctx.null_desc;

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
	if( fb_console_context.key_head == fb_console_context.key_tail)
        return 0;

	int key = fb_console_context.key_buffer[fb_console_context.key_head];
	fb_console_context.key_head = (fb_console_context.key_head + 1) % KEY_BUFFER_LEN;

	return key;
}

/* Caller is expected to hold FB_LOCK() */
int fb_ConsoleKeyHit( void )
{
	return fb_console_context.key_head != fb_console_context.key_tail;
}

/* end of drivers/js/io_inkey.c */
