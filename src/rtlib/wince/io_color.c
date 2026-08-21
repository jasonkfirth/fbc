/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_color.c

    Purpose:

        Maintain BASIC text colors for the Windows CE logical console.

    Responsibilities:

        - apply independent foreground and background updates
        - return the previous BASIC color pair
        - expose the packed attribute stored beside logical text cells

    This file intentionally does NOT contain:

        - desktop SetConsoleTextAttribute calls
        - palette realization
        - graphics-mode COLOR handling
*/

#include "../fb.h"
#include "fb_private_console.h"

unsigned int fb_ConsoleColor( unsigned int foreground,
	                          unsigned int background, int flags )
{
	unsigned int previous;

	previous = __fb_con.foreground | (__fb_con.background << 16);
	if( !(flags & FB_COLOR_FG_DEFAULT) )
		__fb_con.foreground = foreground & 0x0Fu;
	if( !(flags & FB_COLOR_BG_DEFAULT) )
		__fb_con.background = background & 0x0Fu;

	return previous;
}

unsigned int fb_ConsoleGetColorAttEx( HANDLE console )
{
	(void)console;
	return __fb_con.foreground | (__fb_con.background << 4);
}

unsigned int fb_ConsoleGetColorAtt( void )
{
	return fb_ConsoleGetColorAttEx( NULL );
}

/* end of wince/io_color.c */
