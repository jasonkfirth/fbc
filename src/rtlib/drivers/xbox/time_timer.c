/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/time_timer.c
 * Purpose: timer() function.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <xboxkrnl/xboxkrnl.h>

FBCALL double fb_Timer( void )
{
	LARGE_INTEGER ticks;
	/* The kernel counts 100 ns intervals since 1601. Return seconds from
	   the Unix epoch, as on DOS/Unix, without local timezone or DST changes.
	   The epochs are 11644473600 seconds apart. Convert before subtracting
	   so even an invalid signed kernel timestamp cannot overflow an integer. */
	KeQuerySystemTime( &ticks );
	return (double)ticks.QuadPart * 0.0000001 - 11644473600.0;
}

/* end of drivers/xbox/time_timer.c */
