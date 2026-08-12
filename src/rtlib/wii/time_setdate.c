/*
    FreeBASIC runtime Wii date setter stub
    --------------------------------------

    File: time_setdate.c

    Purpose:

        Report that changing the system date is unsupported on Wii.

    Responsibilities:

        - export the target date-setting hook
        - return failure after the generic runtime validates the input

    This file intentionally does NOT contain:

        - real-time clock access
        - timezone conversion
        - date parsing
*/

#include "../fb.h"

int fb_hSetDate(int y, int m, int d)
{
	(void)y;
	(void)m;
	(void)d;

	return -1;
}

/* end of time_setdate.c */
