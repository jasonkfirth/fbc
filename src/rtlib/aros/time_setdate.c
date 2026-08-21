/*
    FreeBASIC runtime library
    -------------------------

    File: aros/time_setdate.c

    Purpose:

        Define DATE assignment behavior on AROS.

    Responsibilities:

        - report that the current AROS SDK has no supported clock setter

    This file intentionally does NOT contain:

        - date parsing
        - file timestamp changes
        - private timer.device manipulation
*/

#include "../fb.h"

int fb_hSetDate( int year, int month, int day )
{
	(void)year;
	(void)month;
	(void)day;
	return -1;
}

/* end of aros/time_setdate.c */
