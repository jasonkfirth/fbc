/* timer() function */

#include "../fb.h"
#include <pthread.h>
#include <time.h>

static pthread_once_t timer_once = PTHREAD_ONCE_INIT;
static double timer_base = 0.0;

static double timer_now(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return 0.0;

	return (double)ts.tv_sec + ((double)ts.tv_nsec * 0.000000001);
}

static void timer_init(void)
{
	timer_base = timer_now();
}

FBCALL double fb_Timer(void)
{
	double now;

	pthread_once(&timer_once, timer_init);
	now = timer_now();

	return now - timer_base;
}

/* end of time_timer.c */
