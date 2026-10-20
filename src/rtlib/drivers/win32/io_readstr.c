/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_readstr.c
 * Purpose: console line input function.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

char *fb_ConsoleReadStr( char *buffer, ssize_t len )
{
	char *res;

	fb_hRestoreConsoleWindow( );
	FB_CON_CORRECT_POSITION();
	fb_hConsolePutBackEvents( );

	res = fgets( buffer, len, stdin );

	fb_hUpdateConsoleWindow( );

	return res;
}

/* end of drivers/win32/io_readstr.c */
