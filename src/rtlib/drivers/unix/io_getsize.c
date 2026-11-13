/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_getsize.c
 * Purpose: Provide io getsize runtime support.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

FBCALL void fb_ConsoleGetSize( int *cols, int *rows )
{
	if( !fb_console_context.inited ) {
		if( cols ) *cols = 80;
		if( rows ) *rows = 24;
		return;
	}

	BG_LOCK( );
	fb_hRecheckConsoleSize( TRUE );
	BG_UNLOCK( );

	if( cols ) *cols = fb_console_context.w;
	if( rows ) *rows = fb_console_context.h;
}

/* end of drivers/unix/io_getsize.c */
