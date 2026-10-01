/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/time_timer.c

    Purpose:
        Supply the time interfaces missing from the pinned Amiga newlib SDK.

    Responsibilities:
        - implement TIMER and the time helpers required by libpthread
        - translate the AmigaDOS epoch and 50 Hz DateStamp tick units

    This file intentionally does NOT contain:
        - system clock changes, device ownership, or console scheduling
*/

#include "../fb.h"

#include <proto/dos.h>
#include <sys/time.h>

/* AmigaDOS starts on 1978-01-01, 2922 days after the Unix epoch. DateStamp's
   tick unit remains 1/50 second on both PAL and NTSC machines. */
#define FB_AMIGA_EPOCH_SECONDS 252460800L
#define FB_AMIGA_TICKS_PER_SECOND 50L

int gettimeofday(struct timeval *value, struct timezone *timezone)
{
	struct DateStamp stamp;

	(void)timezone;
	if( value == NULL ) {
		errno = EINVAL;
		return -1;
	}

	DateStamp(&stamp);
	value->tv_sec = FB_AMIGA_EPOCH_SECONDS + stamp.ds_Days * 86400L +
		stamp.ds_Minute * 60L + stamp.ds_Tick / FB_AMIGA_TICKS_PER_SECOND;
	value->tv_usec = (stamp.ds_Tick % FB_AMIGA_TICKS_PER_SECOND) *
		(1000000L / FB_AMIGA_TICKS_PER_SECOND);
    return 0;
}

/* Newlib's reentrant gettimeofday wrapper calls this platform hook. */
int _gettimeofday(struct timeval *value, void *timezone)
{
    return gettimeofday(value, (struct timezone *)timezone);
}

/* The SDK's pthread archive calls a function named timersub rather than the
   usual POSIX macro. Preserve that ABI in the platform replacement. */
#undef timersub
void timersub(const struct timeval *left, const struct timeval *right,
	struct timeval *result)
{
	result->tv_sec = left->tv_sec - right->tv_sec;
	result->tv_usec = left->tv_usec - right->tv_usec;
	if( result->tv_usec < 0 ) {
		--result->tv_sec;
		result->tv_usec += 1000000L;
	}
}

FBCALL double fb_Timer(void)
{
	struct timeval value;

	gettimeofday(&value, NULL);
	return (double)value.tv_sec + (double)value.tv_usec * 0.000001;
}

/* end of amiga/time_timer.c */
