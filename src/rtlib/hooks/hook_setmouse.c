/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_setmouse.c
 * Purpose: Provide hook setmouse runtime support.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_SetMouse( int x, int y, int cursor, int clip )
{
	int res;

	FB_LOCK();

	if( fb_ctx.hooks.getmouseproc )
		res = fb_ctx.hooks.setmouseproc( x, y, cursor, clip );
	else
		res = fb_ConsoleSetMouse( x, y, cursor, clip );

	FB_UNLOCK();

	return res;
}

/* end of hooks/hook_setmouse.c */
