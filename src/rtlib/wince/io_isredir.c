/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/io_isredir.c

    Purpose:

        Report logical-console stream ownership on Windows CE.

    Responsibilities:

        - identify runtime screen input and output as console services

    This file intentionally does NOT contain:

        - desktop GetConsoleMode probing
        - file-descriptor introspection
        - standard-stream replacement

    CeGCC supplies C standard streams but Windows CE has no desktop console
    handles from which redirection can be inferred.  FreeBASIC's screen device
    is therefore treated as the console unless a BASIC file handle is used.
*/

#include "../fb.h"

int fb_ConsoleIsRedirected( int is_input )
{
	(void)is_input;
	return FB_FALSE;
}

/* end of wince/io_isredir.c */
