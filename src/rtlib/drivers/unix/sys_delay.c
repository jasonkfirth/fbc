/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/sys_delay.c
 * Purpose: Provide sys delay runtime support.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <sys/select.h>

FBCALL void fb_Delay( int msecs )
{
	struct timeval tv;
	unsigned int delay_milliseconds;
	if( msecs <= 0 )
		return;
	delay_milliseconds = (unsigned int)msecs;
	tv.tv_sec = delay_milliseconds / 1000U;
	tv.tv_usec = (suseconds_t)((delay_milliseconds % 1000U) * 1000U);
	select(0, NULL, NULL, NULL, &tv);
}

/* end of drivers/unix/sys_delay.c */
