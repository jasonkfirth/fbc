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

#define FB_TCP_WII_RECV_BUFFER_SIZE 16384

#if defined(HOST_WII)
typedef struct FB_WII_TCP_PUMP FB_WII_TCP_PUMP;
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
#if defined(HOST_WII)
	FB_WII_TCP_PUMP *wii_pump;
#endif
} DEV_TCP_INFO;

int fb_DevTcpParseProtocol( DEV_TCP_PROTOCOL **tcp_proto_out, const char *proto_raw, size_t proto_raw_len, int is_server );
int fb_DevTcpAcceptHandle( FB_FILE *server_handle, FB_FILE *client_handle );
int fb_DevTcpEocEx( FB_FILE *handle );

#if defined(HOST_WII)
FB_WII_TCP_PUMP *fb_WiiTcpPumpCreate( FB_TCP_SOCKET hSocket );
void fb_WiiTcpPumpDestroy( FB_WII_TCP_PUMP *pump );
int fb_WiiTcpPumpPeekState( FB_WII_TCP_PUMP *pump );
size_t fb_WiiTcpPumpRead( FB_WII_TCP_PUMP *pump, void *buffer, size_t length, int wait );
#endif

#endif
