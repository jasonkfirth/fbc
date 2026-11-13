/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_color.c
 * Purpose: console COLOR statement.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

unsigned int fb_ConsoleColor( unsigned int fc, unsigned int bc, int flags )
{
	const char map[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
	unsigned int old_fg = fb_console_context.fg_color;
	unsigned int old_bg = fb_console_context.bg_color;
	int force = FALSE;

	if (!fb_console_context.inited)
		return old_fg | (old_bg << 16);

	if (!(flags & FB_COLOR_FG_DEFAULT))
		fb_console_context.fg_color = (fc & 0xF);
	if (!(flags & FB_COLOR_BG_DEFAULT))
		fb_console_context.bg_color = (bc & 0xF);

	if ((fb_console_context.inited == INIT_CONSOLE) || (fb_console_context.term_type != TERM_XTERM)) {
		/* console and any terminal but xterm do not support extended color attributes and only allow 16+8 colors */
		if (fb_console_context.fg_color != old_fg) {
			if ((fb_console_context.fg_color ^ old_fg) & 0x8) {
				/* bright mode changed: reset attributes and force setting both back and fore colors */
				fb_hTermOut(SEQ_RESET_COLOR, 0, 0);
				if (fb_console_context.fg_color & 0x8)
					fb_hTermOut(SEQ_BRIGHT_COLOR, 0, 0);
				force = TRUE;
			}
			fb_hTermOut(SEQ_FG_COLOR, 0, map[fb_console_context.fg_color & 0x7]);
		}
		if ((fb_console_context.bg_color != old_bg) || (force))
			fb_hTermOut(SEQ_BG_COLOR, 0, map[fb_console_context.bg_color & 0x7]);
	} else {
		/* generic xterm supports 16+16 colors */
		if (fb_console_context.fg_color != old_fg)
			fb_hTermOut(SEQ_SET_COLOR_EX, map[fb_console_context.fg_color & 0x7] + (fb_console_context.fg_color & 0x8 ? 90 : 30), 0);
		if (fb_console_context.bg_color != old_bg)
			fb_hTermOut(SEQ_SET_COLOR_EX, map[fb_console_context.bg_color & 0x7] + (fb_console_context.bg_color & 0x8 ? 100 : 40), 0);
	}

	return old_fg | (old_bg << 16);
}

unsigned int fb_ConsoleGetColorAtt( void )
{
	return fb_console_context.inited ? (fb_console_context.fg_color | (fb_console_context.bg_color << 4)) : 0x7;
}

/* end of drivers/unix/io_color.c */
