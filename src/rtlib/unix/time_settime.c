#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif

#include "../fb.h"
#include <sys/time.h>

#if defined( HOST_SOLARIS )
extern int settimeofday( struct timeval *, void * );
#endif

int fb_hSetTime( int h, int m, int s )
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	tv.tv_sec -= (tv.tv_sec % 86400);
	tv.tv_sec += (h * 3600) + (m * 60) + s;
	if( settimeofday( &tv, NULL ) )
		return -1;
	return 0;
}
