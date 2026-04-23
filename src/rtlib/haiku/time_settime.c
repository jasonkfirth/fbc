#include "../fb.h"
#include <sys/time.h>
#include <time.h>

int fb_hSetTime( int h, int m, int s )
{
	struct timeval tv;
	struct timespec ts;

	gettimeofday( &tv, NULL );
	
	tv.tv_sec -= (tv.tv_sec % 86400);
	tv.tv_sec += (h * 3600) + (m * 60) + s;
	
	ts.tv_sec = tv.tv_sec;
	ts.tv_nsec = tv.tv_usec * 1000;
	
	
	if( clock_settime (CLOCK_REALTIME, &ts) )
		return -1;
	return 0;
}
