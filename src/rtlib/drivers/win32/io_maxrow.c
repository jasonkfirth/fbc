/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_maxrow.c
 * Purpose: Provide io maxrow runtime support.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsoleGetMaxRow( void )
{
	COORD max = GetLargestConsoleWindowSize( __fb_out_handle );
	return (max.Y == 0) ? FB_SCRN_DEFAULT_HEIGHT : max.Y + 1;
}

/* end of drivers/win32/io_maxrow.c */
