/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/time_sleep.c
 * Purpose: sleep() function, Xbox.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

void fb_ConsoleSleep( int msecs )
{
	/*
		The Xbox console backend has no keyboard input to poll while sleeping.
		Graphics mode installs its own sleep hook, so the console path only
		needs to wait for the requested duration.
	*/
	fb_Delay( msecs );
}

/* end of drivers/xbox/time_sleep.c */
