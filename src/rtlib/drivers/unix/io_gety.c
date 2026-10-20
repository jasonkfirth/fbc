/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_gety.c
 * Purpose: Provide io gety runtime support.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsoleGetY( void )
{
	int y;
	fb_ConsoleGetXY( NULL, &y );
	return y;
}

/* end of drivers/unix/io_gety.c */
