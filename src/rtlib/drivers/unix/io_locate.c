/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_locate.c
 * Purpose: console LOCATE statement.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsoleLocate( int row, int col, int cursor )
{
	int x, y;
	static int visible = 0x10000;

	if (!fb_console_context.inited)
		return 0;

	if ((row <= 0) || (col <= 0))
		fb_ConsoleGetXY(&x, &y);

	BG_LOCK();

	if (col > 0)
		x = col;
	if (row > 0)
		y = row;

	fb_hRecheckConsoleSize( TRUE );

	if (x <= fb_console_context.w)
		fb_console_context.cur_x = x;
	else
		fb_console_context.cur_x = fb_console_context.w;
	if (y <= fb_console_context.h)
		fb_console_context.cur_y = y;
	else
		fb_console_context.cur_y = fb_console_context.h;
	fb_hTermOut(SEQ_LOCATE, x-1, y-1);
	if (cursor == 0) {
		fb_hTermOut(SEQ_HIDE_CURSOR, 0, 0);
		visible = 0;
	}
	else if (cursor == 1) {
		fb_hTermOut(SEQ_SHOW_CURSOR, 0, 0);
		visible = 0x10000;
	}

	BG_UNLOCK();

	return (x & 0xFF) | ((y & 0xFF) << 8) | visible;
}

/* end of drivers/unix/io_locate.c */
