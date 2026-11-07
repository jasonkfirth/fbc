/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_cls.c
 * Purpose: cls entrypoint, default to console mode.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_Cls( int mode )
{

	fb_DevScrnInit_NoOpen( );

	FB_LOCK();

	if( fb_ctx.hooks.clsproc )
		fb_ctx.hooks.clsproc( mode );
	else
        fb_ConsoleClear( mode );

    FB_HANDLE_SCREEN->line_length = 0;

	FB_UNLOCK();

}

/* end of hooks/hook_cls.c */
