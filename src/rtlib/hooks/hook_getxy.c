/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_getxy.c
 * Purpose: getxy entrypoint, default to console mode.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_GetXY( int *col, int *row )
{
	FB_LOCK();

	if( fb_ctx.hooks.getxyproc )
		fb_ctx.hooks.getxyproc( col, row );
	else
		fb_ConsoleGetXY( col, row );

	FB_UNLOCK();
}

/* end of hooks/hook_getxy.c */
