/*
 * FreeBASIC runtime: sys_hidle.c
 * Purpose: Supply the legacy timer fallback for the cooperative idle service.
 * Responsibilities: Reuse TIMER where a target has no monotonic clock adapter.
 * This file does not sleep, create workers or implement desktop clocks.
 */
#include "fb_services.h"
int fb_hIdleNow(double *seconds)
{
	*seconds = fb_Timer();
	return TRUE;
}
/* end of sys_hidle.c */
