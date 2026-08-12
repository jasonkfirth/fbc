/*
 * Project: FreeBASIC RISC OS runtime
 * ----------------------------------
 *
 * File: sys_delay.c
 *
 * Purpose:
 *
 *     Suspend the current RISC OS thread for a bounded number of milliseconds.
 *
 * Responsibilities:
 *
 *     - translate FreeBASIC millisecond delays to UnixLib timespec values
 *     - avoid the Unix console backend's zero-descriptor select() delay
 *     - return immediately for zero and negative delay requests
 *
 * This file intentionally does NOT implement SLEEP's keyboard-interrupt
 * policy. The platform-independent time_sleep.c layer owns that behaviour.
 */

#include "../fb.h"

#include <time.h>

FBCALL void fb_Delay( int msecs )
{
	struct timespec delay;

	if( msecs <= 0 )
		return;

	delay.tv_sec = msecs / 1000;
	delay.tv_nsec = (long)(msecs % 1000) * 1000000L;
	nanosleep( &delay, NULL );
}

/* end of sys_delay.c */
