/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_getmouse.c
 * Purpose: Provide hook getmouse runtime support.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_GetMouse( int *x, int *y, int *z, int *buttons, int *clip )
{
	int res;

	FB_LOCK();

	if( __fb_ctx.hooks.getmouseproc )
		res = __fb_ctx.hooks.getmouseproc( x, y, z, buttons, clip );
	else
		res = fb_ConsoleGetMouse( x, y, z, buttons, clip );

	FB_UNLOCK();

	return res;
}

/* end of hooks/hook_getmouse.c */
