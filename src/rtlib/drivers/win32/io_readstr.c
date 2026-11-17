/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/io_readstr.c
 * Purpose: console line input function.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"
#include <limits.h>

char *fb_ConsoleReadStr( char *buffer, ssize_t len )
{
	char *res;
	int input_capacity;

	fb_hRestoreConsoleWindow( );
	FB_CON_CORRECT_POSITION();
	fb_hConsolePutBackEvents( );

	if( (buffer == NULL) || (len <= 0) ) {
		res = NULL;
	} else {
		input_capacity = (len > INT_MAX) ? INT_MAX : (int)len;
		res = fgets( buffer, input_capacity, stdin );
	}

	fb_hUpdateConsoleWindow( );

	return res;
}

/* end of drivers/win32/io_readstr.c */
