/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_getsize.c
 * Purpose: getsize entrypoint, default to console mode.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_GetSize( int *cols, int *rows )
{
	FB_LOCK();

	if( __fb_ctx.hooks.getsizeproc )
		__fb_ctx.hooks.getsizeproc( cols, rows );
	else
		fb_ConsoleGetSize( cols, rows );

	FB_UNLOCK();
}

/* end of hooks/hook_getsize.c */
