/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_viewupdate.c
 * Purpose: view print update (console, no gfx).
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

void fb_ConsoleViewUpdate( void )
{
	if (!__fb_con.inited)
		return;
	__fb_con.scroll_region_changed = TRUE;
	fb_hTermOut(SEQ_SCROLL_REGION, fb_ConsoleGetBotRow(), fb_ConsoleGetTopRow());
}

/* end of drivers/unix/io_viewupdate.c */
