/* FreeBASIC DOS provider investigation: djgpp-timer.c
 *
 * Verify that DJGPP's interval timer can interrupt a busy loop in the guest.
 * This separates basic timer delivery from a candidate thread scheduler.
 * No threading library is linked. Run with an external timeout.
 */

#include <signal.h>
#include <stdio.h>
#include <sys/time.h>

static volatile sig_atomic_t ticks;

static void on_alarm( int signal_number )
{
	(void)signal_number;
	/* Bound the counter even if the main thread is delayed for a long time. */
	if( ticks < 10 )
		ticks++;
}

int main( void )
{
	struct itimerval timer = { { 0, 20000 }, { 0, 20000 } };
	struct itimerval stopped = { { 0, 0 }, { 0, 0 } };
	struct sigaction action = { 0 };

	action.sa_handler = on_alarm;
	if( sigemptyset( &action.sa_mask ) != 0 ||
	    sigaction( SIGALRM, &action, NULL ) != 0 ||
	    setitimer( ITIMER_REAL, &timer, NULL ) != 0 ) {
		puts( "FAIL timer setup" );
		return 1;
	}
	while( ticks < 10 ) {
	}
	if( setitimer( ITIMER_REAL, &stopped, NULL ) != 0 ) {
		puts( "FAIL timer stop" );
		return 1;
	}
	puts( "PASS ten timer signals without yielding" );
	return 0;
}

/* end of djgpp-timer.c */
