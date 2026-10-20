/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_color.c
 * Purpose: color entrypoint, default to console mode.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL unsigned int fb_Color( unsigned int fc, unsigned int bc, int flags )
{
	unsigned int cur;

	FB_LOCK();

	if( __fb_ctx.hooks.colorproc )
		cur = __fb_ctx.hooks.colorproc( fc, bc, flags );
	else
		cur = fb_ConsoleColor( fc, bc, flags );

	FB_UNLOCK();

	return cur;
}

/* end of hooks/hook_color.c */
