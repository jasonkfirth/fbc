/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_width.c
 * Purpose: console width() function.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsoleWidth( int cols, int rows )
{
	if( !fb_console_context.inited )
		return (80 | (25 << 16));

	BG_LOCK( );
	fb_hRecheckConsoleSize( TRUE );
	BG_UNLOCK( );

	int cur = fb_console_context.w | (fb_console_context.h << 16);

	if ((cols > 0) || (rows > 0)) {
		BG_LOCK();

		if (cols <= 0)
			cols = fb_console_context.w;
		if (rows <= 0)
			rows = fb_console_context.h;
		fb_hTermOut(SEQ_WINDOW_SIZE, rows, cols);

		BG_UNLOCK();

		fb_ConsoleClear( 0 );
	}

	return cur;
}

/* end of drivers/unix/io_width.c */
