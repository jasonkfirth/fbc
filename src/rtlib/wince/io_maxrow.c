/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_maxrow.c

    Purpose:

        Return the logical console's last usable row count.

    Responsibilities:

        - expose the current logical height to VIEW and CLS

    This file intentionally does NOT contain:

        - desktop largest-window queries
        - physical display geometry
        - resize operations
*/

#include "../fb.h"
#include "fb_private_console.h"

int fb_ConsoleGetMaxRow( void )
{
	return __fb_con.rows;
}

/* end of wince/io_maxrow.c */
