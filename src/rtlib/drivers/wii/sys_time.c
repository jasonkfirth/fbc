/*
    Project: FreeBASIC runtime Wii timing hooks
    ----------------------------------

    File: drivers/wii/sys_time.c

    Purpose:

        Provide TIMER and delay support using libogc's PowerPC timebase.

    Responsibilities:

        - return monotonically increasing seconds for TIMER
        - yield while waiting in SLEEP and graphics delay paths

    This file intentionally does NOT contain: - calendar/date conversion
        - alarm or callback scheduling
        - video frame pacing policy
*/

#include "../../fb.h"
#include <ogc/lwp.h>
#include <ogc/lwp_watchdog.h>
#include <errno.h>
#include <time.h>

FBCALL double fb_Timer(void)
{
	return (double)gettime() / (double)PPC_TIMER_CLOCK;
}

FBCALL void fb_Delay(int msecs)
{
	struct timespec requested, remaining;

	if (msecs <= 0) {
		LWP_YieldThread();
		return;
	}

	/* Yield only offers the CPU to runnable threads at the same or higher
	 * priority. A polling delay can therefore starve a lower-priority TCP
	 * worker. libogc's nanosleep removes this thread from the ready queue
	 * until its timer expires, allowing every runnable priority to advance.
	 * Splitting milliseconds keeps both timespec fields in their valid range.
	 */
	requested.tv_sec = msecs / 1000;
	requested.tv_nsec = (msecs % 1000) * 1000000L;
	while (nanosleep(&requested, &remaining) != 0 && errno == EINTR)
		requested = remaining;
}

/* end of drivers/wii/sys_time.c */
