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

	if( !fb_console_context.inited )
		return 0;

	BG_LOCK( );
	fb_hRecheckConsoleSize( TRUE );
	BG_UNLOCK( );

	if ((x < 1) || (x > fb_console_context.w) || (y < 1) || (y > fb_console_context.h))
		return 0;

	if (colorflag)
		buffer = fb_console_context.attr_buffer;
	else
		buffer = fb_console_context.char_buffer;

	return (unsigned int)buffer[((y - 1) * fb_console_context.w) + x - 1];
}

/* end of drivers/unix/io_readxy.c */
