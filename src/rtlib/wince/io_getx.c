/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_getx.c

    Purpose:

        Return the process-local Windows CE console column.
*/

#include "../fb.h"
#include "fb_private_console.h"

int fb_ConsoleGetRawXEx( HANDLE console )
{
	(void)console;
	return __fb_con.cursor_x;
}

int fb_ConsoleGetRawX( void )
{
	return __fb_con.cursor_x;
}

int fb_ConsoleGetX( void )
{
	return __fb_con.cursor_x + 1;
}

/* end of wince/io_getx.c */
