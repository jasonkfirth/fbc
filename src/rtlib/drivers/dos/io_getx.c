/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/io_getx.c
 * Purpose: Provide io getx runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsoleGetX( void )
{
	int x;
	fb_ConsoleGetXY( &x, NULL );
	return x;
}

/* end of drivers/dos/io_getx.c */
