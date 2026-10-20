/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/io_viewupdate.c
 * Purpose: view print update (console, no gfx).
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

void fb_ConsoleViewUpdate( void )
{
	int top = fb_ConsoleGetTopRow( );
	if( top >= 0 )
		++top;
	else
		top = 1;
	fb_ConsoleLocate( top, 1, -1 );
}

/* end of drivers/dos/io_viewupdate.c */
