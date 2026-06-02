/*
    FreeBASIC runtime Wii console line input
    ----------------------------------------

    File: io_readstr.c

    Purpose:

        Provide the console input hook used by INPUT and LINE INPUT.

    Responsibilities:

        - keep the generic read-string runtime path linkable
        - defer to newlib stdin when a host or loader provides one

    This file intentionally does NOT contain:

        - on-screen keyboard handling
        - Wii Remote text entry
        - graphics input events
*/

#include "../fb.h"

char *fb_ConsoleReadStr(char *buffer, ssize_t len)
{
	return fgets(buffer, len, stdin);
}

/* end of io_readstr.c */
