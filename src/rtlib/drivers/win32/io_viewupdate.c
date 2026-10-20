/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_viewupdate.c
 * Purpose: view print update (console, no gfx).
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

void fb_ConsoleViewUpdate( void )
{
	fb_hUpdateConsoleWindow( );
}

/* end of drivers/win32/io_viewupdate.c */
