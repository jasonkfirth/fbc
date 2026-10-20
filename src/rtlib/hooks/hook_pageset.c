/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_pageset.c
 * Purpose: 'screen , pg, pg' entrypoint, default to console mode.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_PageSet( int active, int visible )
{
	int res;

	fb_DevScrnInit_NoOpen( );

	FB_LOCK();

	if( __fb_ctx.hooks.pagesetproc ) {
		res = __fb_ctx.hooks.pagesetproc( active, visible );
	} else {
		if( (active >= FB_CONSOLE_MAXPAGES) || (visible >= FB_CONSOLE_MAXPAGES) ) {
			res = -1;
		} else {
			res = fb_ConsolePageSet( active, visible );
		}
	}

	FB_UNLOCK();

	return res;
}

/* end of hooks/hook_pageset.c */
