''
'' FreeBASIC CRT socket declarations for GCCSDK UnixLib
'' ----------------------------------------------------
''
'' File: crt/sys/socket.bi
''
'' Purpose:
''
''     Define the complete UnixLib socket ABI.
''
'' Responsibilities:
''
''     - define UnixLib protocol families, options, and shutdown modes
''     - reproduce BSD-style socket and message layouts
''     - declare UnixLib socket functions and compatibility aliases
''
'' This file intentionally does NOT contain:
''
''     - Internet address structures owned by crt/netinet/in.bi
''     - FreeBASIC OPEN TCP runtime behavior
''     - declarations for other operating systems
''
#ifndef __crt_sys_socket_bi__
#define __crt_sys_socket_bi__

#include once "crt/stdint.bi"
#include once "crt/stddef.bi"
#include once "crt/sys/types.bi"
#include once "crt/sys/uio.bi"

#ifndef socklen_t
type socklen_t as __socklen_t
#endif

type sa_family_t as ubyte

'' ---------------------------------------------------------------------------
'' Socket types and protocol families
'' ---------------------------------------------------------------------------

enum __socket_type
	SOCK_STREAM = 1
	SOCK_DGRAM = 2
	SOCK_RAW = 3
	SOCK_RDM = 4
	SOCK_SEQPACKET = 5
end enum

#define PF_UNSPEC 0
#define PF_LOCAL 1
#define PF_UNIX PF_LOCAL
#define PF_FILE PF_LOCAL
#define PF_INET 2
#define PF_IMPLINK 3
#define PF_PUP 4
#define PF_CHAOS 5
#define PF_NS 6
#define PF_ISO 7
#define PF_OSI PF_ISO
#define PF_ECMA 8
#define PF_DATAKIT 9
#define PF_CCITT 10
#define PF_SNA 11
#define PF_DECnet 12
#define PF_DLI 13
#define PF_LAT 14
#define PF_HYLINK 15
#define PF_APPLETALK 16
#define PF_ROUTE 17
#define PF_LINK 18
#define PF_XTP 19
#define PF_COIP 20
#define PF_CNT 21
#define PF_RTIP 22
#define PF_IPX 23
#define PF_SIP 24
#define PF_PIP 25
#define PF_INET6 26
#define PF_MAX 27

#define AF_UNSPEC PF_UNSPEC
#define AF_LOCAL PF_LOCAL
#define AF_UNIX PF_UNIX
#define AF_FILE PF_FILE
#define AF_INET PF_INET
#define AF_IMPLINK PF_IMPLINK
#define AF_PUP PF_PUP
#define AF_CHAOS PF_CHAOS
#define AF_NS PF_NS
#define AF_ISO PF_ISO
#define AF_OSI PF_OSI
#define AF_ECMA PF_ECMA
#define AF_DATAKIT PF_DATAKIT
#define AF_CCITT PF_CCITT
#define AF_SNA PF_SNA
#define AF_DECnet PF_DECnet
#define AF_DLI PF_DLI
#define AF_LAT PF_LAT
#define AF_HYLINK PF_HYLINK
#define AF_APPLETALK PF_APPLETALK
#define AF_ROUTE PF_ROUTE
#define AF_LINK PF_LINK
#define pseudo_AF_XTP PF_XTP
#define AF_COIP PF_COIP
#define AF_CNT PF_CNT
#define pseudo_AF_RTIP PF_RTIP
#define AF_IPX PF_IPX
#define AF_SIP PF_SIP
#define pseudo_AF_PIP PF_PIP
#define AF_INET6 PF_INET6
#define AF_MAX PF_MAX

'' ---------------------------------------------------------------------------
'' Address and message layouts
'' ---------------------------------------------------------------------------

#ifndef sockaddr
	type sockaddr
		sa_len as ubyte
		sa_family as sa_family_t
		sa_data(0 to 14-1) as byte
	end type
#endif

#define _SS_SIZE 128
type sockaddr_storage
	ss_len as ubyte
	ss_family as sa_family_t
	__ss_align as __uint32_t
	__ss_padding(0 to _SS_SIZE-(2*len(__uint32_t))-1) as byte
end type

enum
	MSG_OOB = &h01
	MSG_PEEK = &h02
	MSG_DONTROUTE = &h04
	MSG_EOR = &h08
	MSG_TRUNC = &h10
	MSG_CTRUNC = &h20
	MSG_WAITALL = &h40
	MSG_DONTWAIT = &h80
	MSG_NOSIGNAL = &h0400
end enum

type msghdr
	msg_name as any ptr
	msg_namelen as socklen_t
	msg_iov as iovec ptr
	msg_iovlen as long
	msg_control as any ptr
	msg_controllen as socklen_t
	msg_flags as long
end type

type cmsghdr
	cmsg_len as socklen_t
	cmsg_level as long
	cmsg_type as long
end type

#define SCM_RIGHTS &h01
#define SCM_TIMESTAMP &h02
#define SCM_CREDS &h03

'' SCM_CREDS carries this fixed-size 4.4BSD credential record.  UnixLib caps
'' supplementary groups at 16 so the structure has a stable wire layout.
#define CMGROUP_MAX 16
type cmsgcred
	cmcred_pid as __pid_t
	cmcred_uid as __uid_t
	cmcred_euid as __uid_t
	cmcred_gid as __gid_t
	cmcred_ngroups as long
	cmcred_groups(0 to CMGROUP_MAX-1) as __gid_t
end type

'' ---------------------------------------------------------------------------
'' Socket-level options
'' ---------------------------------------------------------------------------

#define SOL_SOCKET &hffff
#define SO_DEBUG &h0001
#define SO_ACCEPTCONN &h0002
#define SO_REUSEADDR &h0004
#define SO_KEEPALIVE &h0008
#define SO_DONTROUTE &h0010
#define SO_BROADCAST &h0020
#define SO_USELOOPBACK &h0040
#define SO_LINGER &h0080
#define SO_OOBINLINE &h0100
#define SO_REUSEPORT &h0200
#define SO_SNDBUF &h1001
#define SO_RCVBUF &h1002
#define SO_SNDLOWAT &h1003
#define SO_RCVLOWAT &h1004
#define SO_SNDTIMEO &h1005
#define SO_RCVTIMEO &h1006
#define SO_ERROR &h1007
#define SO_STYLE &h1008
#define SO_TYPE SO_STYLE
#define SOMAXCONN 128

type linger
	l_onoff as long
	l_linger as long
end type


type osockaddr
	sa_family as ushort
	sa_data(0 to 14-1) as ubyte
end type

enum
	SHUT_RD = 0
	SHUT_WR
	SHUT_RDWR
end enum

extern "c"
declare function socket_ alias "socket" (byval __domain as long, byval __type as long, byval __protocol as long) as long
#define opensocket socket_
declare function socketpair (byval __domain as long, byval __type as long, byval __protocol as long, byval __fds as long ptr) as long
declare function bind (byval __fd as long, byval __addr as const sockaddr ptr, byval __len as socklen_t) as long
declare function getsockname (byval __fd as long, byval __addr as sockaddr ptr, byval __len as socklen_t ptr) as long
declare function connect (byval __fd as long, byval __addr as const sockaddr ptr, byval __len as socklen_t) as long
declare function getpeername (byval __fd as long, byval __addr as sockaddr ptr, byval __len as socklen_t ptr) as long
declare function send (byval __fd as long, byval __buf as const zstring ptr, byval __n as size_t, byval __flags as long) as ssize_t
declare function recv (byval __fd as long, byval __buf as zstring ptr, byval __n as size_t, byval __flags as long) as ssize_t
declare function sendto (byval __fd as long, byval __buf as const zstring ptr, byval __n as size_t, byval __flags as long, byval __addr as const sockaddr ptr, byval __addr_len as socklen_t) as ssize_t
declare function recvfrom (byval __fd as long, byval __buf as zstring ptr, byval __n as size_t, byval __flags as long, byval __addr as sockaddr ptr, byval __addr_len as socklen_t ptr) as ssize_t
declare function sendmsg (byval __fd as long, byval __message as const msghdr ptr, byval __flags as long) as ssize_t
declare function recvmsg (byval __fd as long, byval __message as msghdr ptr, byval __flags as long) as ssize_t
declare function getsockopt (byval __fd as long, byval __level as long, byval __optname as long, byval __optval as any ptr, byval __optlen as socklen_t ptr) as long
declare function setsockopt (byval __fd as long, byval __level as long, byval __optname as long, byval __optval as const any ptr, byval __optlen as socklen_t) as long
declare function listen (byval __fd as long, byval __n as long) as long
declare function accept (byval __fd as long, byval __addr as sockaddr ptr, byval __addr_len as socklen_t ptr) as long
declare function shutdown (byval __fd as long, byval __how as long) as long
#ifndef __crt_close_declared__
#define __crt_close_declared__
'' Platform unistd headers may expose close() before this shared socket
'' header is included.  Keep the declaration independent of include order.
declare function close_ alias "close" (byval __fd as long) as long
#endif
declare function isfdtype (byval __fd as long, byval __fdtype as long) as long

#define closesocket close_
end extern

'' winsock-ish typedefs
'' Unix socket descriptors use C int even when FreeBASIC Integer follows the
'' native pointer width on a 64-bit target.
type socket as long
type PSOCKADDR as sockaddr ptr
type LPSOCKADDR as sockaddr ptr
type PSOCKADDR_IN as sockaddr_in ptr
type LPSOCKADDR_IN as sockaddr_in ptr
type PLINGER as linger ptr
type LPLINGER as linger ptr
type PIN_ADDR as in_addr ptr
type LPIN_ADDR as in_addr ptr
type PFD_SET as fd_set ptr
type LPFD_SET as fd_set ptr
type PHOSTENT as hostent ptr
type LPHOSTENT as hostent ptr
type PSERVENT as servent ptr
type LPSERVENT as servent ptr
type PPROTOENT as protoent ptr
type LPPROTOENT as protoent ptr
type PTIMEVAL as timeval ptr
type LPTIMEVAL as timeval ptr

#define SOCKET_ERROR -1

#endif

'' end of crt/sys/socket.bi
