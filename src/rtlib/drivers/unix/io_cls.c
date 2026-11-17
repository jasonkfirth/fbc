/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_cls.c
 * Purpose: console CLS statement.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

void fb_ConsoleClear( int mode )
{
	int start, end;
	size_t cells, width, row;

	if (!fb_console_context.inited || mode==1)
		return;

	/* The view helpers can query the console and acquire BG_LOCK themselves. */
	fb_ConsoleGetView(&start, &end);
	if ((mode != 2) && (mode != (int)0xFFFF0000)) {
		start = 1;
		end = fb_ConsoleGetMaxRow();
	}

	BG_LOCK();
	fb_hRecheckConsoleSize( TRUE );
	if( !fb_hConsoleCellCount( fb_console_context.w, fb_console_context.h, &cells ) ||
	    !fb_console_context.char_buffer || !fb_console_context.attr_buffer ) {
		BG_UNLOCK();
		return;
	}
	if( start < 1 )
		start = 1;
	if (start > fb_console_context.h)
		start = fb_console_context.h;
	if (end > fb_console_context.h)
		end = fb_console_context.h;
	if( end < start ) {
		BG_UNLOCK();
		return;
	}
	width = (size_t)fb_console_context.w;
	for( row = (size_t)(start - 1); row < (size_t)end; row++ ) {
		size_t offset = row * width;
		memset(fb_console_context.char_buffer + offset, ' ', width);
		memset(fb_console_context.attr_buffer + offset,
		       (unsigned char)(fb_console_context.fg_color | (fb_console_context.bg_color << 4)), width);
		fb_hTermOut(SEQ_LOCATE, 0, (int)row);
		fb_hTermOut(SEQ_CLEOL, 0, 0);
	}
	fb_hTermOut(SEQ_LOCATE, 0, start-1);
	fb_console_context.cur_y = start;
	fb_console_context.cur_x = 1;
	BG_UNLOCK();
}

/* end of drivers/unix/io_cls.c */
