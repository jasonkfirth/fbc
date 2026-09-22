/* FreeBASIC DOS runtime: dev_tcp_watt.h
 * Private Watt-32 call boundaries. The caller supplies connected nonblocking
 * sockets. This interface does not implement BASIC file handles or protocols.
 */

#ifndef FB_DOS_DEV_TCP_WATT_H
#define FB_DOS_DEV_TCP_WATT_H

#if defined(FB_DOS_WATT32) && !defined(DISABLE_TCP)
#include <sys/time.h>

void fb_DosTcpInitClock( void );
int fb_DosTcpSend( int socket, const void *buffer, int length, int flags );
int fb_DosTcpRecv( int socket, void *buffer, int length, int flags );
int fb_DosTcpSelect( int count, fd_set *readers, fd_set *writers,
                    fd_set *errors, struct timeval *timeout );
int fb_DosTcpClose( int socket );
int fb_DosTcpShutdown( int socket, int how );
#endif

#endif

/* end of dev_tcp_watt.h */
