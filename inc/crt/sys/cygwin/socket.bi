''
''
'' sys\socket -- Cygwin CRT socket declarations
''
#ifndef __crt_sys_cygwin_socket_bi__
#define __crt_sys_cygwin_socket_bi__

#include once "crt/stdint.bi"
#include once "crt/sys/types.bi"
#include once "crt/sys/uio.bi"

#ifndef socklen_t
type socklen_t as __socklen_t
#endif

type sa_family_t as __sa_family_t

enum __socket_type
	SOCK_STREAM = 1
	SOCK_DGRAM = 2
	SOCK_RAW = 3
	SOCK_RDM = 4
	SOCK_SEQPACKET = 5
end enum

#define SOCK_NONBLOCK &h01000000
#define SOCK_CLOEXEC &h02000000

#define PF_UNSPEC 0
#define PF_LOCAL 1
#define PF_UNIX 1
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
#define PF_NETBIOS 17
#define PF_INET6 23
#define PF_MAX 32

#define AF_UNSPEC PF_UNSPEC
#define AF_LOCAL PF_LOCAL
#define AF_UNIX PF_UNIX
#define AF_INET PF_INET
#define AF_IMPLINK PF_IMPLINK
#define AF_PUP PF_PUP
#define AF_CHAOS PF_CHAOS
#define AF_NS PF_NS
#define AF_ISO PF_ISO
#define AF_OSI AF_ISO
#define AF_ECMA PF_ECMA
#define AF_DATAKIT PF_DATAKIT
#define AF_CCITT PF_CCITT
#define AF_SNA PF_SNA
#define AF_DECnet PF_DECnet
#define AF_DLI PF_DLI
#define AF_LAT PF_LAT
#define AF_HYLINK PF_HYLINK
#define AF_APPLETALK PF_APPLETALK
#define AF_NETBIOS PF_NETBIOS
#define AF_INET6 PF_INET6
#define AF_MAX PF_MAX

#ifndef sockaddr
	type sockaddr
		sa_family as sa_family_t
		sa_data(0 to 14-1) as byte
	end type
#endif

#define _SS_MAXSIZE 128
#define _SS_ALIGNSIZE len(longint)
#define _SS_PAD1SIZE (_SS_ALIGNSIZE - len(sa_family_t))
#define _SS_PAD2SIZE (_SS_MAXSIZE - (len(sa_family_t) + _SS_PAD1SIZE + _SS_ALIGNSIZE))

type sockaddr_storage
	ss_family as sa_family_t
	_ss_pad1(0 to _SS_PAD1SIZE-1) as byte
	__ss_align as longint
	_ss_pad2(0 to _SS_PAD2SIZE-1) as byte
end type

enum
	MSG_OOB = &h0001
	MSG_PEEK = &h0002
	MSG_DONTROUTE = &h0004
	MSG_WAITALL = &h0008
	MSG_DONTWAIT = &h0010
	MSG_NOSIGNAL = &h0020
	MSG_TRUNC = &h0100
	MSG_CTRUNC = &h0200
	MSG_BCAST = &h0400
	MSG_MCAST = &h0800
	MSG_CMSG_CLOEXEC = &h1000
	MSG_EOR = &h8000
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
	cmsg_len as size_t
	cmsg_level as long
	cmsg_type as long
end type

enum
	SCM_RIGHTS = &h01
	SCM_CREDENTIALS = &h02
end enum

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
#define SO_PEERCRED &h0200
#define SO_PASSCRED &h0400
#define SO_SNDBUF &h1001
#define SO_RCVBUF &h1002
#define SO_SNDLOWAT &h1003
#define SO_RCVLOWAT &h1004
#define SO_SNDTIMEO &h1005
#define SO_RCVTIMEO &h1006
#define SO_ERROR &h1007
#define SO_TYPE &h1008
#define SOMAXCONN &h7fffffff

type linger
	l_onoff as ushort
	l_linger as ushort
end type

#endif
