''
''
'' sys\socket -- DragonFly CRT socket declarations
''
''
#ifndef __crt_sys_dragonfly_socket_bi__
#define __crt_sys_dragonfly_socket_bi__

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

#define PF_UNSPEC 0
#define PF_LOCAL 1
#define PF_UNIX PF_LOCAL
#define PF_INET 2
#define PF_ROUTE 17
#define PF_INET6 28
#define PF_KEY 27
#define PF_MAX 37

#define AF_UNSPEC PF_UNSPEC
#define AF_LOCAL PF_LOCAL
#define AF_UNIX PF_UNIX
#define AF_INET PF_INET
#define AF_ROUTE PF_ROUTE
#define AF_INET6 PF_INET6
#define AF_KEY PF_KEY
#define AF_MAX PF_MAX

#ifndef sockaddr
	type sockaddr
		sa_len as ubyte
		sa_family as sa_family_t
		sa_data(0 to 14-1) as byte
	end type
#endif

#define _SS_MAXSIZE 128

type sockaddr_storage
	ss_len as ubyte
	ss_family as sa_family_t
	__ss_pad1(0 to 6-1) as ubyte
	__ss_align as longint
	__ss_pad2(0 to 112-1) as ubyte
end type

enum
	MSG_OOB = &h0001
	MSG_PEEK = &h0002
	MSG_DONTROUTE = &h0004
	MSG_EOR = &h0008
	MSG_TRUNC = &h0010
	MSG_CTRUNC = &h0020
	MSG_WAITALL = &h0040
	MSG_DONTWAIT = &h0080
	MSG_EOF = &h0100
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

enum
	SCM_RIGHTS = &h01
end enum

#define SOL_SOCKET &hffff
#define SO_DEBUG &h0001
#define SO_REUSEADDR &h0004
#define SO_KEEPALIVE &h0008
#define SO_DONTROUTE &h0010
#define SO_BROADCAST &h0020
#define SO_LINGER &h0080
#define SO_OOBINLINE &h0100
#define SO_SNDBUF &h1001
#define SO_RCVBUF &h1002
#define SO_SNDLOWAT &h1003
#define SO_RCVLOWAT &h1004
#define SO_ERROR &h1007
#define SO_TYPE &h1008
#define SOMAXCONN 128

type linger
	l_onoff as long
	l_linger as long
end type

#endif
