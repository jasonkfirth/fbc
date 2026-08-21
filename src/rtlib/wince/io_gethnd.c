/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_gethnd.c

    Purpose:

        Provide the console-handle compatibility hooks on Windows CE.

    Responsibilities:

        - identify that Windows CE has no desktop console handles
        - preserve handle-shaped entry points used by shared runtime code

    This file intentionally does NOT contain:

        - desktop GetStdHandle calls
        - console-mode mutation
        - stream redirection detection
*/

#include "../fb.h"
#include "fb_private_console.h"

HANDLE fb_hConsoleGetHandle( int is_input )
{
	(void)is_input;
	return NULL;
}

void fb_hConsoleResetHandles( void )
{
}

/* end of wince/io_gethnd.c */
