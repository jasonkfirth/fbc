/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_readxy.c
 * Purpose: console SCREEN() function (character/color query).
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

FBCALL unsigned int fb_ConsoleReadXY( int x, int y, int colorflag )
{
	unsigned char *buffer;

	if( !__fb_con.inited )
		return 0;

	BG_LOCK( );
	fb_hRecheckConsoleSize( TRUE );
	BG_UNLOCK( );

	if ((x < 1) || (x > __fb_con.w) || (y < 1) || (y > __fb_con.h))
		return 0;

	if (colorflag)
		buffer = __fb_con.attr_buffer;
	else
		buffer = __fb_con.char_buffer;

	return (unsigned int)buffer[((y - 1) * __fb_con.w) + x - 1];
}

/* end of drivers/unix/io_readxy.c */
