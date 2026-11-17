/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_readstr.c
 * Purpose: input$|line input entrypoint, default to console mode.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <limits.h>

static int fb_hReadStringCapacity( ssize_t len )
{
	if( len <= 0 )
		return 0;

	return (len > INT_MAX) ? INT_MAX : (int)len;
}

char *fb_ReadString( char *buffer, ssize_t len, FILE *f )
{
	char *result;
	int input_capacity;

	if( f != stdin ) {
		input_capacity = fb_hReadStringCapacity( len );
		result = ((buffer == NULL) || (input_capacity == 0)) ?
		         NULL : fgets( buffer, input_capacity, f );
	} else {
		FB_LOCK( );
		if( fb_ctx.hooks.readstrproc )
			result = fb_ctx.hooks.readstrproc( buffer, len );
		else {
			input_capacity = fb_hReadStringCapacity( len );
			result = ((buffer == NULL) || (input_capacity == 0)) ?
			         NULL : fb_ConsoleReadStr( buffer, input_capacity );
		}
		FB_UNLOCK( );
	}

	return result;
}

/* end of hooks/hook_readstr.c */
