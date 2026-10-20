/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_isredir.c
 * Purpose: input/ouput redirection check.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_IsRedirected ( int is_input )
{
	int result;

	FB_LOCK( );

	if( __fb_ctx.hooks.isredirproc != NULL )
		result = __fb_ctx.hooks.isredirproc( is_input );
	else
		result = fb_ConsoleIsRedirected( is_input );

	FB_UNLOCK( );

	return result;
}

/* end of hooks/hook_isredir.c */
