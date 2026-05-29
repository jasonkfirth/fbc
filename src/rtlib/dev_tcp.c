/* TCP device */

#include "fb.h"
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
	#define FB_TCP_WOULDBLOCK(err) ((err) == WSAEWOULDBLOCK)
	#define FB_TCP_SHUT_WR SD_SEND
	#define FB_TCP_SHUT_RDWR SD_BOTH
	#define FB_TCP_SELECT(n, r, w, e, t) select( 0, r, w, e, t )
	#define FB_TCP_IOCTL(s, cmd, argp) ioctlsocket( s, cmd, argp )
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
	#define FB_TCP_CAN_QUERY_BYTES TRUE
#endif

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
static void fb_hDevTcpFormatService( char *service, size_t service_len, unsigned int port );

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

	return info;
}

static int fb_hDevTcpApplySocketOptions( FB_TCP_SOCKET hSocket, unsigned int timeout )
{
#if defined(HOST_DOS) || defined(HOST_JS)
	(void)hSocket;
	(void)timeout;
	return FB_RTERROR_ILLEGALFUNCTIONCALL;
#else
	#if defined(SO_NOSIGPIPE)
		{
			int value = 1;
			setsockopt( hSocket, SOL_SOCKET, SO_NOSIGPIPE, (const char *)&value, sizeof( value ) );
		}
	#endif

	if( timeout != 0 ) {
		#if defined(HOST_WIN32) && !defined(HOST_CYGWIN)
			DWORD value = (DWORD)timeout;
			setsockopt( hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&value, sizeof( value ) );
			setsockopt( hSocket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&value, sizeof( value ) );
		#else
			struct timeval value;

			value.tv_sec = timeout / 1000;
			value.tv_usec = (timeout % 1000) * 1000;

			setsockopt( hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&value, sizeof( value ) );
			setsockopt( hSocket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&value, sizeof( value ) );
		#endif
	}

	return FB_RTERROR_OK;
#endif
}

static void fb_hDevTcpFormatService( char *service, size_t service_len, unsigned int port )
{
	char tmp[16];
	size_t used = 0;

	if( service_len == 0 )
		return;

	do {
		tmp[used++] = (char)( '0' + ( port % 10 ) );
		port /= 10;
	} while( (port != 0) && (used < sizeof( tmp )) );

	if( used >= service_len )
		used = service_len - 1;

	for( size_t i = 0; i < used; i++ )
		service[i] = tmp[used - i - 1];

	service[used] = '\0';
}

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
static int fb_hDevTcpRecvUnlocked( DEV_TCP_INFO *info, void *buffer, size_t length, int flags, int *err )
{
	FB_TCP_SOCKET hSocket = info->hSocket;
	int bytes;

	FB_UNLOCK();
	bytes = recv( hSocket, buffer, (int)MIN( length, (size_t)INT_MAX ), flags );
	*err = FB_TCP_ERRNO();
	FB_LOCK();

	return bytes;
}

static int fb_hDevTcpSendUnlocked( DEV_TCP_INFO *info, const char *buffer, size_t length, int flags, int *err )
{
	FB_TCP_SOCKET hSocket = info->hSocket;
	int bytes;

	FB_UNLOCK();
	bytes = send( hSocket, buffer, (int)MIN( length, (size_t)INT_MAX ), flags );
	*err = FB_TCP_ERRNO();
	FB_LOCK();

	return bytes;
}
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

	fb_hDevTcpApplySocketOptions( hSocket, tcp_proto->timeout );

	if( connect( hSocket, (struct sockaddr *)&addr, sizeof( addr ) ) == 0 ) {
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

		fb_hDevTcpApplySocketOptions( hSocket, tcp_proto->timeout );

		if( connect( hSocket, it->ai_addr, (int)it->ai_addrlen ) == 0 ) {
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

	setsockopt( hSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof( yes ) );
	fb_hDevTcpApplySocketOptions( hSocket, tcp_proto->timeout );

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
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;

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
		fb_hDevTcpApplySocketOptions( hSocket, tcp_proto->timeout );

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
	err = FB_TCP_ERRNO();
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

static int fb_DevTcpClose( FB_FILE *handle )
{
	int res = FB_RTERROR_OK;
	DEV_TCP_INFO *info;

	info = (DEV_TCP_INFO*)handle->opaque;
	if( info != NULL ) {
		if( FB_TCP_SOCKET_ERROR( info->hSocket ) == FALSE ) {
			int close_res;

			fb_hDevTcpShutdownConnectedSocket( info );
			FB_UNLOCK();
			close_res = FB_TCP_CLOSESOCKET( info->hSocket );
			FB_LOCK();
			if( close_res != 0 )
				res = fb_ErrorSetNum( FB_RTERROR_FILEIO );
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

	info = (DEV_TCP_INFO*)handle->opaque;
	if( info == NULL || pValuelen == NULL ) {
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	if( info->is_closed ) {
		*pValuelen = 0;
		return FB_RTERROR_OK;
	}

#if defined(HOST_DOS) || defined(HOST_JS)
	*pValuelen = 0;
	res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#else
	{
		int err = 0;
		int bytes = fb_hDevTcpRecvUnlocked( info, value, *pValuelen, 0, &err );
		if( bytes > 0 ) {
			*pValuelen = bytes;
		} else if( bytes == 0 ) {
			info->is_closed = TRUE;
			*pValuelen = 0;
		} else if( FB_TCP_WOULDBLOCK( err ) ) {
			*pValuelen = 0;
		} else {
			info->is_closed = TRUE;
			*pValuelen = 0;
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

	if( info->is_closed )
		return FB_RTERROR_OK;

#if defined(HOST_DOS) || defined(HOST_JS)
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
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

	res = fb_hDevTcpApplySocketOptions( hSocket, server_info->timeout );
	if( res != FB_RTERROR_OK ) {
		FB_TCP_CLOSESOCKET( hSocket );
		return res;
	}

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
		return fb_hDevTcpFinishOpen( client_handle, &dummy, hSocket, &hooks_dev_tcp, FB_FILE_TYPE_TCP, FALSE, "TCP" );
	}
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
	res = (fb_hDevTcpPeekState( info ) < 0);
	FB_UNLOCK();

	return res ? FB_TRUE : FB_FALSE;
}

#endif /* DISABLE_TCP */
