/*
    FreeBASIC runtime library
    -------------------------

    File: amiga/time_settime.c

    Purpose:

        Define TIME assignment behavior on AmigaOS.

    Responsibilities:

        - report that the current AmigaOS SDK has no supported clock setter

    This file intentionally does NOT contain:

        - time parsing
        - timer.device request management
        - undocumented kernel clock mutation
*/

#include "../fb.h"

int fb_hSetTime( int hour, int minute, int second )
{
	(void)hour;
	(void)minute;
	(void)second;
	return -1;
}

/* end of amiga/time_settime.c */
