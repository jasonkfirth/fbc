/*
    FreeBASIC runtime Wii timing hooks
    ----------------------------------

    File: sys_time.c

    Purpose:

        Provide TIMER and delay support using libogc's PowerPC timebase.

    Responsibilities:

        - return monotonically increasing seconds for TIMER
        - yield while waiting in SLEEP and graphics delay paths

    This file intentionally does NOT contain:

        - calendar/date conversion
        - alarm or callback scheduling
        - video frame pacing policy
*/

#include "../fb.h"
#include <ogc/lwp.h>
#include <ogc/lwp_watchdog.h>

FBCALL double fb_Timer(void)
{
	return (double)gettime() / (double)PPC_TIMER_CLOCK;
}

FBCALL void fb_Delay(int msecs)
{
	u64 start;

	if (msecs <= 0) {
		LWP_YieldThread();
		return;
	}

	start = gettime();
	while (diff_msec(start, gettime()) < (u32)msecs)
		LWP_YieldThread();
}

/* end of sys_time.c */
