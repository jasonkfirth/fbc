/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_gety.c
 * Purpose: gety entrypoint, default to console mode.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL int fb_GetY( void )
{
	int res;

	FB_LOCK();

	if( fb_ctx.hooks.getyproc )
		res = fb_ctx.hooks.getyproc( );
	else
		res = fb_ConsoleGetY( );

	FB_UNLOCK();

	return res;
}

/* end of hooks/hook_gety.c */
