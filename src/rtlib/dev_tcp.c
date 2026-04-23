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

#if !defined(HOST_DOS) && !defined(HOST_JS) && !defined(HOST_XBOX)
	#if defined(HOST_WIN32) && !defined(HOST_CYGWIN)
		#include <ws2tcpip.h>
	#else
		#include <errno.h>
		#include <netdb.h>
		#include <sys/ioctl.h>
		#include <sys/select.h>
		#include <sys/socket.h>
		#include <sys/types.h>
		#include <netinet/in.h>
		#include <arpa/inet.h>
		#include <unistd.h>
	#endif
#endif

#if defined(HOST_DOS) || defined(HOST_JS) || defined(HOST_XBOX)
	#define FB_TCP_CLOSESOCKET(s) (0)
	#define FB_TCP_ERRNO() 0
	#define FB_TCP_WOULDBLOCK(err) FALSE
	#define FB_TCP_SHUT_WR 1
	#define FB_TCP_SHUT_RDWR 2
#elif defined(HOST_WIN32) && !defined(HOST_CYGWIN)
	#define FB_TCP_CLOSESOCKET(s) closesocket( s )
	#define FB_TCP_ERRNO() WSAGetLastError( )
	#define FB_TCP_WOULDBLOCK(err) ((err) == WSAEWOULDBLOCK)
	#define FB_TCP_SHUT_WR SD_SEND
	#define FB_TCP_SHUT_RDWR SD_BOTH
#else
	#define FB_TCP_CLOSESOCKET(s) close( s )
	#define FB_TCP_ERRNO() errno
	#define FB_TCP_WOULDBLOCK(err) ((err) == EAGAIN || (err) == EWOULDBLOCK)
	#define FB_TCP_SHUT_WR SHUT_WR
	#define FB_TCP_SHUT_RDWR SHUT_RDWR
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
#if defined(HOST_DOS) || defined(HOST_JS) || defined(HOST_XBOX)
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

static int fb_hDevTcpCreateConnectedSocket( DEV_TCP_PROTOCOL *tcp_proto, FB_TCP_SOCKET *hSocketOut )
{
#if defined(HOST_DOS) || defined(HOST_JS) || defined(HOST_XBOX)
	(void)tcp_proto;
	(void)hSocketOut;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
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

	snprintf( service, sizeof( service ), "%u", tcp_proto->port );
	res = getaddrinfo( tcp_proto->host, service, &hints, &result );
	if( res != 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	for( it = result; it != NULL; it = it->ai_next ) {
		hSocket = (FB_TCP_SOCKET)socket( it->ai_family, it->ai_socktype, it->ai_protocol );
		if( hSocket == FB_TCP_INVALID_SOCKET )
			continue;

		fb_hDevTcpApplySocketOptions( hSocket, tcp_proto->timeout );

		if( connect( hSocket, it->ai_addr, (int)it->ai_addrlen ) == 0 ) {
			*hSocketOut = hSocket;
			freeaddrinfo( result );
			return FB_RTERROR_OK;
		}

		FB_TCP_CLOSESOCKET( hSocket );
		hSocket = FB_TCP_INVALID_SOCKET;
	}

	freeaddrinfo( result );
	return fb_ErrorSetNum( FB_RTERROR_FILEIO );
#endif
}

static int fb_hDevTcpCreateServerSocket( DEV_TCP_PROTOCOL *tcp_proto, FB_TCP_SOCKET *hSocketOut )
{
#if defined(HOST_DOS) || defined(HOST_JS) || defined(HOST_XBOX)
	(void)tcp_proto;
	(void)hSocketOut;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
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

	snprintf( service, sizeof( service ), "%u", tcp_proto->port );
	res = getaddrinfo( (*tcp_proto->host != '\0') ? tcp_proto->host : NULL, service, &hints, &result );
	if( res != 0 )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	for( it = result; it != NULL; it = it->ai_next ) {
		int yes = 1;

		hSocket = (FB_TCP_SOCKET)socket( it->ai_family, it->ai_socktype, it->ai_protocol );
		if( hSocket == FB_TCP_INVALID_SOCKET )
			continue;

		setsockopt( hSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof( yes ) );
		fb_hDevTcpApplySocketOptions( hSocket, tcp_proto->timeout );

		if( bind( hSocket, it->ai_addr, (int)it->ai_addrlen ) != 0 ) {
			FB_TCP_CLOSESOCKET( hSocket );
			hSocket = FB_TCP_INVALID_SOCKET;
			continue;
		}

		if( listen( hSocket, (int)tcp_proto->backlog ) != 0 ) {
			FB_TCP_CLOSESOCKET( hSocket );
			hSocket = FB_TCP_INVALID_SOCKET;
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
#if defined(HOST_DOS) || defined(HOST_JS) || defined(HOST_XBOX)
	(void)info;
	return -1;
#else
	fd_set set;
	struct timeval tv;
	int res;
	char ch;

	if( info == NULL || info->hSocket == FB_TCP_INVALID_SOCKET )
		return -1;

	if( info->is_closed )
		return -1;

	FD_ZERO( &set );
	FD_SET( info->hSocket, &set );
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	#if defined(HOST_WIN32) && !defined(HOST_CYGWIN)
		res = select( 0, &set, NULL, NULL, &tv );
	#else
		res = select( info->hSocket + 1, &set, NULL, NULL, &tv );
	#endif

	if( res < 0 ) {
		info->is_closed = TRUE;
		return -1;
	}

	if( res == 0 )
		return 0;

	res = recv( info->hSocket, &ch, 1, MSG_PEEK );
	if( res > 0 )
		return 1;

	if( res == 0 ) {
		info->is_closed = TRUE;
		return -1;
	}

	if( FB_TCP_WOULDBLOCK( FB_TCP_ERRNO() ) )
		return 0;

	info->is_closed = TRUE;
	return -1;
#endif
}

static int fb_hDevTcpSendAll( DEV_TCP_INFO *info, const char *buffer, size_t length )
{
#if defined(HOST_DOS) || defined(HOST_JS) || defined(HOST_XBOX)
	(void)info;
	(void)buffer;
	(void)length;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#else
	size_t total = 0;

	while( total < length ) {
		int chunk = (int)MIN( length - total, (size_t)INT_MAX );
		int sent;
		int flags = 0;

		#if defined(MSG_NOSIGNAL)
			flags = MSG_NOSIGNAL;
		#endif

		sent = send( info->hSocket, buffer + total, chunk, flags );
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
#if defined(HOST_DOS) || defined(HOST_JS) || defined(HOST_XBOX)
	(void)info;
#else
	unsigned int timeout = 1000;
	char buffer[256];

	if( info == NULL || info->is_server || info->hSocket == FB_TCP_INVALID_SOCKET )
		return;

	if( info->timeout != 0 )
		timeout = info->timeout;

	shutdown( info->hSocket, FB_TCP_SHUT_WR );

	while( timeout > 0 ) {
		fd_set set;
		struct timeval tv;
		unsigned int slice = MIN( timeout, 100u );
		int res;

		FD_ZERO( &set );
		FD_SET( info->hSocket, &set );
		tv.tv_sec = slice / 1000;
		tv.tv_usec = (slice % 1000) * 1000;

		#if defined(HOST_WIN32) && !defined(HOST_CYGWIN)
			res = select( 0, &set, NULL, NULL, &tv );
		#else
			res = select( info->hSocket + 1, &set, NULL, NULL, &tv );
		#endif

		if( res <= 0 )
			break;

		res = recv( info->hSocket, buffer, sizeof( buffer ), 0 );
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

	FB_LOCK();

	info = (DEV_TCP_INFO*)handle->opaque;
	if( info != NULL ) {
		if( info->hSocket != FB_TCP_INVALID_SOCKET ) {
			fb_hDevTcpShutdownConnectedSocket( info );
			if( FB_TCP_CLOSESOCKET( info->hSocket ) != 0 )
				res = fb_ErrorSetNum( FB_RTERROR_FILEIO );
		}

		if( res == FB_RTERROR_OK ) {
			free( info->pszDevice );
			free( info );
		}
	}

	FB_UNLOCK();

	return res;
}

static int fb_DevTcpWrite( FB_FILE *handle, const void *value, size_t valuelen )
{
	int res;
	DEV_TCP_INFO *info;

	FB_LOCK();
	info = (DEV_TCP_INFO*)handle->opaque;
	res = fb_hDevTcpSendAll( info, (const char *)value, valuelen );
	FB_UNLOCK();

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

	FB_LOCK();

	info = (DEV_TCP_INFO*)handle->opaque;
	if( info == NULL || pValuelen == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	if( info->is_closed ) {
		*pValuelen = 0;
		FB_UNLOCK();
		return FB_RTERROR_OK;
	}

#if defined(HOST_DOS) || defined(HOST_JS) || defined(HOST_XBOX)
	*pValuelen = 0;
	res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#else
	{
		int bytes = recv( info->hSocket, value, (int)MIN( *pValuelen, (size_t)INT_MAX ), 0 );
		if( bytes > 0 ) {
			*pValuelen = bytes;
		} else if( bytes == 0 ) {
			info->is_closed = TRUE;
			*pValuelen = 0;
		} else if( FB_TCP_WOULDBLOCK( FB_TCP_ERRNO() ) ) {
			*pValuelen = 0;
		} else {
			info->is_closed = TRUE;
			*pValuelen = 0;
			res = fb_ErrorSetNum( FB_RTERROR_FILEIO );
		}
	}
#endif

	FB_UNLOCK();

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

	FB_LOCK();

	info = (DEV_TCP_INFO*)handle->opaque;
	if( info == NULL ) {
		res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	} else {
#if defined(HOST_DOS) || defined(HOST_JS) || defined(HOST_XBOX)
		*pOffset = 0;
		res = fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
#elif defined(HOST_WIN32) && !defined(HOST_CYGWIN)
		u_long bytes = 0;
		if( ioctlsocket( info->hSocket, FIONREAD, &bytes ) != 0 ) {
			res = fb_ErrorSetNum( FB_RTERROR_FILEIO );
		}
		*pOffset = bytes;
#else
		int bytes = 0;
		if( ioctl( info->hSocket, FIONREAD, &bytes ) != 0 ) {
			res = fb_ErrorSetNum( FB_RTERROR_FILEIO );
		}
		*pOffset = bytes;
#endif
	}

	FB_UNLOCK();

	return res;
}

static int fb_DevTcpEof( FB_FILE *handle )
{
	int res;
	DEV_TCP_INFO *info;

	FB_LOCK();
	info = (DEV_TCP_INFO*)handle->opaque;
	res = (fb_hDevTcpPeekState( info ) == 1 ? FB_FALSE : FB_TRUE);
	FB_UNLOCK();

	return res;
}

static int fb_DevTcpServerEof( FB_FILE *handle )
{
	(void)handle;
	return FB_TRUE;
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
	char buffer[1024];
	ssize_t len = 0;
	int res = FB_RTERROR_OK;

	fb_StrDelete( dst );

	do {
		char ch;
		size_t read_len = 0;

		res = fb_FileGetDataEx( handle, 0, &ch, 1, &read_len, FALSE, FALSE );
		if( res != FB_RTERROR_OK || read_len == 0 )
			break;

		if( ch == '\r' ) {
			res = fb_FileGetDataEx( handle, 0, &ch, 1, &read_len, FALSE, FALSE );
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
#if defined(HOST_DOS) || defined(HOST_JS) || defined(HOST_XBOX)
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
#if defined(HOST_DOS) || defined(HOST_JS) || defined(HOST_XBOX)
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
#if defined(HOST_DOS) || defined(HOST_JS) || defined(HOST_XBOX)
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

	hSocket = (FB_TCP_SOCKET)accept( server_info->hSocket, NULL, NULL );
	if( hSocket == FB_TCP_INVALID_SOCKET )
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
