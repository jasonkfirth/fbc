/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_isredir.c
 * Purpose: Provide io isredir runtime support.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsoleIsRedirected( int is_input )
{
	DWORD mode;
	return (GetConsoleMode( (is_input ? __fb_in_handle : __fb_out_handle), &mode ) == 0) ? FB_TRUE : FB_FALSE;
}

/* end of drivers/win32/io_isredir.c */
