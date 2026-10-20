/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_getx.c
 * Purpose: getx entrypoint, default to console mode.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL int fb_GetX( void )
{
	int res;

	FB_LOCK();

	if( __fb_ctx.hooks.getxproc )
		res = __fb_ctx.hooks.getxproc( );
	else
		res = fb_ConsoleGetX( );

	FB_UNLOCK();

	return res;
}

/* end of hooks/hook_getx.c */
