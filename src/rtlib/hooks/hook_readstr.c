/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_readstr.c
 * Purpose: input$|line input entrypoint, default to console mode.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

char *fb_ReadString( char *buffer, ssize_t len, FILE *f )
{
	char *result;

	if( f != stdin ) {
		result = fgets( buffer, len, f );
	} else {
		FB_LOCK( );
		if( __fb_ctx.hooks.readstrproc )
			result = __fb_ctx.hooks.readstrproc( buffer, len );
		else
			result = fb_ConsoleReadStr( buffer, len );
		FB_UNLOCK( );
	}

	return result;
}

/* end of hooks/hook_readstr.c */
