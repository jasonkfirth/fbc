/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/time_setdate.c
 * Purpose: Provide time setdate runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <dos.h>

int fb_hSetDate( int y, int m, int d )
{
	struct date dt;

	if( y < 1980 || y > 2099 || m < 1 || m > 12 || d < 1 || d > 31 )
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	dt.da_year = (short)y;
	dt.da_mon = (char)m;
	dt.da_day = (char)d;
	setdate(&dt);
	return 0;
}

/* end of drivers/dos/time_setdate.c */
