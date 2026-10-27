/*
 * Project: FreeBASIC classic AmigaOS runtime
 * File: drivers/amiga/dev_tcp_socket.h
 * Purpose: Adapt the shared TCP device to task-owned bsdsocket libraries.
 * Responsibilities: Declare private socket operations and IPv4 resolution.
 * This file intentionally does NOT contain: BASIC file hooks or native ownership.
 */

#ifndef FB_AMIGA_DEV_TCP_SOCKET_H
#define FB_AMIGA_DEV_TCP_SOCKET_H

#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

int fb_AmigaTcpAddress( const char *host, unsigned int port, int passive, struct sockaddr_in *address );
int fb_AmigaTcpSocket( int domain, int type, int protocol );
int fb_AmigaTcpClose( int socket );
int fb_AmigaTcpConnect( int socket, const struct sockaddr *address, int length );
int fb_AmigaTcpBind( int socket, const struct sockaddr *address, int length );
int fb_AmigaTcpListen( int socket, int backlog );
int fb_AmigaTcpAccept( int socket, struct sockaddr *address, socklen_t *length );
int fb_AmigaTcpShutdown( int socket, int how );
int fb_AmigaTcpRecv( int socket, void *buffer, int length, int flags );
int fb_AmigaTcpSend( int socket, const void *buffer, int length, int flags );
int fb_AmigaTcpIoctl( int socket, unsigned long command, void *argument );
int fb_AmigaTcpSetSockOpt( int socket, int level, int option, const void *value, int length );
int fb_AmigaTcpSelect( int count, fd_set *readers, fd_set *writers, fd_set *errors, struct timeval *timeout );

#endif

/* end of drivers/amiga/dev_tcp_socket.h */
