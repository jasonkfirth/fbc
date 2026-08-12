/*
    FreeBASIC runtime Wii time setter stub
    --------------------------------------

    File: time_settime.c

    Purpose:

        Report that changing the system time is unsupported on Wii.

    Responsibilities:

        - export the target time-setting hook
        - return failure after the generic runtime validates the input

    This file intentionally does NOT contain:

        - real-time clock access
        - timezone conversion
        - time parsing
*/

#include "../fb.h"

int fb_hSetTime(int h, int m, int s)
{
	(void)h;
	(void)m;
	(void)s;

	return -1;
}

/* end of time_settime.c */
