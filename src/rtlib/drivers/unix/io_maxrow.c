/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_maxrow.c
 * Purpose: Provide io maxrow runtime support.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsoleGetMaxRow( void )
{
	if( !__fb_con.inited )
		return 24;

	BG_LOCK( );
	fb_hRecheckConsoleSize( TRUE );
	BG_UNLOCK( );
	return __fb_con.h;
}

/* end of drivers/unix/io_maxrow.c */
