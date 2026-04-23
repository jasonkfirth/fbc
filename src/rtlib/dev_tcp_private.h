#ifndef __FB_DEV_TCP_PRIVATE_H__
#define __FB_DEV_TCP_PRIVATE_H__

#if defined(HOST_WIN32) && !defined(HOST_CYGWIN)
	#include <winsock2.h>
	typedef SOCKET FB_TCP_SOCKET;
	#define FB_TCP_INVALID_SOCKET INVALID_SOCKET
#else
	typedef int FB_TCP_SOCKET;
	#define FB_TCP_INVALID_SOCKET (-1)
#endif

typedef struct {
	char *host;
	unsigned int port;
	unsigned int timeout;
	unsigned int backlog;
	int is_server;
	char raw[];
} DEV_TCP_PROTOCOL;

typedef struct {
	FB_TCP_SOCKET hSocket;
	char *pszDevice;
	unsigned int timeout;
	int is_server;
	int is_closed;
} DEV_TCP_INFO;

int fb_DevTcpParseProtocol( DEV_TCP_PROTOCOL **tcp_proto_out, const char *proto_raw, size_t proto_raw_len, int is_server );
int fb_DevTcpOpen( FB_FILE *handle, const char *filename, size_t filename_len );
int fb_DevTcpOpenServer( FB_FILE *handle, const char *filename, size_t filename_len );
int fb_DevTcpAcceptHandle( FB_FILE *server_handle, FB_FILE *client_handle );
int fb_DevTcpEocEx( FB_FILE *handle );

#endif
