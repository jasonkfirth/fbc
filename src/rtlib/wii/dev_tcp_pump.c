/*
    FreeBASIC runtime Wii TCP receive pump
    --------------------------------------

    File: dev_tcp_pump.c

    Purpose:

        Provide Wii TCP handles with FreeBASIC's EOF/EOC semantics without
        depending on libogc nonblocking recv or peek support.

    Responsibilities:

        - read connected TCP sockets on a small LWP worker thread
        - keep received bytes in handle-local order
        - let EOF mean "buffered bytes are available"
        - let EOC mean "the connection is closed and no buffered bytes remain"

    This file intentionally does NOT contain:

        - TCP protocol string parsing
        - listening socket creation or accept logic
        - generic TCP device hooks for other platforms
*/

#include "../fb.h"
#include "../dev_tcp_private.h"

#include <errno.h>
#include <network.h>
#include <ogc/cond.h>
#include <ogc/lwp.h>
#include <ogc/mutex.h>
#include <string.h>

#define FB_WII_TCP_PUMP_STACK_SIZE (32 * 1024)
#define FB_WII_TCP_PUMP_READ_SIZE 1024

struct FB_WII_TCP_PUMP {
	FB_TCP_SOCKET hSocket;
	lwp_t thread;
	mutex_t mutex;
	cond_t cond;
	int thread_started;
	int closing;
	int closed;
	int error;
	size_t read_pos;
	size_t write_pos;
	size_t used;
	unsigned char buffer[FB_TCP_WII_RECV_BUFFER_SIZE];
};

static int fb_hWiiTcpSocketError( int result )
{
	int err = errno;

	if( result < 0 ) {
		if( result == -EAGAIN || result == -EWOULDBLOCK )
			return result;
		if( err == 0 )
			return result;
	}

	return err;
}

static void fb_hWiiTcpPumpCopyIn( FB_WII_TCP_PUMP *pump, const unsigned char *src, size_t bytes )
{
	size_t first;

	first = FB_TCP_WII_RECV_BUFFER_SIZE - pump->write_pos;
	if( first > bytes )
		first = bytes;

	memcpy( pump->buffer + pump->write_pos, src, first );

	if( bytes > first )
		memcpy( pump->buffer, src + first, bytes - first );

	pump->write_pos = (pump->write_pos + bytes) % FB_TCP_WII_RECV_BUFFER_SIZE;
	pump->used += bytes;
}

static size_t fb_hWiiTcpPumpCopyOut( FB_WII_TCP_PUMP *pump, unsigned char *dst, size_t bytes )
{
	size_t first;

	if( bytes > pump->used )
		bytes = pump->used;

	first = FB_TCP_WII_RECV_BUFFER_SIZE - pump->read_pos;
	if( first > bytes )
		first = bytes;

	memcpy( dst, pump->buffer + pump->read_pos, first );

	if( bytes > first )
		memcpy( dst + first, pump->buffer, bytes - first );

	pump->read_pos = (pump->read_pos + bytes) % FB_TCP_WII_RECV_BUFFER_SIZE;
	pump->used -= bytes;

	return bytes;
}

static void *fb_hWiiTcpPumpThread( void *param )
{
	FB_WII_TCP_PUMP *pump = (FB_WII_TCP_PUMP *)param;
	unsigned char temp[FB_WII_TCP_PUMP_READ_SIZE];

	for( ;; ) {
		size_t room;
		size_t want;
		int bytes;
		int err;

		LWP_MutexLock( pump->mutex );
		while( pump->used == FB_TCP_WII_RECV_BUFFER_SIZE && !pump->closing )
			LWP_CondWait( pump->cond, pump->mutex );

		if( pump->closing ) {
			LWP_MutexUnlock( pump->mutex );
			break;
		}

		room = FB_TCP_WII_RECV_BUFFER_SIZE - pump->used;
		LWP_MutexUnlock( pump->mutex );

		want = room;
		if( want > sizeof( temp ) )
			want = sizeof( temp );

		bytes = recv( pump->hSocket, temp, (s32)want, 0 );
		err = fb_hWiiTcpSocketError( bytes );

		if( bytes > 0 ) {
			LWP_MutexLock( pump->mutex );
			fb_hWiiTcpPumpCopyIn( pump, temp, (size_t)bytes );
			LWP_CondBroadcast( pump->cond );
			LWP_MutexUnlock( pump->mutex );
			continue;
		}

		if( bytes == 0 ||
		    (err != EAGAIN && err != EWOULDBLOCK &&
		     err != -EAGAIN && err != -EWOULDBLOCK) ) {
			/*
				EOF and EOC answer different questions in FreeBASIC.
				A transient empty read means "no bytes are buffered yet",
				but an orderly close or real socket error must wake blocked
				readers so EOC can become true after buffered bytes drain.
			*/
			LWP_MutexLock( pump->mutex );
			pump->closed = TRUE;
			pump->error = err;
			LWP_CondBroadcast( pump->cond );
			LWP_MutexUnlock( pump->mutex );
			break;
		}

		LWP_YieldThread();
	}

	return NULL;
}

FB_WII_TCP_PUMP *fb_WiiTcpPumpCreate( FB_TCP_SOCKET hSocket )
{
	FB_WII_TCP_PUMP *pump;

	pump = (FB_WII_TCP_PUMP *)calloc( 1, sizeof( FB_WII_TCP_PUMP ) );
	if( pump == NULL )
		return NULL;

	pump->hSocket = hSocket;
	pump->thread = LWP_THREAD_NULL;
	pump->mutex = LWP_MUTEX_NULL;
	pump->cond = LWP_COND_NULL;

	if( LWP_MutexInit( &pump->mutex, TRUE ) < 0 ) {
		free( pump );
		return NULL;
	}

	if( LWP_CondInit( &pump->cond ) < 0 ) {
		LWP_MutexDestroy( pump->mutex );
		free( pump );
		return NULL;
	}

	if( LWP_CreateThread( &pump->thread,
	                      fb_hWiiTcpPumpThread,
	                      pump,
	                      NULL,
	                      FB_WII_TCP_PUMP_STACK_SIZE,
	                      64 ) < 0 ) {
		LWP_CondDestroy( pump->cond );
		LWP_MutexDestroy( pump->mutex );
		free( pump );
		return NULL;
	}

	pump->thread_started = TRUE;
	return pump;
}

void fb_WiiTcpPumpDestroy( FB_WII_TCP_PUMP *pump )
{
	if( pump == NULL )
		return;

	LWP_MutexLock( pump->mutex );
	pump->closing = TRUE;
	LWP_CondBroadcast( pump->cond );
	LWP_MutexUnlock( pump->mutex );

	shutdown( pump->hSocket, SHUT_RDWR );
	net_close( pump->hSocket );

	if( pump->thread_started && pump->thread != LWP_GetSelf() )
		LWP_JoinThread( pump->thread, NULL );

	if( pump->cond != LWP_COND_NULL )
		LWP_CondDestroy( pump->cond );
	if( pump->mutex != LWP_MUTEX_NULL )
		LWP_MutexDestroy( pump->mutex );

	free( pump );
}

int fb_WiiTcpPumpPeekState( FB_WII_TCP_PUMP *pump )
{
	int state;

	if( pump == NULL )
		return -1;

	LWP_MutexLock( pump->mutex );

	if( pump->used > 0 )
		state = 1;
	else if( pump->closed || pump->closing )
		state = -1;
	else
		state = 0;

	LWP_MutexUnlock( pump->mutex );
	return state;
}

size_t fb_WiiTcpPumpRead( FB_WII_TCP_PUMP *pump, void *buffer, size_t length, int wait )
{
	size_t bytes = 0;

	if( pump == NULL || buffer == NULL || length == 0 )
		return 0;

	LWP_MutexLock( pump->mutex );

	while( wait && pump->used == 0 && !pump->closed && !pump->closing )
		LWP_CondWait( pump->cond, pump->mutex );

	if( pump->used > 0 )
		bytes = fb_hWiiTcpPumpCopyOut( pump, (unsigned char *)buffer, length );

	if( bytes > 0 )
		LWP_CondBroadcast( pump->cond );

	LWP_MutexUnlock( pump->mutex );
	return bytes;
}

/* end of dev_tcp_pump.c */
