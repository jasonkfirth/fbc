''
'' FreeBASIC CRT socket declarations
'' ---------------------------------
''
'' File: crt/sys/solaris/socket.bi
''
'' Purpose:
''
''     Describe the Solaris and illumos socket ABI used by crt/sys/socket.bi.
''
'' Responsibilities:
''
''     - define Solaris socket types and address families
''     - define the public socket address and message structures
''     - define commonly used socket options and message flags
''
'' This file intentionally does NOT contain:
''
''     - socket function declarations shared by Unix targets
''     - FreeBASIC OPEN TCP runtime behavior
''     - resolver declarations from netdb.bi
''

#ifndef __crt_sys_solaris_socket_bi__
#define __crt_sys_solaris_socket_bi__

#include once "crt/stdint.bi"
#include once "crt/sys/types.bi"
#include once "crt/sys/uio.bi"

#ifndef socklen_t
type socklen_t as uint32_t
#endif

type sa_family_t as uint16_t

enum __socket_type
	SOCK_STREAM = 2
	SOCK_DGRAM = 1
	SOCK_RAW = 4
	SOCK_RDM = 5
	SOCK_SEQPACKET = 6
end enum

#define AF_UNSPEC 0
#define AF_UNIX 1
#define AF_LOCAL AF_UNIX
#define AF_FILE AF_UNIX
#define AF_INET 2
#define AF_ROUTE 24
#define AF_LINK 25
#define AF_INET6 26
#define AF_KEY 27
#define AF_PACKET 32
#define AF_MAX 32

#define PF_UNSPEC AF_UNSPEC
#define PF_UNIX AF_UNIX
#define PF_LOCAL AF_LOCAL
#define PF_FILE AF_FILE
#define PF_INET AF_INET
#define PF_ROUTE AF_ROUTE
#define PF_LINK AF_LINK
#define PF_INET6 AF_INET6
#define PF_KEY AF_KEY
#define PF_PACKET AF_PACKET
#define PF_MAX AF_MAX

#ifndef sockaddr
	type sockaddr
		sa_family as sa_family_t
		sa_data(0 to 14-1) as byte
	end type
#endif

#define _SS_MAXSIZE 256

type sockaddr_storage
	ss_family as sa_family_t
	__ss_pad1(0 to 6-1) as byte
	__ss_align as double
	__ss_pad2(0 to 240-1) as byte
end type

enum
	MSG_OOB = &h0001
	MSG_PEEK = &h0002
	MSG_DONTROUTE = &h0004
	MSG_EOR = &h0008
	MSG_CTRUNC = &h0010
	MSG_TRUNC = &h0020
	MSG_WAITALL = &h0040
	MSG_DONTWAIT = &h0080
	MSG_NOTIFICATION = &h0100
	MSG_NOSIGNAL = &h0200
	MSG_DUPCTRL = &h0800
	MSG_CMSG_CLOEXEC = &h1000
	MSG_CMSG_CLOFORK = &h2000
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
#define SO_DGRAM_ERRIND &h0200
#define SO_RECVUCRED &h0400
#define SO_SNDBUF &h1001
#define SO_RCVBUF &h1002
#define SO_SNDLOWAT &h1003
#define SO_RCVLOWAT &h1004
#define SO_SNDTIMEO &h1005
#define SO_RCVTIMEO &h1006
#define SO_ERROR &h1007
#define SO_TYPE &h1008
#define SO_PROTOTYPE &h1009
#define SO_PROTOCOL SO_PROTOTYPE
#define SCM_RIGHTS &h1010
#define SOMAXCONN 128

type linger
	l_onoff as long
	l_linger as long
end type

#endif

'' end of crt/sys/solaris/socket.bi
