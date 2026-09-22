/* FreeBASIC DOS runtime: time_timer.c
 * Read wall time for TIMER. Serialize DJGPP's clock conversion state against
 * other callers running under the optional scheduler.
 * This file does not control the scheduler's RTC or the hardware PIT rate.
 */

#include "../fb.h"
#include <time.h>

FBCALL double fb_Timer( void )
{
	struct timeval tv;
	/* gettimeofday() updates shared DJGPP clock conversion state.
	 * Protect the complete operation, not just its inner BIOS interrupt.
	 */
	FB_LOCK();
	gettimeofday(&tv, NULL);
	FB_UNLOCK();
	return (((double)tv.tv_sec * 1000000.0) + (double)tv.tv_usec) * 0.000001;
}

/* end of time_timer.c */
