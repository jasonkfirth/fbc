/*
    Project: FreeBASIC Runtime Library
    -------------------------

    File: drivers/wince/dev_tcp.c

    Purpose:

        Implement FreeBASIC TCP file devices on Windows CE.

    Responsibilities:

        - initialize and shut down the classic Windows CE WinSock service
        - open IPv4 client and listening sockets without getaddrinfo
        - provide file-device reads, writes, EOF/EOC, LOC, and TCP ACCEPT

    This file intentionally does NOT contain:

        - TCP protocol-string parsing
        - desktop console or graphics behavior
        - IPv6, which is not uniformly available on classic CE devices

    Windows CE socket baseline:

        The maintained CeGCC headers expose WinSock 2 declarations, but many
        deployable CE 3 through CE 5 images provide only the original WinSock
        service.  The runtime therefore requests version 1.1 and resolves IPv4
        names through gethostbyname().  This avoids importing getaddrinfo from
        WS2.DLL and keeps target programs usable on the broadest CE baseline.
*/

#include "../../fb.h"
#include <strings.h>
#include "fb_tcp_wince.h"

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

/* fb_tcp_wince.h owns the classic WinSock declarations. */

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
static int fb_hDevTcpSocketError( int result )
{
	(void)result;
	return (int)FB_TCP_ERRNO();
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
		if( WSAStartup( MAKEWORD( 1, 1 ), &wsaData ) != 0 ) {
			init_result = fb_ErrorSetNum( FB_RTERROR_FILEIO );
		} else {
			atexit( fb_hDevTcpShutdownWinsock );
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
		DWORD value = (DWORD)timeout;
		DWORD send_value = (socket_role == FB_TCP_SOCKET_ACCEPTED) ? 0 : value;
		FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&value, sizeof( value ) );
		FB_TCP_SETSOCKOPT( hSocket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&send_value, sizeof( send_value ) );
	}

	return FB_RTERROR_OK;
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

static int fb_hDevTcpWaitWritable( DEV_TCP_INFO *info )
{
	fd_set error_set;
	fd_set write_set;
	int ready;

	if( info == NULL || FB_TCP_SOCKET_ERROR( info->hSocket ) )
		return FALSE;

	FD_ZERO( &error_set );
	FD_ZERO( &write_set );
	FD_SET( info->hSocket, &error_set );
	FD_SET( info->hSocket, &write_set );

	/*
		Connected sockets are nonblocking so EOF can act as a readiness
		probe. A complete BASIC PUT remains a blocking operation, however.
		Wait without holding the runtime lock so another FreeBASIC thread in
		the same process can drain the peer socket and relieve backpressure.
	*/
	FB_UNLOCK();
	ready = FB_TCP_SELECT( info->hSocket + 1, NULL, &write_set,
	                       &error_set, NULL );
	FB_LOCK();

	if( ready <= 0 || FD_ISSET( info->hSocket, &error_set ) )
		return FALSE;

	return FD_ISSET( info->hSocket, &write_set ) ? TRUE : FALSE;
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

static int fb_hDevTcpBuildWinCEAddress( const char *host, unsigned int port,
	                                    int passive,
	                                    struct sockaddr_in *address )
{
	struct hostent *host_entry;
	unsigned long raw_address;

	if( address == NULL )
		return FALSE;

	memset( address, 0, sizeof(*address) );
	address->sin_family = AF_INET;
	address->sin_port = htons( (unsigned short)port );

	if( (host == NULL) || (*host == '\0') ) {
		address->sin_addr.s_addr = htonl( passive ? INADDR_ANY
		                                         : INADDR_LOOPBACK );
		return TRUE;
	}

	if( strcasecmp( host, "localhost" ) == 0 ) {
		address->sin_addr.s_addr = htonl( INADDR_LOOPBACK );
		return TRUE;
	}

	raw_address = inet_addr( host );
	if( (raw_address != INADDR_NONE) ||
	    (strcmp( host, "255.255.255.255" ) == 0) ) {
		address->sin_addr.s_addr = raw_address;
		return TRUE;
	}

	host_entry = gethostbyname( host );
	if( (host_entry == NULL) ||
	    (host_entry->h_addrtype != AF_INET) ||
	    (host_entry->h_length != (int)sizeof(struct in_addr)) ||
	    (host_entry->h_addr_list == NULL) ||
	    (host_entry->h_addr_list[0] == NULL) )
		return FALSE;

	memcpy( &address->sin_addr, host_entry->h_addr_list[0],
	        sizeof(struct in_addr) );
	return TRUE;
}

static int fb_hDevTcpCreateConnectedSocket( DEV_TCP_PROTOCOL *tcp_proto, FB_TCP_SOCKET *hSocketOut )
{
	struct sockaddr_in address;
	FB_TCP_SOCKET socket_handle;

	if( !fb_hDevTcpBuildWinCEAddress( tcp_proto->host, tcp_proto->port,
	                                FALSE, &address ) )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	socket_handle = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( FB_TCP_SOCKET_ERROR( socket_handle ) )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	fb_hDevTcpApplySocketOptions( socket_handle, tcp_proto->timeout,
	                             FB_TCP_SOCKET_CLIENT );
	if( connect( socket_handle, (struct sockaddr *)&address,
	             sizeof(address) ) == 0 ) {
		fb_hDevTcpSetNoDelay( socket_handle );
		*hSocketOut = socket_handle;
		return FB_RTERROR_OK;
	}

	FB_TCP_CLOSESOCKET( socket_handle );
	return fb_ErrorSetNum( FB_RTERROR_FILEIO );
}

static int fb_hDevTcpCreateServerSocket( DEV_TCP_PROTOCOL *tcp_proto, FB_TCP_SOCKET *hSocketOut )
{
	struct sockaddr_in address;
	FB_TCP_SOCKET socket_handle;
	int reuse_address = 1;

	if( !fb_hDevTcpBuildWinCEAddress( tcp_proto->host, tcp_proto->port,
	                                TRUE, &address ) )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	socket_handle = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( FB_TCP_SOCKET_ERROR( socket_handle ) )
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );

	FB_TCP_SETSOCKOPT( socket_handle, SOL_SOCKET, SO_REUSEADDR,
	                   (const char *)&reuse_address,
	                   sizeof(reuse_address) );
	fb_hDevTcpApplySocketOptions( socket_handle, tcp_proto->timeout,
	                             FB_TCP_SOCKET_LISTENER );

	if( bind( socket_handle, (struct sockaddr *)&address,
	          sizeof(address) ) != 0 ) {
		FB_TCP_CLOSESOCKET( socket_handle );
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	if( listen( socket_handle, (int)tcp_proto->backlog ) != 0 ) {
		FB_TCP_CLOSESOCKET( socket_handle );
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	*hSocketOut = socket_handle;
	return FB_RTERROR_OK;
}

static int fb_hDevTcpPeekState( DEV_TCP_INFO *info )
{
	fd_set set;
	struct timeval tv;
	int res;
	int err = 0;

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

	{
		u_long available = 0;

		res = FB_TCP_IOCTL( info->hSocket, FIONREAD, &available );
		err = fb_hDevTcpSocketError( res );
		FB_LOCK();

		if( res == 0 ) {
			/*
				WinCE can report a newly connected socket as readable while
				FIONREAD is still zero. Treat that state as idle; actual read
				and write hooks record an eventual peer closure in is_closed.
			*/
			return available > 0 ? 1 : 0;
		}

		if( FB_TCP_WOULDBLOCK( err ) )
			return 0;
	}

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
		int chunk = (int)MIN( length - total, (size_t)INT_MAX );
		int sent;
		int err;
		int flags = 0;

		#if defined(MSG_NOSIGNAL)
			flags = MSG_NOSIGNAL;
		#endif

		sent = fb_hDevTcpSendUnlocked( info, buffer + total, (size_t)chunk,
		                               flags, &err );
		if( sent <= 0 ) {
			if( sent < 0 && FB_TCP_WOULDBLOCK( err ) &&
			    fb_hDevTcpWaitWritable( info ) ) {
				continue;
			}
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
		FD_SET( info->hSocket, &set );
		tv.tv_sec = (long)(slice / 1000u);
		tv.tv_usec = (long)((slice % 1000u) * 1000u);

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
		u_long bytes = 0;
		if( FB_TCP_IOCTL( info->hSocket, FIONREAD, &bytes ) != 0 ) {
			res = fb_ErrorSetNum( FB_RTERROR_FILEIO );
		}
		*pOffset = (fb_off_t)bytes;
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

	if( len <= 0 )
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

/* end of drivers/wince/dev_tcp.c */
