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
	int start, end, i;

	if (!fb_console_context.inited || mode==1)
		return;

	BG_LOCK( );
	fb_hRecheckConsoleSize( TRUE );
	BG_UNLOCK( );

	fb_ConsoleGetView(&start, &end);
	if ((mode != 2) && (mode != (int)0xFFFF0000)) {
		start = 1;
		end = fb_ConsoleGetMaxRow();
	}
	if (start > fb_console_context.h)
		start = fb_console_context.h;
	if (end > fb_console_context.h)
		end = fb_console_context.h;
	for (i = start; i <= end; i++) {
		memset(fb_console_context.char_buffer + ((i - 1) * fb_console_context.w), ' ', fb_console_context.w);
		memset(fb_console_context.attr_buffer + ((i - 1) * fb_console_context.w), fb_console_context.fg_color | (fb_console_context.bg_color << 4), fb_console_context.w);
		fb_hTermOut(SEQ_LOCATE, 0, i-1);
		fb_hTermOut(SEQ_CLEOL, 0, 0);
	}
	fb_hTermOut(SEQ_LOCATE, 0, start-1);
	fb_console_context.cur_y = start;
	fb_console_context.cur_x = 1;
}

/* end of drivers/unix/io_cls.c */
