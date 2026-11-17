/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/time_settime.c
 * Purpose: Provide time settime runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <dos.h>

int fb_hSetTime( int h, int m, int s )
{
	struct time t;

	if( h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59 )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	t.ti_hour = (unsigned char)h;
	t.ti_min = (unsigned char)m;
	t.ti_sec = (unsigned char)s;
	t.ti_hund = 0;
	settime(&t);
	return 0;
}

/* end of drivers/dos/time_settime.c */
