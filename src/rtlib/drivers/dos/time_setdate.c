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
	dt.da_year = y;
	dt.da_mon = m;
	dt.da_day = d;
	setdate(&dt);
	return 0;
}

/* end of drivers/dos/time_setdate.c */
