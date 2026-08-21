/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/fb_tcp_wince.h

    Purpose:

        Define the private Windows CE TCP transport contract.

    Responsibilities:

        - select the original WinSock ABI available on classic CE images
        - describe parsed OPEN TCP options and live socket state
        - expose parser, accept, and end-of-connection helpers

    This file intentionally does NOT contain:

        - protocol-string parsing implementation
        - socket I/O operations
        - desktop WinSock 2 address-resolution types
*/

#ifndef FB_RT_WINCE_TCP_H
#define FB_RT_WINCE_TCP_H

#include <winsock.h>

typedef SOCKET FB_TCP_SOCKET;

#define FB_TCP_INVALID_SOCKET INVALID_SOCKET

typedef struct DEV_TCP_PROTOCOL {
	char *host;
	unsigned int port;
	unsigned int timeout;
	unsigned int backlog;
	int is_server;
	char raw[];
} DEV_TCP_PROTOCOL;

typedef struct DEV_TCP_INFO {
	FB_TCP_SOCKET hSocket;
	char *pszDevice;
	unsigned int timeout;
	int is_server;
	int is_closed;
} DEV_TCP_INFO;

int fb_DevTcpParseProtocol( DEV_TCP_PROTOCOL **tcp_proto_out,
	                        const char *proto_raw, size_t proto_raw_len,
	                        int is_server );
int fb_DevTcpAcceptHandle( FB_FILE *server_handle, FB_FILE *client_handle );
int fb_DevTcpEocEx( FB_FILE *handle );

#endif

/* end of wince/fb_tcp_wince.h */
