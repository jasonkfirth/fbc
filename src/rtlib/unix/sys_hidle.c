/*
 * FreeBASIC Unix runtime: sys_hidle.c
 * Purpose: Supply monotonic idle deadlines on POSIX desktops and Android.
 * Responsibilities: Read CLOCK_MONOTONIC and report clock failures.
 * This file contains no scheduler, sleep policy or wall-clock fallback.
 */
#include "../fb_services.h"
#include <time.h>
int fb_hIdleNow(double *seconds)
{
#ifdef CLOCK_MONOTONIC
	struct timespec value;
	if (clock_gettime(CLOCK_MONOTONIC, &value) != 0)
		return FALSE;
	*seconds = (double)value.tv_sec + (double)value.tv_nsec / 1000000000.0;
	return TRUE;
#else
	*seconds = fb_Timer();
	return TRUE;
#endif
}
/* end of sys_hidle.c */
