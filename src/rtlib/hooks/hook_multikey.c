/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_multikey.c
 * Purpose: multikey entrypoint, default to console mode.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL int fb_Multikey( int scancode )
{
	int res;
	
	FB_LOCK();
	
	if( __fb_ctx.hooks.multikeyproc )
		res = __fb_ctx.hooks.multikeyproc( scancode );
	else
		res = fb_ConsoleMultikey( scancode );

	FB_UNLOCK();
	
	return res;
}

/* end of hooks/hook_multikey.c */
