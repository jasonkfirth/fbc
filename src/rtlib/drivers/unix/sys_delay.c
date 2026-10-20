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
	tv.tv_sec = msecs / 1000;
	tv.tv_usec = (msecs % 1000) * 1000;
	select(0, NULL, NULL, NULL, &tv);
}

/* end of drivers/unix/sys_delay.c */
