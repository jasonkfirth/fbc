/*
    Project: FreeBASIC runtime Wii console redirection
    -----------------------------------------

    File: drivers/wii/io_isredir.c

    Purpose:

        Report whether console input/output is redirected.

    Responsibilities:

        - return false for Wii's libogc console

    This file intentionally does NOT contain: - console drawing
        - keyboard input
*/

#include "../../fb.h"

int fb_ConsoleIsRedirected(int is_input)
{
	return FB_FALSE;
}

/* end of drivers/wii/io_isredir.c */
