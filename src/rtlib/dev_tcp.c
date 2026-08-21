/* TCP device */

#include "fb.h"
#include <strings.h>
#include "dev_tcp_private.h"

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

#if !defined(HOST_DOS) && !defined(HOST_JS)
	#if defined(HOST_WIN32) && !defined(HOST_CYGWIN)
		#include <ws2tcpip.h>
	#elif defined(HOST_XBOX)
		#include <errno.h>
		#include <lwip/netdb.h>
		#include <lwip/sockets.h>
		#include <nxdk/net.h>
	#elif defined(HOST_WII)
		#include <errno.h>
		#include <network.h>
	#else
		#include <errno.h>
		#include <netdb.h>
		#include <sys/ioctl.h>
		#ifdef HOST_SOLARIS
			#include <sys/filio.h>
		#endif
		#include <sys/select.h>
		#include <sys/socket.h>
		#include <sys/types.h>
		#include <netinet/in.h>
		#include <netinet/tcp.h>
		#include <arpa/inet.h>
		#include <unistd.h>
	#endif
#endif

#if defined(HOST_DOS) || defined(HOST_JS)
	#define FB_TCP_CLOSESOCKET(s) (0)
	#define FB_TCP_SOCKET_ERROR(s) TRUE
	#define FB_TCP_ERRNO() 0
	#define FB_TCP_WOULDBLOCK(err) FALSE
	#define FB_TCP_SHUT_WR 1
	#define FB_TCP_SHUT_RDWR 2
#elif defined(HOST_WIN32) && !defined(HOST_CYGWIN)
	#define FB_TCP_CLOSESOCKET(s) closesocket( s )
	#define FB_TCP_SOCKET_ERROR(s) ((s) == FB_TCP_INVALID_SOCKET)
	#define FB_TCP_ERRNO() WSAGetLastError( )
	#define FB_TCP_WOULDBLOCK(err) \
		((err) == WSAEWOULDBLOCK || (err) == WSAETIMEDOUT)
	#define FB_TCP_SHUT_WR SD_SEND
	#define FB_TCP_SHUT_RDWR SD_BOTH
	#define FB_TCP_SELECT(n, r, w, e, t) select( 0, r, w, e, t )
	#define FB_TCP_IOCTL(s, cmd, argp) ioctlsocket( s, cmd, argp )
	#define FB_TCP_RECV(s, b, l, f) recv( s, b, l, f )
	#define FB_TCP_SEND(s, b, l, f) send( s, b, l, f )
	#define FB_TCP_SETSOCKOPT(s, l, o, v, n) setsockopt( s, l, o, v, n )
	#define FB_TCP_CAN_QUERY_BYTES TRUE
#elif defined(HOST_WII)
	#define FB_TCP_CLOSESOCKET(s) net_close( s )
	#define FB_TCP_SOCKET_ERROR(s) ((s) < 0)
	#define FB_TCP_ERRNO() errno
	#define FB_TCP_WOULDBLOCK(err) \
		((err) == EAGAIN || (err) == EWOULDBLOCK || \
		 (err) == -EAGAIN || (err) == -EWOULDBLOCK)
	#define FB_TCP_SHUT_WR SHUT_WR
	#define FB_TCP_SHUT_RDWR SHUT_RDWR
	#define FB_TCP_SELECT(n, r, w, e, t) net_select( n, r, w, e, t )
	#define FB_TCP_IOCTL(s, cmd, argp) net_ioctl( s, cmd, argp )
	#define FB_TCP_RECV(s, b, l, f) recv( s, b, l, f )
	#define FB_TCP_SEND(s, b, l, f) send( s, b, l, f )
	#define FB_TCP_SETSOCKOPT(s, l, o, v, n) setsockopt( s, l, o, v, n )
	#define FB_TCP_CAN_QUERY_BYTES FALSE
#else
	#define FB_TCP_CLOSESOCKET(s) close( s )
	#define FB_TCP_SOCKET_ERROR(s) ((s) == FB_TCP_INVALID_SOCKET)
	#define FB_TCP_ERRNO() errno
	#define FB_TCP_WOULDBLOCK(err) ((err) == EAGAIN || (err) == EWOULDBLOCK)
	#define FB_TCP_SHUT_WR SHUT_WR
	#define FB_TCP_SHUT_RDWR SHUT_RDWR
	#define FB_TCP_SELECT(n, r, w, e, t) select( n, r, w, e, t )
	#define FB_TCP_IOCTL(s, cmd, argp) ioctl( s, cmd, argp )
	#define FB_TCP_RECV(s, b, l, f) recv( s, b, l, f )
	#define FB_TCP_SEND(s, b, l, f) send( s, b, l, f )
	#define FB_TCP_SETSOCKOPT(s, l, o, v, n) setsockopt( s, l, o, v, n )
	#define FB_TCP_CAN_QUERY_BYTES TRUE
#endif

#define FB_TCP_SOCKET_CLIENT 0
#define FB_TCP_SOCKET_LISTENER 1
#define FB_TCP_SOCKET_ACCEPTED 2

static int fb_DevTcpClose( FB_FILE *handle );
static int fb_DevTcpRead( FB_FILE *handle, void *value, size_t *pValuelen );
static int fb_DevTcpReadWstr( FB_FILE *handle, FB_WCHAR *value, size_t *pValuelen );
static int fb_DevTcpReadLine( FB_FILE *handle, FBSTRING *dst );
static int fb_DevTcpReadLineWstr( FB_FILE *handle, FB_WCHAR *dst, ssize_t dst_chars );
static int fb_DevTcpWrite( FB_FILE *handle, const void *value, size_t valuelen );
static int fb_DevTcpWriteWstr( FB_FILE *handle, const FB_WCHAR *value, size_t valuelen );
static int fb_DevTcpTell( FB_FILE *handle, fb_off_t *pOffset );
static int fb_DevTcpEof( FB_FILE *handle );
static int fb_DevTcpServerEof( FB_FILE *handle );
#if !defined(HOST_DOS) && !defined(HOST_JS) && !defined(HOST_WII)
static void fb_hDevTcpFormatService( char *service, size_t service_len, unsigned int port );
#endif

static int fb_hDevTcpSocketError( int result )
{
#if defined(HOST_WII)
	int err = FB_TCP_ERRNO();

	/*
		libogc socket calls may return the negative error value directly
		instead of setting errno.  Keep that result so transient states such
		as -EAGAIN remain distinguishable from a closed connection.
	*/
	if( result < 0 ) {
		if( FB_TCP_WOULDBLOCK( result ) )
			return result;
		if( err == 0 )
			return result;
	}

	return err;
#else
	(void)result;
	return FB_TCP_ERRNO();
#endif
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

#if defined(HOST_WIN32) && !defined(HOST_CYGWIN)
static void fb_hDevTcpShutdownWinsock( void )
{
	WSACleanup();
}

static int fb_hDevTcpInit( void )
{
	static int is_init = FALSE;
	static int init_result = FB_RTERROR_OK;

	if( is_init )
		return init_result;

	FB_LOCK();
	if( is_init == FALSE ) {
		WSADATA wsaData;
		if( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 ) {
			init_result = fb_ErrorSetNum( FB_RTERROR_FILEIO );
		} else {
			atexit( fb_hDevTcpShutdownWinsock );
		}
		is_init = TRUE;
	}
	FB_UNLOCK();

	return init_result;
}
#elif defined(HOST_XBOX)
static void fb_hDevTcpShutdownXbox( void )
{
	nxNetShutdown();
}

static int fb_hDevTcpInit( void )
{
	static int is_init = FALSE;
	static int init_result = FB_RTERROR_OK;

	if( is_init )
		return init_result;

	FB_LOCK();
	if( is_init == FALSE ) {
		/*
			nxdk keeps TCP/IP in its optional lwIP network library.  Programs
			do not need to opt in manually; the FreeBASIC TCP device starts it
			the first time OPEN TCP or OPEN TCP SERVER is used.
		*/
		if( nxNetInit( NULL ) < 0 ) {
			init_result = fb_ErrorSetNum( FB_RTERROR_FILEIO );
		} else {
			atexit( fb_hDevTcpShutdownXbox );
		}
		is_init = TRUE;
	}
	FB_UNLOCK();

	return init_result;
}
#elif defined(HOST_WII)
static void fb_hDevTcpShutdownWii( void )
{
	net_deinit();
}

static int fb_hDevTcpInit( void )
{
	static int is_init = FALSE;
	static int init_result = FB_RTERROR_OK;

	if( is_init )
		return init_result;

	FB_LOCK();
	if( is_init == FALSE ) {
		char local_ip[16];
		char netmask[16];
		char gateway[16];

		/*
			libogc exposes the Wii network stack through network.h.  Its
			BSD-style socket wrappers are present, but the stack is not ready
			until if_config() has brought IOS networking up and obtained an
			address.  Keep that startup lazy so ordinary console/graphics
			programs do not block on DHCP.
		*/
		memset( local_ip, 0, sizeof( local_ip ) );
		memset( netmask, 0, sizeof( netmask ) );
		memset( gateway, 0, sizeof( gateway ) );
		if( if_config( local_ip, netmask, gateway, TRUE, 20 ) < 0 ) {
			init_result = fb_ErrorSetNum( FB_RTERROR_FILEIO );
		} else {
			atexit( fb_hDevTcpShutdownWii );
		}
		is_init = TRUE;
	}
	FB_UNLOCK();

	return init_result;
}
#else
static int fb_hDevTcpInit( void )
{
	return FB_RTERROR_OK;
}
#endif

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

#if defined(HOST_WII)
	if( is_server == FALSE ) {
		info->wii_pump = fb_WiiTcpPumpCreate( hSocket );
		if( info->wii_pump == NULL ) {
			free( info->pszDevice );
			free( info );
			return NULL;
		}
	}
#endif

	return info;
}

static int fb_hDevTcpApplySocketOptions( FB_TCP_SOCKET hSocket, unsigned int timeout, int socket_role )
{
#if defined(HOST_DOS) || defined(HOST_JS)
	(void)hSocket;
	(void)timeout;
	(void)socket_role;
	return FB_RTERROR_ILLEGALFUNCTIONCALL;
#else
	#if defined(SO_NOSIGPIPE)
		{
			int value = 1;
			FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_NOSIGPIPE, (const char *)&value, sizeof( value ) );
		}
	#endif

	#if defined(HOST_WII)
		{
			int buffer_size = 32768;
			int low_water = 1;

			/*
				Duel-era FreeBASIC games can send a complete world snapshot as
				hundreds of small PUT operations.  The Wii TCP defaults are
				small enough that the local client/server pair can stall while
				one side is still draining that startup packet.
			*/
			FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_SNDBUF, (const char *)&buffer_size, sizeof( buffer_size ) );
			FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_RCVBUF, (const char *)&buffer_size, sizeof( buffer_size ) );

			/*
				Old code often polls EOF and then reads a single byte.  Keep
				libogc from waiting for a larger low-water amount when only the
				tail of a logical packet remains.
			*/
			FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_SNDLOWAT, (const char *)&low_water, sizeof( low_water ) );
			FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_RCVLOWAT, (const char *)&low_water, sizeof( low_water ) );
		}
	#endif

	if( timeout != 0 ) {
		#if defined(HOST_WIN32) && !defined(HOST_CYGWIN)
			DWORD value = (DWORD)timeout;
			DWORD receive_value = value;
			DWORD send_value = (socket_role == FB_TCP_SOCKET_LISTENER) ? value : 0;
			/*
				Connected reads retain the protocol timeout so a GET following
				EOF cannot block forever if readiness changes between the two
				calls. The TCP device reports that timeout as an idle read.
			*/
			FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&receive_value, sizeof( receive_value ) );
			FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&send_value, sizeof( send_value ) );
		#else
			struct timeval value;

			value.tv_sec = timeout / 1000;
			value.tv_usec = (timeout % 1000) * 1000;

			#if !defined(HOST_WII)
			FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&value, sizeof( value ) );
			#else
				/*
					On Wii, connected sockets are drained by the receive pump.
					Only listening sockets keep the BASIC timeout so TCP ACCEPT
					can be used as a polling operation.
				*/
				if( socket_role == FB_TCP_SOCKET_LISTENER )
					FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&value, sizeof( value ) );
			#endif

			#if !defined(HOST_WII)
				/*
				OPEN TCP SERVER timeout is useful on the listening socket
				because old threaded games poll TCP ACCEPT from their main
				server loop. Connected sockets retain a receive timeout so
				EOF followed by a read remains bounded. Sends stay blocking
				because a complete framed packet may be larger than one TCP
				write.
				*/
				if( socket_role == FB_TCP_SOCKET_LISTENER )
					FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&value, sizeof( value ) );
				else {
					struct timeval no_timeout;

					/* Connected writes must not time out while a peer drains a
					   bounded but potentially large frame. */
					no_timeout.tv_sec = 0;
					no_timeout.tv_usec = 0;
					FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&no_timeout, sizeof( no_timeout ) );
				}
			#else
				/*
					libogc applies short send timeouts aggressively.  Old
					threaded games such as Duel use timeout=1 on the listening
					socket so their server loop can poll, but the accepted data
					socket still needs blocking sends while the peer drains a
					large startup packet.  send() releases FB_LOCK(), so the
					receiving BASIC thread can continue running.
				*/
			#endif
		#endif
	}

	return FB_RTERROR_OK;
#endif
}

#if !defined(HOST_DOS) && !defined(HOST_JS) && !defined(HOST_WII)
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
#endif

#if defined(HOST_WII)
static int fb_hDevTcpBuildWiiAddress( const char *host, unsigned int port, int passive, struct sockaddr_in *addr )
{
	u32 raw_addr;
	struct hostent *hostent;

	if( addr == NULL )
		return FALSE;

	memset( addr, 0, sizeof( *addr ) );
	addr->sin_len = sizeof( *addr );
	addr->sin_family = AF_INET;
	addr->sin_port = htons( (uint16_t)port );

	if( host == NULL || *host == '\0' ) {
		addr->sin_addr.s_addr = htonl( passive ? INADDR_ANY : INADDR_LOOPBACK );
		return TRUE;
	}

	if( strcasecmp( host, "localhost" ) == 0 ) {
		addr->sin_addr.s_addr = htonl( INADDR_LOOPBACK );
		return TRUE;
	}

	raw_addr = inet_addr( host );
	if( raw_addr != INADDR_NONE || strcmp( host, "255.255.255.255" ) == 0 ) {
		addr->sin_addr.s_addr = raw_addr;
		return TRUE;
	}

	hostent = net_gethostbyname( host );
	if( hostent == NULL )
		return FALSE;

	if( hostent->h_addrtype != AF_INET )
		return FALSE;

	if( hostent->h_length != (int)sizeof( struct in_addr ) )
		return FALSE;

	if( hostent->h_addr_list == NULL || hostent->h_addr_list[0] == NULL )
		return FALSE;

	memcpy( &addr->sin_addr, hostent->h_addr_list[0], sizeof( struct in_addr ) );
	return TRUE;
}
#endif

#if !defined(HOST_DOS) && !defined(HOST_JS)
/*
	TCP device hooks are called by the generic file layer while FB_LOCK()
	is held.  Blocking socket syscalls must not keep that lock, because a
	thread waiting for TCP input can otherwise prevent another FB thread in
	the same process from writing the data it is waiting for.
*/
#if !defined(HOST_WII)
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
#endif

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

#if !defined(HOST_DOS) && !defined(HOST_JS)
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

static void fb_hDevTcpSetConnectedNonBlocking( FB_TCP_SOCKET hSocket )
{
	/*
		EOF is the readiness probe for a connected BASIC TCP handle. A second
		thread or a peer-side packet boundary can change readiness before GET
		runs, so connected reads must return a transient no-data result rather
		than block at the socket syscall.
	*/
#if defined(HOST_WIN32) && !defined(HOST_CYGWIN)
	u_long mode = 1;
	FB_TCP_IOCTL( hSocket, FIONBIO, &mode );
#elif !defined(HOST_DOS) && !defined(HOST_JS) && !defined(HOST_WII)
	int mode = 1;
	FB_TCP_IOCTL( hSocket, FIONBIO, &mode );
#else
	(void)hSocket;
#endif
}
#endif
#endif

static int fb_hDevTcpCreateConnectedSocket( DEV_TCP_PROTOCOL *tcp_proto, FB_TCP_SOCKET *hSocketOut )
{
#if defined(HOST_DOS) || defined(HOST_JS)
	(void)tcp_proto;
	(void)hSocketOut;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#elif defined(HOST_WII)
	struct sockaddr_in addr;
	FB_TCP_SOCKET hSocket;

	if( fb_hDevTcpBuildWiiAddress( tcp_proto->host, tcp_proto->port, FALSE, &addr ) == FALSE )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	hSocket = (FB_TCP_SOCKET)socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( FB_TCP_SOCKET_ERROR( hSocket ) )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	fb_hDevTcpApplySocketOptions( hSocket, tcp_proto->timeout, FB_TCP_SOCKET_CLIENT );

	if( connect( hSocket, (struct sockaddr *)&addr, sizeof( addr ) ) == 0 ) {
		fb_hDevTcpSetConnectedNonBlocking( hSocket );
		fb_hDevTcpSetNoDelay( hSocket );
		*hSocketOut = hSocket;
		return FB_RTERROR_OK;
	}

	FB_TCP_CLOSESOCKET( hSocket );
	return fb_ErrorSetNum( FB_RTERROR_FILEIO );
#else
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
		hSocket = (FB_TCP_SOCKET)socket( it->ai_family, it->ai_socktype, it->ai_protocol );
		if( FB_TCP_SOCKET_ERROR( hSocket ) )
			continue;

		fb_hDevTcpApplySocketOptions( hSocket, tcp_proto->timeout, FB_TCP_SOCKET_CLIENT );

		if( connect( hSocket, it->ai_addr, (int)it->ai_addrlen ) == 0 ) {
			fb_hDevTcpSetConnectedNonBlocking( hSocket );
			fb_hDevTcpSetNoDelay( hSocket );
			*hSocketOut = hSocket;
			freeaddrinfo( result );
			return FB_RTERROR_OK;
		}

		FB_TCP_CLOSESOCKET( hSocket );
	}

	freeaddrinfo( result );
	return fb_ErrorSetNum( FB_RTERROR_FILEIO );
#endif
}

static int fb_hDevTcpCreateServerSocket( DEV_TCP_PROTOCOL *tcp_proto, FB_TCP_SOCKET *hSocketOut )
{
#if defined(HOST_DOS) || defined(HOST_JS)
	(void)tcp_proto;
	(void)hSocketOut;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#elif defined(HOST_WII)
	struct sockaddr_in addr;
	FB_TCP_SOCKET hSocket;
	int yes = 1;

	if( fb_hDevTcpBuildWiiAddress( tcp_proto->host, tcp_proto->port, TRUE, &addr ) == FALSE )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	hSocket = (FB_TCP_SOCKET)socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( FB_TCP_SOCKET_ERROR( hSocket ) )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof( yes ) );
	fb_hDevTcpApplySocketOptions( hSocket, tcp_proto->timeout, FB_TCP_SOCKET_LISTENER );

	if( bind( hSocket, (struct sockaddr *)&addr, sizeof( addr ) ) != 0 ) {
		FB_TCP_CLOSESOCKET( hSocket );
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	if( listen( hSocket, (int)tcp_proto->backlog ) != 0 ) {
		FB_TCP_CLOSESOCKET( hSocket );
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	*hSocketOut = hSocket;
	return FB_RTERROR_OK;
#else
	struct addrinfo hints;
	struct addrinfo *result;
	struct addrinfo *it;
	FB_TCP_SOCKET hSocket = FB_TCP_INVALID_SOCKET;
	char service[16];
	int res;

	memset( &hints, 0, sizeof( hints ) );
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
#if defined(HOST_CYGWIN) || defined(HOST_HAIKU) || defined(HOST_FREEBSD) || \
    defined(HOST_NETBSD) || defined(HOST_OPENBSD) || defined(HOST_DRAGONFLY) || \
    defined(HOST_SOLARIS) || \
    (defined(HOST_WIN32) && !defined(HOST_CYGWIN))
	/*
		Some stacks return an IPv6-only passive AF_UNSPEC listener first.
		Old TCP programs commonly start a wildcard server and then connect
		to 127.0.0.1 or the host's IPv4 LAN address, so keep wildcard
		servers on IPv4 unless the program explicitly names a host.
	*/
	hints.ai_family = (*tcp_proto->host == '\0') ? AF_INET : AF_UNSPEC;
#else
	hints.ai_family = AF_UNSPEC;
#endif

	fb_hDevTcpFormatService( service, sizeof( service ), tcp_proto->port );
	res = getaddrinfo( (*tcp_proto->host != '\0') ? tcp_proto->host : NULL, service, &hints, &result );
	if( res != 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	for( it = result; it != NULL; it = it->ai_next ) {
		int yes = 1;

		hSocket = (FB_TCP_SOCKET)socket( it->ai_family, it->ai_socktype, it->ai_protocol );
		if( FB_TCP_SOCKET_ERROR( hSocket ) )
			continue;

		setsockopt( hSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof( yes ) );
		fb_hDevTcpApplySocketOptions( hSocket, tcp_proto->timeout, FB_TCP_SOCKET_LISTENER );

		if( bind( hSocket, it->ai_addr, (int)it->ai_addrlen ) != 0 ) {
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
#endif
}

static int fb_hDevTcpPeekState( DEV_TCP_INFO *info )
{
#if defined(HOST_DOS) || defined(HOST_JS)
	(void)info;
	return -1;
#elif defined(HOST_WII)
	int state;

	if( info == NULL || FB_TCP_SOCKET_ERROR( info->hSocket ) )
		return -1;

	if( info->is_closed )
		return -1;

	/*
		libogc does not provide the exact readiness contract that FreeBASIC's
		TCP device needs here.  A Wii receive pump owns blocking recv() and
		keeps bytes in a handle-local buffer, so EOF can answer only the
		FreeBASIC question: is there data available to GET right now?
	*/
	state = fb_WiiTcpPumpPeekState( info->wii_pump );
	if( state < 0 )
		info->is_closed = TRUE;

	return state;
#else
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
	FD_SET( info->hSocket, &set );
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
#endif
}

static size_t fb_hDevTcpReadPeekByte( DEV_TCP_INFO *info, char *buffer, size_t length )
{
#if defined(HOST_WII)
	if( info == NULL || buffer == NULL || length == 0 )
		return 0;

	return fb_WiiTcpPumpRead( info->wii_pump, buffer, length, FALSE );
#else
	(void)info;
	(void)buffer;
	(void)length;
	return 0;
#endif
}

static int fb_hDevTcpEocState( DEV_TCP_INFO *info )
{
#if defined(HOST_WII)
	int state;

	if( info == NULL || FB_TCP_SOCKET_ERROR( info->hSocket ) )
		return -1;

	if( info->is_closed )
		return -1;

	/*
		EOC is a connection-lost test, not a readiness probe.  The receive pump
		reports a closed connection only after buffered bytes have been drained,
		so old code can still read the final packet before EOC becomes true.
	*/
	state = fb_WiiTcpPumpPeekState( info->wii_pump );
	if( state < 0 )
		info->is_closed = TRUE;

	return state;
#else
	return fb_hDevTcpPeekState( info );
#endif
}

static int fb_hDevTcpSendAll( DEV_TCP_INFO *info, const char *buffer, size_t length )
{
#if defined(HOST_DOS) || defined(HOST_JS)
	(void)info;
	(void)buffer;
	(void)length;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#else
	size_t total = 0;

	while( total < length ) {
		int chunk = (int)MIN( length - total, (size_t)INT_MAX );
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

		total += sent;
	}

	return FB_RTERROR_OK;
#endif
}

#if !defined(HOST_WII)
static void fb_hDevTcpShutdownConnectedSocket( DEV_TCP_INFO *info )
{
#if defined(HOST_DOS) || defined(HOST_JS)
	(void)info;
#else
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
		FD_SET( info->hSocket, &set );
		tv.tv_sec = slice / 1000;
		tv.tv_usec = (slice % 1000) * 1000;

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
#endif
}
#endif

static int fb_DevTcpClose( FB_FILE *handle )
{
	int res = FB_RTERROR_OK;
	DEV_TCP_INFO *info;

	info = (DEV_TCP_INFO*)handle->opaque;
	if( info != NULL ) {
		if( FB_TCP_SOCKET_ERROR( info->hSocket ) == FALSE ) {
			int close_res;
			int socket_closed = FALSE;

#if defined(HOST_WII)
			if( info->wii_pump != NULL ) {
				FB_UNLOCK();
				fb_WiiTcpPumpDestroy( info->wii_pump );
				info->wii_pump = NULL;
				FB_LOCK();
				socket_closed = TRUE;
			}
#else
			fb_hDevTcpShutdownConnectedSocket( info );
#endif
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

#if defined(HOST_DOS) || defined(HOST_JS)
	*pValuelen = 0;
	res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#elif defined(HOST_WII)
	{
		size_t bytes;

		FB_UNLOCK();
		bytes = fb_WiiTcpPumpRead( info->wii_pump,
		                           ((char *)value) + buffered,
		                           length - buffered,
		                           TRUE );
		FB_LOCK();

		*pValuelen = buffered + bytes;
		if( bytes == 0 )
			info->is_closed = TRUE;
	}
#else
	{
		int err = 0;
		int bytes = fb_hDevTcpRecvUnlocked( info,
		                                    ((char *)value) + buffered,
		                                    length - buffered,
		                                    0,
		                                    &err );
		if( bytes > 0 ) {
			*pValuelen = buffered + bytes;
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
#endif

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
#if defined(HOST_DOS) || defined(HOST_JS)
		*pOffset = 0;
		res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#elif FB_TCP_CAN_QUERY_BYTES == FALSE
		/*
			libogc exposes net_ioctl(), but currently only FIONBIO is wired
			through to IOS.  LOC() on a Wii TCP handle therefore cannot report
			buffered bytes without consuming data.  Returning zero is safer
			than pretending FIONREAD worked.
		*/
		*pOffset = 0;
#elif defined(HOST_WIN32) && !defined(HOST_CYGWIN)
		u_long bytes = 0;
		if( FB_TCP_IOCTL( info->hSocket, FIONREAD, &bytes ) != 0 ) {
			res = fb_ErrorSetNum( FB_RTERROR_FILEIO );
		}
		*pOffset = bytes;
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

#if defined(HOST_DOS) || defined(HOST_JS)
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#elif defined(HOST_WII)
	{
		size_t bytes;

		FB_UNLOCK();
		bytes = fb_WiiTcpPumpRead( info->wii_pump, ch, 1, TRUE );
		FB_LOCK();

		if( bytes > 0 )
			*read_len = 1;
		else
			info->is_closed = TRUE;
	}
#else
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
#endif

	return FB_RTERROR_OK;
}

static void fb_hDevTcpAppendChunk( FBSTRING *dst, const char *buffer, ssize_t len )
{
	FBSTRING *src;

	if( len <= 0 )
		return;

	src = fb_StrAllocTempDescF( (void *)buffer, len + 1 );
	if( dst->data == NULL ) {
		fb_StrAssign( dst, -1, src, -1, FALSE );
	} else {
		fb_StrConcatAssign( dst, -1, src, -1, FALSE );
	}
}

static int fb_DevTcpReadLine( FB_FILE *handle, FBSTRING *dst )
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
#if defined(HOST_DOS) || defined(HOST_JS)
	(void)handle;
	(void)filename;
	(void)filename_len;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#else
	return fb_hDevTcpOpenCommon( handle, filename, filename_len, FALSE );
#endif
}

int fb_DevTcpOpenServer( FB_FILE *handle, const char *filename, size_t filename_len )
{
#if defined(HOST_DOS) || defined(HOST_JS)
	(void)handle;
	(void)filename;
	(void)filename_len;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#else
	return fb_hDevTcpOpenCommon( handle, filename, filename_len, TRUE );
#endif
}

int fb_DevTcpAcceptHandle( FB_FILE *server_handle, FB_FILE *client_handle )
{
#if defined(HOST_DOS) || defined(HOST_JS)
	(void)server_handle;
	(void)client_handle;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#else
	DEV_TCP_INFO *server_info;
	FB_TCP_SOCKET hSocket;
	int res;

	if( server_handle == NULL || client_handle == NULL )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	server_info = (DEV_TCP_INFO*)server_handle->opaque;
	if( server_info == NULL || server_handle->type != FB_FILE_TYPE_TCPSERVER )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	/*
		The listener timeout is expressed in milliseconds by the OPEN TCP
		SERVER protocol.  SO_RCVTIMEO does not make accept() interruptible on
		Windows, so use the same readiness wait explicitly before accepting.
		Without this step a threaded server cannot poll for shutdown and the
		timeout option behaves differently on Windows and POSIX hosts.
	*/
	if( server_info->timeout != 0 ) {
		fd_set set;
		struct timeval tv;
		int ready;

		FD_ZERO( &set );
		FD_SET( server_info->hSocket, &set );
		tv.tv_sec = server_info->timeout / 1000;
		tv.tv_usec = (server_info->timeout % 1000) * 1000;

		ready = FB_TCP_SELECT( server_info->hSocket + 1, &set, NULL, NULL, &tv );
		if( ready <= 0 )
			return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	#if defined(HOST_WII)
	{
		struct sockaddr_in addr;
		socklen_t addrlen = sizeof( addr );
		memset( &addr, 0, sizeof( addr ) );
		hSocket = (FB_TCP_SOCKET)accept( server_info->hSocket, (struct sockaddr *)&addr, &addrlen );
	}
	#else
	hSocket = (FB_TCP_SOCKET)accept( server_info->hSocket, NULL, NULL );
	#endif
	if( FB_TCP_SOCKET_ERROR( hSocket ) )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	res = fb_hDevTcpApplySocketOptions( hSocket, server_info->timeout, FB_TCP_SOCKET_ACCEPTED );
	if( res != FB_RTERROR_OK ) {
		FB_TCP_CLOSESOCKET( hSocket );
		return res;
	}

	fb_hDevTcpSetNoDelay( hSocket );
	fb_hDevTcpSetConnectedNonBlocking( hSocket );

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
#endif
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
