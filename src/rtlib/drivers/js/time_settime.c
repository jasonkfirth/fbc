/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/time_settime.c
 * Purpose: Provide time settime runtime support.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_hSetTime( int h, int m, int s )
{
	return 0;
}

/* end of drivers/js/time_settime.c */
