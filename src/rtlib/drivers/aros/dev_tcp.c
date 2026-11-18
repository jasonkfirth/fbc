/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/aros/dev_tcp.c

    Purpose:

        Implement OPEN TCP and OPEN TCP SERVER through AROS
        bsdsocket.library.

    Responsibilities:

        - lazily open and configure bsdsocket.library
        - expose the FreeBASIC TCP file-device contract
        - use AROS socket close, select, and ioctl entry points
        - resolve client and server addresses through the native resolver

    This file intentionally does NOT contain:

        - TCP parsing policy, which remains in dev_tcp_parse.c
        - networking startup requesters or unconditional diagnostics
        - socket implementations for other FreeBASIC targets

    AROS socket model:

        Socket calls are library vectors rather than ordinary libc symbols.
        The target replacement therefore owns SocketBase and its lifetime.
        A weak definition allows an AROS application with an existing strong
        SocketBase definition to keep ownership of that base.
*/

#define __CONFIG_ROADSHOW__ 1
#define _BSD_SOURCE 1

#include <proto/exec.h>
#include <proto/socket.h>
#include <bsdsocket/socketbasetags.h>
#include <utility/tagitem.h>

struct Library *SocketBase __attribute__((weak));

#include "../../fb.h"
#include <strings.h>
#include "../../devices/dev_tcp_private.h"

#ifdef DISABLE_TCP

int fb_DevTcpOpen( FB_FILE *handle, const char *filename, size_t filename_len )
{
	(void)handle;
	(void)filename;
	(void)filename_len;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_DevTcpOpenServer( FB_FILE *handle, const char *filename, size_t filename_len )
{
	(void)handle;
	(void)filename;
	(void)filename_len;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_DevTcpAcceptHandle( FB_FILE *server_handle, FB_FILE *client_handle )
{
	(void)server_handle;
	(void)client_handle;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_DevTcpEocEx( FB_FILE *handle )
{
	(void)handle;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

#else

#include <limits.h>

#include <errno.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>

/*
    bsdsocket.library vectors marshal signed POSIX values through unsigned
    register words. Keep that AROS ABI conversion inside these wrappers so
    strict diagnostics remain enabled for the runtime's own socket logic.
*/
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
static ULONG fb_hArosSocketBaseTags(struct TagItem *tags)
{
	return SocketBaseTagList(tags);
}

static int fb_hArosSocket(int domain, int type, int protocol)
{
	return socket(domain, type, protocol);
}

static int fb_hArosConnect(int socket, const struct sockaddr *address,
	socklen_t address_length)
{
	return connect(socket, address, address_length);
}

static int fb_hArosBind(int socket, const struct sockaddr *address,
	socklen_t address_length)
{
	return bind(socket, address, address_length);
}

static int fb_hArosListen(int socket, int backlog)
{
	return listen(socket, backlog);
}

static int fb_hArosAccept(int socket, struct sockaddr *address,
	socklen_t *address_length)
{
	return accept(socket, address, address_length);
}

static int fb_hArosShutdown(int socket, int how)
{
	return shutdown(socket, how);
}

static int fb_hArosSend(int socket, const void *buffer, int length, int flags)
{
	return send(socket, buffer, length, flags);
}

static int fb_hArosRecv(int socket, void *buffer, int length, int flags)
{
	return recv(socket, buffer, length, flags);
}

static int fb_hArosSetSockOpt(int socket, int level, int option,
	const void *value, socklen_t value_length)
{
	return setsockopt(socket, level, option, value, value_length);
}

static int fb_hArosCloseSocket(int socket)
{
	return CloseSocket(socket);
}

static int fb_hArosWaitSelect(int count, fd_set *readers, fd_set *writers,
	fd_set *errors, struct timeval *timeout)
{
	return WaitSelect(count, readers, writers, errors, timeout, NULL);
}

static int fb_hArosIoctlSocket(int socket, unsigned long request, char *argument)
{
	return IoctlSocket(socket, request, argument);
}

static LONG fb_hArosGetAddrInfo(const char *node, const char *service,
	const struct addrinfo *hints, struct addrinfo **result)
{
	return getaddrinfo(node, service, hints, result);
}

static void fb_hArosFreeAddrInfo(struct addrinfo *result)
{
	freeaddrinfo(result);
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#undef SocketBaseTagList
#define SocketBaseTagList(tags) fb_hArosSocketBaseTags(tags)
#undef socket
#define socket(domain, type, protocol) fb_hArosSocket(domain, type, protocol)
#undef connect
#define connect(socket, address, length) fb_hArosConnect(socket, address, length)
#undef bind
#define bind(socket, address, length) fb_hArosBind(socket, address, length)
#undef listen
#define listen(socket, backlog) fb_hArosListen(socket, backlog)
#undef accept
#define accept(socket, address, length) fb_hArosAccept(socket, address, length)
#undef shutdown
#define shutdown(socket, how) fb_hArosShutdown(socket, how)
#undef send
#define send(socket, buffer, length, flags) fb_hArosSend(socket, buffer, length, flags)
#undef recv
#define recv(socket, buffer, length, flags) fb_hArosRecv(socket, buffer, length, flags)
#undef setsockopt
#define setsockopt(socket, level, option, value, length) \
	fb_hArosSetSockOpt(socket, level, option, value, length)
#undef CloseSocket
#define CloseSocket(socket) fb_hArosCloseSocket(socket)
#undef WaitSelect
#define WaitSelect(count, readers, writers, errors, timeout, signals) \
	fb_hArosWaitSelect(count, readers, writers, errors, timeout)
#undef IoctlSocket
#define IoctlSocket(socket, request, argument) \
	fb_hArosIoctlSocket(socket, request, argument)
#undef getaddrinfo
#define getaddrinfo(node, service, hints, result) \
	fb_hArosGetAddrInfo(node, service, hints, result)
#undef freeaddrinfo
#define freeaddrinfo(result) fb_hArosFreeAddrInfo(result)

#define FB_TCP_CLOSESOCKET(s) CloseSocket( s )
#define FB_TCP_SOCKET_ERROR(s) ((s) == FB_TCP_INVALID_SOCKET)
#define FB_TCP_ERRNO() errno
#define FB_TCP_WOULDBLOCK(err) ((err) == EAGAIN || (err) == EWOULDBLOCK)
#define FB_TCP_SHUT_WR SHUT_WR
#define FB_TCP_SHUT_RDWR SHUT_RDWR
#define FB_TCP_SELECT(n, r, w, e, t) WaitSelect( n, r, w, e, t, NULL )
#define FB_TCP_IOCTL(s, cmd, argp) IoctlSocket( s, cmd, (char *)(argp) )
#define FB_TCP_RECV(s, b, l, f) recv( s, b, l, f )
#define FB_TCP_SEND(s, b, l, f) send( s, b, l, f )
#define FB_TCP_SETSOCKOPT(s, l, o, v, n) setsockopt( s, l, o, v, n )
#define FB_TCP_CAN_QUERY_BYTES TRUE

static FB_TCP_SOCKET fb_hArosTcpCreateSocket(long domain, long type,
	long protocol)
{
	long native_socket;
	FB_TCP_SOCKET result;

	if ((domain < INT_MIN) || (domain > INT_MAX) ||
	    (type < INT_MIN) || (type > INT_MAX) ||
	    (protocol < INT_MIN) || (protocol > INT_MAX))
		return FB_TCP_INVALID_SOCKET;

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
	native_socket = socket((int)domain, (int)type, (int)protocol);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

	if (native_socket < 0)
		return FB_TCP_INVALID_SOCKET;
	if (native_socket >= FD_SETSIZE) {
		if (native_socket <= INT_MAX) {
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
			FB_TCP_CLOSESOCKET((FB_TCP_SOCKET)native_socket);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
		}
		return FB_TCP_INVALID_SOCKET;
	}

	/* AROS' fd_set is limited to FD_SETSIZE and stores int socket numbers. */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
	result = (FB_TCP_SOCKET)native_socket;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
	return result;
}

static int fb_hArosTcpAddressLength(size_t length, socklen_t *result)
{
	if ((result == NULL) || (length == 0) || (length > (size_t)UINT_MAX))
		return FALSE;

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
	*result = (socklen_t)length;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
	return TRUE;
}

static int fb_hArosTcpSetFd(FB_TCP_SOCKET socket, fd_set *set)
{
	if ((set == NULL) || (socket < 0) || (socket >= FD_SETSIZE))
		return FALSE;

	FD_SET((unsigned int)socket, set);
	return TRUE;
}

#define FB_TCP_SOCKET_CLIENT 0
#define FB_TCP_SOCKET_LISTENER 1
#define FB_TCP_SOCKET_ACCEPTED 2

static int fb_DevTcpClose( FB_FILE *handle );
static int fb_DevTcpRead( FB_FILE *handle, void *value, size_t *pValuelen );
static int fb_DevTcpReadWstr( FB_FILE *handle, FB_WCHAR *value, size_t *pValuelen );
static int fb_DevTcpReadLine( FB_FILE *handle, FBSTRING_ACCESS *dst );
static int fb_DevTcpReadLineWstr( FB_FILE *handle, FB_WCHAR *dst, ssize_t dst_chars );
static int fb_DevTcpWrite( FB_FILE *handle, const void *value, size_t valuelen );
static int fb_DevTcpWriteWstr( FB_FILE *handle, const FB_WCHAR *value, size_t valuelen );
static int fb_DevTcpTell( FB_FILE *handle, fb_off_t *pOffset );
static int fb_DevTcpEof( FB_FILE *handle );
static int fb_DevTcpServerEof( FB_FILE *handle );
static void fb_hDevTcpFormatService( char *service, size_t service_len, unsigned int port );

static int fb_hDevTcpSocketError( int result )
{
	(void)result;
	return FB_TCP_ERRNO();
}

static FB_FILE_HOOKS hooks_dev_tcp = {
	fb_DevTcpEof,
	fb_DevTcpClose,
	NULL,
	fb_DevTcpTell,
	fb_DevTcpRead,
	fb_DevTcpReadWstr,
	fb_DevTcpWrite,
	fb_DevTcpWriteWstr,
	NULL,
	NULL,
	fb_DevTcpReadLine,
	fb_DevTcpReadLineWstr,
	NULL,
	NULL
};

static FB_FILE_HOOKS hooks_dev_tcp_server = {
	fb_DevTcpServerEof,
	fb_DevTcpClose,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static int fb_aros_owns_socket_base = FALSE;

static void fb_hDevTcpShutdownAros( void )
{
	if( fb_aros_owns_socket_base && SocketBase != NULL ) {
		CloseLibrary( SocketBase );
		SocketBase = NULL;
		fb_aros_owns_socket_base = FALSE;
	}
}

static int fb_hDevTcpInit( void )
{
	static int is_init = FALSE;
	static int init_result = FB_RTERROR_OK;

	if( is_init )
		return init_result;

	FB_LOCK();
	if( is_init == FALSE ) {
		if( SocketBase == NULL ) {
			/*
				Version 4 provides the resolver and thread-safe errno
				configuration used by the AROS TCP replacement.
			*/
	SocketBase = OpenLibrary( (CONST_STRPTR)"bsdsocket.library", 4 );
			if( SocketBase != NULL )
				fb_aros_owns_socket_base = TRUE;
		}

		if( SocketBase == NULL ) {
			init_result = fb_ErrorSetNum( FB_RTERROR_FILEIO );
		} else {
			struct TagItem tags[2];
			int *errno_pointer = &errno;

			tags[0].ti_Tag = SBTM_SETVAL( SBTC_ERRNOPTR( sizeof( errno ) ) );
			tags[0].ti_Data = (IPTR)errno_pointer;
			tags[1].ti_Tag = TAG_DONE;
			tags[1].ti_Data = 0;

			if( SocketBaseTagList( tags ) != 0 ) {
				init_result = fb_ErrorSetNum( FB_RTERROR_FILEIO );
			} else if( fb_aros_owns_socket_base ) {
				atexit( fb_hDevTcpShutdownAros );
			}
		}

		is_init = TRUE;
	}
	FB_UNLOCK();

	return init_result;
}

static void fb_hDevTcpNormalizeOpenMode( FB_FILE *handle )
{
	if( handle->mode == FB_FILE_MODE_RANDOM ) {
		handle->mode = FB_FILE_MODE_BINARY;
		handle->access = FB_FILE_ACCESS_READWRITE;
	}
}

static DEV_TCP_INFO *fb_hDevTcpAllocInfo( FB_TCP_SOCKET hSocket, const char *pszDevice, DEV_TCP_PROTOCOL *tcp_proto, int is_server )
{
	DEV_TCP_INFO *info = calloc( 1, sizeof( DEV_TCP_INFO ) );
	if( info == NULL )
		return NULL;

	info->hSocket = hSocket;
	info->pszDevice = strdup( pszDevice );
	info->timeout = tcp_proto->timeout;
	info->is_server = is_server;
	info->is_closed = FALSE;

	if( info->pszDevice == NULL ) {
		free( info );
		return NULL;
	}

	return info;
}

static int fb_hDevTcpApplySocketOptions( FB_TCP_SOCKET hSocket, unsigned int timeout, int socket_role )
{
	#if defined(SO_NOSIGPIPE)
		{
			int value = 1;
			FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_NOSIGPIPE, (const char *)&value, sizeof( value ) );
		}
	#endif

	if( timeout != 0 ) {
		struct timeval value;

		value.tv_sec = timeout / 1000;
		value.tv_usec = (suseconds_t)((timeout % 1000u) * 1000u);

		FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&value, sizeof( value ) );

		/*
			OPEN TCP SERVER timeout is useful on the listening socket
			because old threaded games poll TCP ACCEPT from their main
			server loop.  The accepted data socket must still allow a
			large startup packet to drain.  Inheriting timeout=1 as a
			send timeout can otherwise make byte-wise PUT code fail on
			stacks that enforce SO_SNDTIMEO strictly.
		*/
		if( socket_role != FB_TCP_SOCKET_ACCEPTED )
			FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&value, sizeof( value ) );
		else {
			struct timeval no_timeout;

			/*
				Some BSD stacks inherit SO_SNDTIMEO from the listener.
				Clear it explicitly so an accepted data socket can
				block while the peer drains byte-wise PUT traffic.
			*/
			no_timeout.tv_sec = 0;
			no_timeout.tv_usec = 0;
			FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&no_timeout, sizeof( no_timeout ) );
		}
	}

	return FB_RTERROR_OK;
}

static void fb_hDevTcpFormatService( char *service, size_t service_len, unsigned int port )
{
	char tmp[16];
	size_t used = 0;
	size_t i;

	if( service_len == 0 )
		return;

	do {
		tmp[used++] = (char)( '0' + ( port % 10 ) );
		port /= 10;
	} while( (port != 0) && (used < sizeof( tmp )) );

	if( used >= service_len )
		used = service_len - 1;

	for( i = 0; i < used; i++ )
		service[i] = tmp[used - i - 1];

	service[used] = '\0';
}

/*
	TCP device hooks are called by the generic file layer while FB_LOCK()
	is held.  Blocking socket syscalls must not keep that lock, because a
	thread waiting for TCP input can otherwise prevent another FB thread in
	the same process from writing the data it is waiting for.
*/
static int fb_hDevTcpRecvUnlocked( DEV_TCP_INFO *info, void *buffer, size_t length, int flags, int *err )
{
	FB_TCP_SOCKET hSocket = info->hSocket;
	int bytes;

	FB_UNLOCK();
	bytes = FB_TCP_RECV( hSocket, buffer, (int)MIN( length, (size_t)INT_MAX ), flags );
	*err = fb_hDevTcpSocketError( bytes );
	FB_LOCK();

	return bytes;
}

static int fb_hDevTcpSendUnlocked( DEV_TCP_INFO *info, const char *buffer, size_t length, int flags, int *err )
{
	FB_TCP_SOCKET hSocket = info->hSocket;
	int bytes;

	FB_UNLOCK();
	bytes = FB_TCP_SEND( hSocket, buffer, (int)MIN( length, (size_t)INT_MAX ), flags );
	*err = fb_hDevTcpSocketError( bytes );
	FB_LOCK();

	return bytes;
}

static void fb_hDevTcpSetNoDelay( FB_TCP_SOCKET hSocket )
{
#ifdef TCP_NODELAY
	int value = 1;

	/*
		Old FreeBASIC TCP game code often writes packet fields one PUT at a
		time.  Nagle delays make that pattern painfully slow on some targets,
		especially when the peer polls EOF between byte reads.
	*/
	FB_TCP_SETSOCKOPT( hSocket, IPPROTO_TCP, TCP_NODELAY, (const char *)&value, sizeof( value ) );
#else
	(void)hSocket;
#endif
}

static int fb_hDevTcpCreateConnectedSocket( DEV_TCP_PROTOCOL *tcp_proto, FB_TCP_SOCKET *hSocketOut )
{
	struct addrinfo hints;
	struct addrinfo *result;
	struct addrinfo *it;
	FB_TCP_SOCKET hSocket = FB_TCP_INVALID_SOCKET;
	char service[16];
	int res;

	memset( &hints, 0, sizeof( hints ) );
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;

	fb_hDevTcpFormatService( service, sizeof( service ), tcp_proto->port );
	res = getaddrinfo( tcp_proto->host, service, &hints, &result );
	if( res != 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	for( it = result; it != NULL; it = it->ai_next ) {
		socklen_t address_length;
		hSocket = fb_hArosTcpCreateSocket( it->ai_family,
		                                  it->ai_socktype,
		                                  it->ai_protocol );
		if( FB_TCP_SOCKET_ERROR( hSocket ) )
			continue;

		fb_hDevTcpApplySocketOptions( hSocket, tcp_proto->timeout, FB_TCP_SOCKET_CLIENT );

		if( !fb_hArosTcpAddressLength( it->ai_addrlen, &address_length ) ) {
			FB_TCP_CLOSESOCKET( hSocket );
			continue;
		}
		if( connect( hSocket, it->ai_addr, address_length ) == 0 ) {
			fb_hDevTcpSetNoDelay( hSocket );
			*hSocketOut = hSocket;
			freeaddrinfo( result );
			return FB_RTERROR_OK;
		}

		FB_TCP_CLOSESOCKET( hSocket );
	}

	freeaddrinfo( result );
	return fb_ErrorSetNum( FB_RTERROR_FILEIO );
}

static int fb_hDevTcpCreateServerSocket( DEV_TCP_PROTOCOL *tcp_proto, FB_TCP_SOCKET *hSocketOut )
{
	struct addrinfo hints;
	struct addrinfo *result;
	struct addrinfo *it;
	FB_TCP_SOCKET hSocket = FB_TCP_INVALID_SOCKET;
	char service[16];
	int res;

	memset( &hints, 0, sizeof( hints ) );
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;

	fb_hDevTcpFormatService( service, sizeof( service ), tcp_proto->port );
	res = getaddrinfo( (*tcp_proto->host != '\0') ? tcp_proto->host : NULL, service, &hints, &result );
	if( res != 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	for( it = result; it != NULL; it = it->ai_next ) {
		int yes = 1;
		socklen_t address_length;
		hSocket = fb_hArosTcpCreateSocket( it->ai_family,
		                                  it->ai_socktype,
		                                  it->ai_protocol );
		if( FB_TCP_SOCKET_ERROR( hSocket ) )
			continue;

		setsockopt( hSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof( yes ) );
		fb_hDevTcpApplySocketOptions( hSocket, tcp_proto->timeout, FB_TCP_SOCKET_LISTENER );

		if( !fb_hArosTcpAddressLength( it->ai_addrlen, &address_length ) ||
		    (bind( hSocket, it->ai_addr, address_length ) != 0) ) {
			FB_TCP_CLOSESOCKET( hSocket );
			continue;
		}

		if( listen( hSocket, (int)tcp_proto->backlog ) != 0 ) {
			FB_TCP_CLOSESOCKET( hSocket );
			continue;
		}

		*hSocketOut = hSocket;
		freeaddrinfo( result );
		return FB_RTERROR_OK;
	}

	freeaddrinfo( result );
	return fb_ErrorSetNum( FB_RTERROR_FILEIO );
}

static int fb_hDevTcpPeekState( DEV_TCP_INFO *info )
{
	fd_set set;
	struct timeval tv;
	int res;
	int err = 0;
	char ch;

	if( info == NULL || FB_TCP_SOCKET_ERROR( info->hSocket ) )
		return -1;

	if( info->is_closed )
		return -1;

	FD_ZERO( &set );
	if( !fb_hArosTcpSetFd( info->hSocket, &set ) ) {
		info->is_closed = TRUE;
		return -1;
	}
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FB_UNLOCK();
	res = FB_TCP_SELECT( info->hSocket + 1, &set, NULL, NULL, &tv );

	if( res < 0 ) {
		FB_LOCK();
		info->is_closed = TRUE;
		return -1;
	}

	if( res == 0 ) {
		FB_LOCK();
		return 0;
	}

	res = recv( info->hSocket, &ch, 1, MSG_PEEK );
	err = fb_hDevTcpSocketError( res );
	FB_LOCK();

	if( res > 0 )
		return 1;

	if( res == 0 ) {
		info->is_closed = TRUE;
		return -1;
	}

	if( FB_TCP_WOULDBLOCK( err ) )
		return 0;

	info->is_closed = TRUE;
	return -1;
}

static size_t fb_hDevTcpReadPeekByte( DEV_TCP_INFO *info, char *buffer, size_t length )
{
	(void)info;
	(void)buffer;
	(void)length;
	return 0;
}

static int fb_hDevTcpEocState( DEV_TCP_INFO *info )
{
	return fb_hDevTcpPeekState( info );
}

static int fb_hDevTcpSendAll( DEV_TCP_INFO *info, const char *buffer, size_t length )
{
	size_t total = 0;

	while( total < length ) {
		size_t chunk = MIN( length - total, (size_t)INT_MAX );
		int sent;
		int err;
		int flags = 0;

		#if defined(MSG_NOSIGNAL)
			flags = MSG_NOSIGNAL;
		#endif

		sent = fb_hDevTcpSendUnlocked( info, buffer + total, chunk, flags, &err );
		if( sent <= 0 ) {
			info->is_closed = TRUE;
			return fb_ErrorSetNum( FB_RTERROR_FILEIO );
		}

		total += (size_t)sent;
	}

	return FB_RTERROR_OK;
}

static void fb_hDevTcpShutdownConnectedSocket( DEV_TCP_INFO *info )
{
	unsigned int timeout = 1000;
	char buffer[256];

	if( info == NULL || info->is_server || FB_TCP_SOCKET_ERROR( info->hSocket ) )
		return;

	if( info->timeout != 0 )
		timeout = info->timeout;

	FB_UNLOCK();
	shutdown( info->hSocket, FB_TCP_SHUT_WR );
	FB_LOCK();

	while( timeout > 0 ) {
		fd_set set;
		struct timeval tv;
		unsigned int slice = MIN( timeout, 100u );
		int res;

		FD_ZERO( &set );
		if( !fb_hArosTcpSetFd( info->hSocket, &set ) ) {
			info->is_closed = TRUE;
			break;
		}
		tv.tv_sec = slice / 1000;
		tv.tv_usec = (suseconds_t)((slice % 1000u) * 1000u);

		FB_UNLOCK();
		res = FB_TCP_SELECT( info->hSocket + 1, &set, NULL, NULL, &tv );

		if( res <= 0 ) {
			FB_LOCK();
			break;
		}

		res = recv( info->hSocket, buffer, sizeof( buffer ), 0 );
		FB_LOCK();
		if( res <= 0 )
			break;

		timeout -= slice;
	}
}

static int fb_DevTcpClose( FB_FILE *handle )
{
	int res = FB_RTERROR_OK;
	DEV_TCP_INFO *info;

	info = (DEV_TCP_INFO*)handle->opaque;
	if( info != NULL ) {
		if( FB_TCP_SOCKET_ERROR( info->hSocket ) == FALSE ) {
			int close_res;
			int socket_closed = FALSE;

			fb_hDevTcpShutdownConnectedSocket( info );
			if( socket_closed == FALSE ) {
				FB_UNLOCK();
				close_res = FB_TCP_CLOSESOCKET( info->hSocket );
				FB_LOCK();
				if( close_res != 0 )
					res = fb_ErrorSetNum( FB_RTERROR_FILEIO );
			}
		}

		if( res == FB_RTERROR_OK ) {
			free( info->pszDevice );
			free( info );
		}
	}

	return res;
}

static int fb_DevTcpWrite( FB_FILE *handle, const void *value, size_t valuelen )
{
	int res;
	DEV_TCP_INFO *info;

	info = (DEV_TCP_INFO*)handle->opaque;
	if( info == NULL || info->is_closed ) {
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	res = fb_hDevTcpSendAll( info, (const char *)value, valuelen );
	return res;
}

static int fb_DevTcpWriteWstr( FB_FILE *handle, const FB_WCHAR *value, size_t valuelen )
{
	return fb_DevTcpWrite( handle, (const void*)value, valuelen * sizeof( FB_WCHAR ) );
}

static int fb_DevTcpRead( FB_FILE *handle, void *value, size_t *pValuelen )
{
	int res = FB_RTERROR_OK;
	DEV_TCP_INFO *info;
	size_t length;
	size_t buffered;

	info = (DEV_TCP_INFO*)handle->opaque;
	if( info == NULL || pValuelen == NULL ) {
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	length = *pValuelen;
	buffered = fb_hDevTcpReadPeekByte( info, (char *)value, length );

	if( buffered == length ) {
		*pValuelen = buffered;
		return FB_RTERROR_OK;
	}

	if( info->is_closed ) {
		*pValuelen = buffered;
		return FB_RTERROR_OK;
	}

	if( length == 0 ) {
		*pValuelen = 0;
		return FB_RTERROR_OK;
	}

	{
		int err = 0;
		int bytes = fb_hDevTcpRecvUnlocked( info,
		                                    ((char *)value) + buffered,
		                                    length - buffered,
		                                    0,
		                                    &err );
		if( bytes > 0 ) {
			*pValuelen = buffered + (size_t)bytes;
		} else if( bytes == 0 ) {
			info->is_closed = TRUE;
			*pValuelen = buffered;
		} else if( FB_TCP_WOULDBLOCK( err ) ) {
			*pValuelen = buffered;
		} else {
			info->is_closed = TRUE;
			*pValuelen = buffered;
			if( buffered == 0 )
				res = fb_ErrorSetNum( FB_RTERROR_FILEIO );
		}
	}

	return res;
}

static int fb_DevTcpReadWstr( FB_FILE *handle, FB_WCHAR *value, size_t *pValuelen )
{
	size_t len = *pValuelen * sizeof( FB_WCHAR );
	return fb_DevTcpRead( handle, (void *)value, &len );
}

static int fb_DevTcpTell( FB_FILE *handle, fb_off_t *pOffset )
{
	int res = FB_RTERROR_OK;
	DEV_TCP_INFO *info;

	DBG_ASSERT( pOffset != NULL );

	info = (DEV_TCP_INFO*)handle->opaque;
	if( info == NULL ) {
		res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	} else {
#if   FB_TCP_CAN_QUERY_BYTES == FALSE
		/*
			libogc exposes net_ioctl(), but currently only FIONBIO is wired
			through to IOS.  LOC() on a Wii TCP handle therefore cannot report
			buffered bytes without consuming data.  Returning zero is safer
			than pretending FIONREAD worked.
		*/
		*pOffset = 0;
#else
		int bytes = 0;
		if( FB_TCP_IOCTL( info->hSocket, FIONREAD, &bytes ) != 0 ) {
			res = fb_ErrorSetNum( FB_RTERROR_FILEIO );
		}
		*pOffset = bytes;
#endif
	}

	return res;
}

static int fb_DevTcpEof( FB_FILE *handle )
{
	int res;
	DEV_TCP_INFO *info;

	info = (DEV_TCP_INFO*)handle->opaque;
	res = (fb_hDevTcpPeekState( info ) == 1 ? FB_FALSE : FB_TRUE);

	return res;
}

static int fb_DevTcpServerEof( FB_FILE *handle )
{
	(void)handle;
	return FB_TRUE;
}

static int fb_hDevTcpReadPutbackByte( FB_FILE *handle, char *ch )
{
	size_t bytes;

	if( handle->putback_size == 0 )
		return FALSE;

	if( handle->encod == FB_FILE_ENCOD_ASCII || handle->putback_size < sizeof( FB_WCHAR ) ) {
		*ch = handle->putback_buffer[0];
		bytes = 1;
	} else {
		FB_WCHAR wc;

		memcpy( &wc, handle->putback_buffer, sizeof( wc ) );
		*ch = (char)wc;
		bytes = sizeof( wc );
	}

	handle->putback_size -= bytes;
	if( handle->putback_size != 0 ) {
		memmove( handle->putback_buffer,
		         handle->putback_buffer + bytes,
		         handle->putback_size );
	}

	return TRUE;
}

static int fb_hDevTcpReadByte( FB_FILE *handle, DEV_TCP_INFO *info, char *ch, size_t *read_len )
{
	*read_len = 0;

	if( fb_hDevTcpReadPutbackByte( handle, ch ) ) {
		*read_len = 1;
		return FB_RTERROR_OK;
	}

	if( fb_hDevTcpReadPeekByte( info, ch, 1 ) ) {
		*read_len = 1;
		return FB_RTERROR_OK;
	}

	if( info->is_closed )
		return FB_RTERROR_OK;

	{
		int err = 0;
		int bytes = fb_hDevTcpRecvUnlocked( info, ch, 1, 0, &err );

		if( bytes > 0 ) {
			*read_len = 1;
		} else if( bytes == 0 ) {
			info->is_closed = TRUE;
		} else if( FB_TCP_WOULDBLOCK( err ) ) {
			*read_len = 0;
		} else {
			info->is_closed = TRUE;
			return fb_ErrorSetNum( FB_RTERROR_FILEIO );
		}
	}

	return FB_RTERROR_OK;
}

static void fb_hDevTcpAppendChunk( FBSTRING_ACCESS *dst, const char *buffer, ssize_t len )
{
	FBSTRING_ACCESS *src;

	if( (len <= 0) || (len >= SSIZE_MAX) )
		return;

	src = fb_StrAllocTempDescF( buffer, len + 1 );
	if( dst->data == NULL ) {
		fb_StrAssign( dst, -1, src, -1, FALSE );
	} else {
		fb_StrConcatAssign( dst, -1, src, -1, FALSE );
	}
}

static int fb_DevTcpReadLine( FB_FILE *handle, FBSTRING_ACCESS *dst )
{
	DEV_TCP_INFO *info = (DEV_TCP_INFO*)handle->opaque;
	char buffer[1024];
	ssize_t len = 0;
	int res = FB_RTERROR_OK;

	fb_StrDelete( dst );

	if( info == NULL )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	do {
		char ch;
		size_t read_len = 0;

		res = fb_hDevTcpReadByte( handle, info, &ch, &read_len );
		if( res != FB_RTERROR_OK || read_len == 0 )
			break;

		if( ch == '\r' ) {
			res = fb_hDevTcpReadByte( handle, info, &ch, &read_len );
			if( res == FB_RTERROR_OK && read_len == 1 ) {
				if( ch != '\n' )
					fb_FilePutBackEx( handle, &ch, 1 );
			}
			break;
		}

		if( ch == '\n' )
			break;

		buffer[len++] = ch;
		if( len == (ssize_t)(sizeof( buffer ) - 1) ) {
			buffer[len] = '\0';
			fb_hDevTcpAppendChunk( dst, buffer, len );
			len = 0;
		}
	} while( TRUE );

	if( len != 0 ) {
		buffer[len] = '\0';
		fb_hDevTcpAppendChunk( dst, buffer, len );
	}

	return res;
}

static int fb_DevTcpReadLineWstr( FB_FILE *handle, FB_WCHAR *dst, ssize_t dst_chars )
{
	int res;
	FBSTRING temp = { 0, 0, 0 };

	res = fb_DevTcpReadLine( handle, &temp );
	if( res == FB_RTERROR_OK )
		fb_WstrAssignFromA( dst, dst_chars, (void *)&temp, -1 );

	fb_StrDelete( &temp );
	return res;
}

static int fb_hDevTcpFinishOpen( FB_FILE *handle, DEV_TCP_PROTOCOL *tcp_proto, FB_TCP_SOCKET hSocket, FB_FILE_HOOKS *hooks, int type, int is_server, const char *pszDevice )
{
	DEV_TCP_INFO *info;

	info = fb_hDevTcpAllocInfo( hSocket, pszDevice, tcp_proto, is_server );
	if( info == NULL ) {
		FB_TCP_CLOSESOCKET( hSocket );
		return fb_ErrorSetNum( FB_RTERROR_OUTOFMEM );
	}

	fb_hDevTcpNormalizeOpenMode( handle );
	handle->hooks = hooks;
	handle->opaque = info;
	handle->type = type;
	handle->size = -1;

	return FB_RTERROR_OK;
}

static int fb_hDevTcpOpenCommon( FB_FILE *handle, const char *filename, size_t filename_len, int is_server )
{
	DEV_TCP_PROTOCOL *tcp_proto = NULL;
	FB_TCP_SOCKET hSocket = FB_TCP_INVALID_SOCKET;
	int res;

	if( fb_hDevTcpInit() != FB_RTERROR_OK )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	if( fb_DevTcpParseProtocol( &tcp_proto, filename, filename_len, is_server ) == FALSE ) {
		free( tcp_proto );
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	FB_LOCK();

	res = (is_server ? fb_hDevTcpCreateServerSocket( tcp_proto, &hSocket )
	                 : fb_hDevTcpCreateConnectedSocket( tcp_proto, &hSocket ));
	if( res == FB_RTERROR_OK ) {
		res = fb_hDevTcpFinishOpen( handle, tcp_proto, hSocket,
			is_server ? &hooks_dev_tcp_server : &hooks_dev_tcp,
			is_server ? FB_FILE_TYPE_TCPSERVER : FB_FILE_TYPE_TCP,
			is_server,
			filename );
	}

	FB_UNLOCK();

	free( tcp_proto );
	return res;
}

int fb_DevTcpOpen( FB_FILE *handle, const char *filename, size_t filename_len )
{
	return fb_hDevTcpOpenCommon( handle, filename, filename_len, FALSE );
}

int fb_DevTcpOpenServer( FB_FILE *handle, const char *filename, size_t filename_len )
{
	return fb_hDevTcpOpenCommon( handle, filename, filename_len, TRUE );
}

int fb_DevTcpAcceptHandle( FB_FILE *server_handle, FB_FILE *client_handle )
{
	DEV_TCP_INFO *server_info;
	FB_TCP_SOCKET hSocket;
	int res;

	if( server_handle == NULL || client_handle == NULL )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	server_info = (DEV_TCP_INFO*)server_handle->opaque;
	if( server_info == NULL || server_handle->type != FB_FILE_TYPE_TCPSERVER )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	hSocket = (FB_TCP_SOCKET)accept( server_info->hSocket, NULL, NULL );
	if( FB_TCP_SOCKET_ERROR( hSocket ) )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	res = fb_hDevTcpApplySocketOptions( hSocket, server_info->timeout, FB_TCP_SOCKET_ACCEPTED );
	if( res != FB_RTERROR_OK ) {
		FB_TCP_CLOSESOCKET( hSocket );
		return res;
	}

	fb_hDevTcpSetNoDelay( hSocket );

	/*
		fb_TcpAccept() reserves this file number before doing the blocking
		accept().  Keep the final handle transition under FB_LOCK() too.  If the
		placeholder were cleared without the lock, another thread could observe
		the slot as free and reuse it via FREEFILE while the accepted TCP handle
		was still being installed.
	*/
	FB_LOCK();
	memset( client_handle, 0, sizeof( FB_FILE ) );
	client_handle->mode = FB_FILE_MODE_BINARY;
	client_handle->access = FB_FILE_ACCESS_READWRITE;
	client_handle->lock = FB_FILE_LOCK_SHARED;
	client_handle->encod = FB_FILE_ENCOD_DEFAULT;
	client_handle->size = -1;

	{
		DEV_TCP_PROTOCOL dummy;
		memset( &dummy, 0, sizeof( dummy ) );
		dummy.timeout = server_info->timeout;
		res = fb_hDevTcpFinishOpen( client_handle, &dummy, hSocket, &hooks_dev_tcp, FB_FILE_TYPE_TCP, FALSE, "TCP" );
	}
	FB_UNLOCK();

	return res;
}

int fb_DevTcpEocEx( FB_FILE *handle )
{
	DEV_TCP_INFO *info;
	int res;

	if( handle == NULL )
		return FB_TRUE;

	if( handle->type == FB_FILE_TYPE_TCPSERVER )
		return FB_FALSE;

	info = (DEV_TCP_INFO*)handle->opaque;
	if( info == NULL )
		return FB_TRUE;

	FB_LOCK();
	res = (fb_hDevTcpEocState( info ) < 0);
	FB_UNLOCK();

	return res ? FB_TRUE : FB_FALSE;
}

#endif /* DISABLE_TCP */

/* end of drivers/aros/dev_tcp.c */
