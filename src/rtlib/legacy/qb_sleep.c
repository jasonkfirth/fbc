/*
 * Project: FreeBASIC runtime library
 * File: legacy/qb_sleep.c
 * Purpose: QB compatible SLEEP.
 * Responsibilities: Legacy and QuickBASIC compatibility entry points.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void fb_SleepQB( int secs )
{
	fb_Sleep( secs < 0 ? secs : secs * 1000 );
}

/* end of legacy/qb_sleep.c */
