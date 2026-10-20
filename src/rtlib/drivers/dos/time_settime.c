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
	t.ti_hour = h;
	t.ti_min = m;
	t.ti_sec = s;
	t.ti_hund = 0;
	settime(&t);
	return 0;
}

/* end of drivers/dos/time_settime.c */
