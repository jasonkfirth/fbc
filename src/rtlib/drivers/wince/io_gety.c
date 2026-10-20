/*
 * Project: FreeBASIC runtime library
 * File: drivers/wince/io_gety.c
 * Purpose: Provide io gety runtime support.
 * Responsibilities: Target-specific runtime support for wince.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
    FreeBASIC Runtime Library
    -------------------------

    File: drivers/wince/io_gety.c

    Purpose:

        Return the process-local Windows CE console row.
*/

#include "../../fb.h"
#include "fb_private_console.h"

int fb_ConsoleGetRawYEx( HANDLE console )
{
	(void)console;
	return __fb_con.cursor_y;
}

int fb_ConsoleGetRawY( void )
{
	return __fb_con.cursor_y;
}

int fb_ConsoleGetY( void )
{
	return __fb_con.cursor_y + 1;
}

/* end of drivers/wince/io_gety.c */
