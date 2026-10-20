/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/time_timer.c
 * Purpose: timer() function.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <time.h>

FBCALL double fb_Timer( void )
{
	LARGE_INTEGER t;
	KeQuerySystemTime(&t);
	return ((double)(t.QuadPart) * 100.0);
}

/* end of drivers/xbox/time_timer.c */
