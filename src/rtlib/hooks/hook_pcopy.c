/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_pcopy.c
 * Purpose: pcopy entrypoint, default to console mode.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_PageCopy( int src, int dst )
{
	fb_DevScrnInit_NoOpen( );

	FB_LOCK();

	int res;

	if( __fb_ctx.hooks.pagecopyproc )
		res = __fb_ctx.hooks.pagecopyproc( src, dst );
	else
	{
		if( (src >= FB_CONSOLE_MAXPAGES) || (dst >= FB_CONSOLE_MAXPAGES) )
			return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

		res = fb_ConsolePageCopy( src, dst );
	}

	FB_UNLOCK();

	return res;
}

/* end of hooks/hook_pcopy.c */
