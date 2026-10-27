/*
 * Project: FreeBASIC classic AmigaOS runtime
 * File: drivers/amiga/dev_tcp_socket.c
 * Purpose: Keep native TCP socket calls in the task that owns SocketBase.
 * Responsibilities:
 *   - resolve IPv4 addresses through the version 4 library interface
 *   - keep idle sockets in the native public socket pool
 *   - borrow a socket into the calling task for each native operation
 *   - serialize operations on one socket without holding the BASIC file lock
 * This file intentionally does NOT contain: BASIC stream buffering or parsing.
 *
 * Each OpenLibrary returns a task-owned descriptor table and signal state.
 * A descriptor opened by the main task cannot simply be used by a BASIC
 * worker. ReleaseSocket and ObtainSocket move ownership through the native
 * public pool. An idle socket therefore outlives the task that created it,
 * and CLOSE can run from another task, including process cleanup.
 *
 * The slot table contains private runtime descriptors, not DOS handles.
 * Each slot semaphore protects its pool identifier through a complete call.
 * Calls that can wait are entered by the shared device with FB_LOCK released.
 */

#include "../../fb.h"
#include "dev_tcp_socket.h"
#include <strings.h>

#include <proto/exec.h>
#define __NOLIBBASE__
#include <proto/bsdsocket.h>
#undef __NOLIBBASE__

/* Bound the runtime's pool use below the native default descriptor limit.
   The shared device uses these indices in fd_set, so they must also fit it. */
#define FB_AMIGA_TCP_SOCKETS 64
#if FD_SETSIZE < FB_AMIGA_TCP_SOCKETS
#error Amiga TCP descriptors must fit fd_set
#endif

typedef struct {
	struct SignalSemaphore lock;
	int used;
	LONG pool;
} FB_AMIGA_TCP_SLOT;

typedef struct {
	struct Library *base;
	FB_AMIGA_TCP_SLOT *slot;
	LONG descriptor;
} FB_AMIGA_TCP_LEASE;

static FB_AMIGA_TCP_SLOT sockets[FB_AMIGA_TCP_SOCKETS];
static int initialized;

/* Keep one library context per BASIC task. Reopening it for every byte would
   repeatedly create the stack's signal resources and, on UAE, host workers.
   All idle descriptors are in the public pool before this TLS destructor
   runs, so closing the task's library cannot close another task's stream. */
typedef struct {
	struct Library *base;
} FB_AMIGA_TCP_CONTEXT;

static void free_context( void *data )
{
	FB_AMIGA_TCP_CONTEXT *context = data;
	if( context->base != NULL ) CloseLibrary( context->base );
}

/* ------------------------------------------------------------------------- */
/* Descriptor ownership                                                      */
/* ------------------------------------------------------------------------- */

static struct Library *open_library( void )
{
	FB_AMIGA_TCP_CONTEXT *context = fb_TlsGetCtx( FB_TLSKEY_AMIGA_TCP,
		sizeof( FB_AMIGA_TCP_CONTEXT ), free_context );
	if( context == NULL ) { errno = ENOMEM; return NULL; }
	if( context->base == NULL )
		context->base = OpenLibrary( (CONST_STRPTR)"bsdsocket.library", 4 );
	if( context->base == NULL ) errno = ENETDOWN;
	return context->base;
}

static int publish_socket( struct Library *SocketBase, LONG descriptor )
{
	int index;
	LONG pool;

	/* Forbid protects only the short table allocation. Never call a socket
	   vector, OpenLibrary or ObtainSemaphore with task switching disabled. */
	Forbid();
	if( !initialized ) {
		for( index = 0; index < FB_AMIGA_TCP_SOCKETS; index++ )
			InitSemaphore( &sockets[index].lock );
		initialized = TRUE;
	}
	for( index = 0; index < FB_AMIGA_TCP_SOCKETS; index++ ) {
		if( !sockets[index].used ) {
			sockets[index].used = TRUE;
			break;
		}
	}
	Permit();
	if( index == FB_AMIGA_TCP_SOCKETS ) {
		CloseSocket( descriptor );
		errno = EMFILE;
		return -1;
	}

	ObtainSemaphore( &sockets[index].lock );
	/* -1 asks the stack to allocate a unique public pool identifier. */
	pool = ReleaseSocket( descriptor, -1 );
	if( pool == -1 ) {
		int error = Errno();
		CloseSocket( descriptor );
		sockets[index].used = FALSE;
		ReleaseSemaphore( &sockets[index].lock );
		errno = error;
		return -1;
	}
	sockets[index].pool = pool;
	ReleaseSemaphore( &sockets[index].lock );
	return index;
}

static int borrow_socket( int index, FB_AMIGA_TCP_LEASE *lease )
{
	struct Library *SocketBase;

	memset( lease, 0, sizeof( *lease ) );
	if( index < 0 || index >= FB_AMIGA_TCP_SOCKETS || !initialized ) {
		errno = EBADF;
		return FALSE;
	}
	lease->slot = &sockets[index];
	ObtainSemaphore( &lease->slot->lock );
	if( !lease->slot->used ) {
		ReleaseSemaphore( &lease->slot->lock );
		errno = EBADF;
		return FALSE;
	}
	SocketBase = open_library();
	if( SocketBase == NULL ) {
		ReleaseSemaphore( &lease->slot->lock );
		return FALSE;
	}
	lease->base = SocketBase;
	lease->descriptor = ObtainSocket( lease->slot->pool, AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( lease->descriptor < 0 ) {
		int error = Errno();
		ReleaseSemaphore( &lease->slot->lock );
		errno = error;
		return FALSE;
	}
	return TRUE;
}

static int return_socket( FB_AMIGA_TCP_LEASE *lease, int result )
{
	struct Library *SocketBase = lease->base;
	int error = result < 0 ? Errno() : 0;
	LONG pool = ReleaseSocket( lease->descriptor, -1 );

	if( pool == -1 ) {
		if( error == 0 ) error = Errno();
		CloseSocket( lease->descriptor );
		lease->slot->used = FALSE;
		result = -1;
	} else {
		lease->slot->pool = pool;
	}
	ReleaseSemaphore( &lease->slot->lock );
	if( error != 0 ) errno = error;
	return result;
}

/* ------------------------------------------------------------------------- */
/* IPv4 resolver and socket creation                                         */
/* ------------------------------------------------------------------------- */

int fb_AmigaTcpAddress( const char *host, unsigned int port, int passive, struct sockaddr_in *address )
{
	struct Library *SocketBase;
	struct hostent *entry;
	ULONG raw;
	int result = FALSE;

	if( address == NULL || port > 65535 ) return FALSE;
	memset( address, 0, sizeof( *address ) );
	address->sin_len = sizeof( *address );
	address->sin_family = AF_INET;
	/* This provider is m68k-only: host and network byte order are identical. */
	address->sin_port = port;
	if( host == NULL || *host == '\0' || strcasecmp( host, "localhost" ) == 0 ) {
		address->sin_addr.s_addr = (passive && (host == NULL || *host == '\0'))
			? INADDR_ANY : 0x7f000001UL; /* IPv4 loopback, 127.0.0.1. */
		return TRUE;
	}
	SocketBase = open_library();
	if( SocketBase == NULL ) return FALSE;
	raw = inet_addr( (STRPTR)host );
	if( raw != INADDR_NONE || strcmp( host, "255.255.255.255" ) == 0 ) {
		address->sin_addr.s_addr = raw;
		result = TRUE;
	} else {
		entry = gethostbyname( (STRPTR)host );
		if( entry != NULL && entry->h_addrtype == AF_INET &&
		    entry->h_length == sizeof( address->sin_addr ) &&
		    entry->h_addr_list != NULL && entry->h_addr_list[0] != NULL ) {
			memcpy( &address->sin_addr, entry->h_addr_list[0], sizeof( address->sin_addr ) );
			result = TRUE;
		}
	}
	return result;
}

int fb_AmigaTcpSocket( int domain, int type, int protocol )
{
	struct Library *SocketBase = open_library();
	LONG descriptor;
	int result;
	if( SocketBase == NULL ) return -1;
	descriptor = socket( domain, type, protocol );
	if( descriptor < 0 ) {
		errno = Errno();
		result = -1;
	} else {
		result = publish_socket( SocketBase, descriptor );
	}
	return result;
}

/* The SDK's original vector prototypes predate const qualification. The
   casts below adapt read-only arguments without changing their ownership. */

/* ------------------------------------------------------------------------- */
/* Native operations                                                         */
/* ------------------------------------------------------------------------- */

int fb_AmigaTcpClose( int index )
{
	FB_AMIGA_TCP_LEASE lease;
	struct Library *SocketBase;
	int result, error;
	if( !borrow_socket( index, &lease ) ) return -1;
	SocketBase = lease.base;
	result = CloseSocket( lease.descriptor );
	error = result < 0 ? Errno() : 0;
	lease.slot->used = FALSE;
	ReleaseSemaphore( &lease.slot->lock );
	if( error ) errno = error;
	return result;
}

int fb_AmigaTcpConnect( int index, const struct sockaddr *address, int length )
{
	FB_AMIGA_TCP_LEASE lease;
	struct Library *SocketBase;
	if( !borrow_socket( index, &lease ) ) return -1;
	SocketBase = lease.base;
	return return_socket( &lease, connect( lease.descriptor, (struct sockaddr *)address, length ) );
}

int fb_AmigaTcpBind( int index, const struct sockaddr *address, int length )
{
	FB_AMIGA_TCP_LEASE lease;
	struct Library *SocketBase;
	if( !borrow_socket( index, &lease ) ) return -1;
	SocketBase = lease.base;
	return return_socket( &lease, bind( lease.descriptor, (struct sockaddr *)address, length ) );
}

int fb_AmigaTcpListen( int index, int backlog )
{
	FB_AMIGA_TCP_LEASE lease;
	struct Library *SocketBase;
	if( !borrow_socket( index, &lease ) ) return -1;
	SocketBase = lease.base;
	return return_socket( &lease, listen( lease.descriptor, backlog ) );
}

int fb_AmigaTcpAccept( int index, struct sockaddr *address, socklen_t *length )
{
	FB_AMIGA_TCP_LEASE lease;
	struct Library *SocketBase;
	LONG descriptor;
	int result;
	if( !borrow_socket( index, &lease ) ) return -1;
	SocketBase = lease.base;
	descriptor = accept( lease.descriptor, address, length );
	if( descriptor < 0 ) return return_socket( &lease, -1 );
	result = publish_socket( SocketBase, descriptor );
	if( return_socket( &lease, 0 ) < 0 ) {
		if( result >= 0 ) fb_AmigaTcpClose( result );
		return -1;
	}
	return result;
}

int fb_AmigaTcpShutdown( int index, int how )
{
	FB_AMIGA_TCP_LEASE lease;
	struct Library *SocketBase;
	if( !borrow_socket( index, &lease ) ) return -1;
	SocketBase = lease.base;
	return return_socket( &lease, shutdown( lease.descriptor, how ) );
}

int fb_AmigaTcpRecv( int index, void *buffer, int length, int flags )
{
	FB_AMIGA_TCP_LEASE lease;
	struct Library *SocketBase;
	if( !borrow_socket( index, &lease ) ) return -1;
	SocketBase = lease.base;
	return return_socket( &lease, recv( lease.descriptor, buffer, length, flags ) );
}

int fb_AmigaTcpSend( int index, const void *buffer, int length, int flags )
{
	FB_AMIGA_TCP_LEASE lease;
	struct Library *SocketBase;
	if( !borrow_socket( index, &lease ) ) return -1;
	SocketBase = lease.base;
	return return_socket( &lease, send( lease.descriptor, (void *)buffer, length, flags ) );
}

int fb_AmigaTcpIoctl( int index, unsigned long command, void *argument )
{
	FB_AMIGA_TCP_LEASE lease;
	struct Library *SocketBase;
	if( !borrow_socket( index, &lease ) ) return -1;
	SocketBase = lease.base;
	return return_socket( &lease, IoctlSocket( lease.descriptor, command, argument ) );
}

int fb_AmigaTcpSetSockOpt( int index, int level, int option, const void *value, int length )
{
	FB_AMIGA_TCP_LEASE lease;
	struct Library *SocketBase;
	if( !borrow_socket( index, &lease ) ) return -1;
	SocketBase = lease.base;
	return return_socket( &lease, setsockopt( lease.descriptor, level, option, (void *)value, length ) );
}

int fb_AmigaTcpSelect( int count, fd_set *readers, fd_set *writers, fd_set *errors, struct timeval *timeout )
{
	FB_AMIGA_TCP_LEASE lease;
	struct Library *SocketBase;
	fd_set native_readers, native_writers, native_errors;
	int index, selected = -1, result;
	struct __timeval native_timeout;

	/* This is a private single-socket readiness operation, as used by the
	   shared device. Reject wider sets rather than losing descriptor bits. */
	if( count < 0 || count > FB_AMIGA_TCP_SOCKETS ) { errno = EINVAL; return -1; }
	for( index = 0; index < count; index++ ) {
		if( (readers && FD_ISSET( index, readers )) ||
		    (writers && FD_ISSET( index, writers )) ||
		    (errors && FD_ISSET( index, errors )) ) {
			if( selected >= 0 ) { errno = EINVAL; return -1; }
			selected = index;
		}
	}
	if( !borrow_socket( selected, &lease ) ) return -1;
	SocketBase = lease.base;
	FD_ZERO( &native_readers ); FD_ZERO( &native_writers ); FD_ZERO( &native_errors );
	if( readers && FD_ISSET( selected, readers ) ) FD_SET( lease.descriptor, &native_readers );
	if( writers && FD_ISSET( selected, writers ) ) FD_SET( lease.descriptor, &native_writers );
	if( errors && FD_ISSET( selected, errors ) ) FD_SET( lease.descriptor, &native_errors );
	if( timeout != NULL ) {
		native_timeout.tv_sec = timeout->tv_sec;
		native_timeout.tv_usec = timeout->tv_usec;
	}
	result = WaitSelect( lease.descriptor + 1, readers ? &native_readers : NULL,
	                     writers ? &native_writers : NULL, errors ? &native_errors : NULL,
	                     timeout ? &native_timeout : NULL, NULL );
	if( result >= 0 ) {
		if( readers && !FD_ISSET( lease.descriptor, &native_readers ) ) FD_CLR( selected, readers );
		if( writers && !FD_ISSET( lease.descriptor, &native_writers ) ) FD_CLR( selected, writers );
		if( errors && !FD_ISSET( lease.descriptor, &native_errors ) ) FD_CLR( selected, errors );
	}
	return return_socket( &lease, result );
}

/* end of drivers/amiga/dev_tcp_socket.c */
