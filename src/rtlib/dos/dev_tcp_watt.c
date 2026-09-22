/* FreeBASIC DOS runtime: dev_tcp_watt.c
 *
 * Keep Watt-32's clocks and socket operations compatible with graphics and
 * the optional DOS scheduler. The packet driver continues receiving IRQs
 * while the runtime lock excludes thread switches through shared stack state.
 * BASIC file semantics and network initialization remain in dev_tcp.c.
 */

#include "../fb.h"
#include "dev_tcp_watt.h"

#if defined(FB_DOS_WATT32) && !defined(DISABLE_TCP)
#include <errno.h>
#include <limits.h>
#include <sys/socket.h>
#include <tcp.h>

void fb_DosTcpInitClock( void )
{
	struct timeval now;

	/* Watt's first clock read programs PIT channel 0 for 18.2 Hz. Do this
	 * before BASIC global constructors can enter graphics, then select the
	 * DJGPP clock. Doing it at OPEN TCP would overwrite gfxlib2's 1 kHz PIT
	 * reload without changing its BIOS-tick divider. No packet driver or
	 * WATTCP.CFG is accessed here; socket initialization remains lazy.
	 */
	gettimeofday2( &now, NULL );
	hires_timer( FALSE );
}

int fb_DosTcpSend( int socket, const void *buffer, int length, int flags )
{
	int result;
	FB_LOCK();
	result = send( socket, buffer, length, flags );
	FB_UNLOCK();
	return result;
}

int fb_DosTcpRecv( int socket, void *buffer, int length, int flags )
{
	int result;
	FB_LOCK();
	result = recv( socket, buffer, length, flags );
	FB_UNLOCK();
	return result;
}

int fb_DosTcpClose( int socket )
{
	int result;
	FB_LOCK();
	result = closesocket( socket );
	FB_UNLOCK();
	return result;
}

int fb_DosTcpShutdown( int socket, int how )
{
	int result;
	FB_LOCK();
	result = shutdown( socket, how );
	FB_UNLOCK();
	return result;
}

int fb_DosTcpSelect( int count, fd_set *readers, fd_set *writers,
                    fd_set *errors, struct timeval *timeout )
{
	fd_set input_readers, input_writers, input_errors;
	double started = 0;
	double seconds = 0;
	int result;

	if( count < 0 || count > FD_SETSIZE ) {
		errno = EINVAL;
		return -1;
	}
	if( timeout ) {
		/* DJGPP's timeval seconds are unsigned. BASIC timeouts are bounded
		 * by UINT_MAX milliseconds, so larger second counts are invalid here.
		 */
		if( timeout->tv_sec > UINT_MAX / 1000u || timeout->tv_usec < 0 || timeout->tv_usec >= 1000000 ) {
			errno = EINVAL;
			return -1;
		}
		seconds = timeout->tv_sec + timeout->tv_usec * 0.000001;
		if( seconds > 0 ) started = fb_Timer();
	}
	if( readers ) input_readers = *readers;
	if( writers ) input_writers = *writers;
	if( errors ) input_errors = *errors;

	/* select_s() pumps the entire user-space stack for each readiness poll.
	 * Serialize each poll, but sleep outside the lock between polls. Holding
	 * the lock for a blocking select would starve a local peer or audio worker.
	 * select modifies its sets, so restore the requested sets on each attempt.
	 * All callers enter without the file lock, including TCP ACCEPT.
	 */
	for( ;; ) {
		struct timeval poll = { 0, 0 };
		if( readers ) *readers = input_readers;
		if( writers ) *writers = input_writers;
		if( errors ) *errors = input_errors;
		FB_LOCK();
		result = select_s( count, readers, writers, errors, &poll );
		FB_UNLOCK();
		if( result != 0 || (timeout && (seconds == 0 || fb_Timer() - started >= seconds)) )
			return result;
		fb_Delay( 1 );
	}
}

#endif

/* end of dev_tcp_watt.c */
