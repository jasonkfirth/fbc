''
'' FreeBASIC CRT socket declarations
'' ---------------------------------
''
'' File: crt/sys/haiku/socket.bi
''
'' Purpose:
''
''     Describe the Haiku socket ABI used by crt/sys/socket.bi.
''
'' Responsibilities:
''
''     - define Haiku socket types and address families
''     - define the public socket address and message structures
''     - define commonly used socket options and message flags
''
'' This file intentionally does NOT contain:
''
''     - socket function declarations shared by Unix targets
''     - FreeBASIC OPEN TCP runtime behavior
''     - resolver declarations from netdb.bi
''

#ifndef __crt_sys_haiku_socket_bi__
#define __crt_sys_haiku_socket_bi__

#include once "crt/stdint.bi"
#include once "crt/sys/types.bi"
#include once "crt/sys/uio.bi"

#ifndef socklen_t
type socklen_t as uint32_t
#endif

type sa_family_t as uint8_t

enum __socket_type
	SOCK_STREAM = 1
	SOCK_DGRAM = 2
	SOCK_RAW = 3
	SOCK_SEQPACKET = 5
	SOCK_MISC = 255
end enum

#define SOCK_NONBLOCK &h00040000
#define SOCK_CLOEXEC &h00080000
#define SOCK_CLOFORK &h00100000

#define AF_UNSPEC 0
#define AF_INET 1
#define AF_APPLETALK 2
#define AF_ROUTE 3
#define AF_LINK 4
#define AF_INET6 5
#define AF_DLI 6
#define AF_IPX 7
#define AF_NOTIFY 8
#define AF_LOCAL 9
#define AF_UNIX AF_LOCAL
#define AF_BLUETOOTH 10
#define AF_MAX 11

#define PF_UNSPEC AF_UNSPEC
#define PF_INET AF_INET
#define PF_ROUTE AF_ROUTE
#define PF_LINK AF_LINK
#define PF_INET6 AF_INET6
#define PF_LOCAL AF_LOCAL
#define PF_UNIX AF_UNIX
#define PF_BLUETOOTH AF_BLUETOOTH

#ifndef sockaddr
	type sockaddr
		sa_len as uint8_t
		sa_family as sa_family_t
		sa_data(0 to 30-1) as uint8_t
	end type
#endif

type sockaddr_storage
	ss_len as uint8_t
	ss_family as sa_family_t
	__ss_pad1(0 to 6-1) as uint8_t
	__ss_pad2 as uint64_t
	__ss_pad3(0 to 112-1) as uint8_t
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
	MSG_BCAST = &h0100
	MSG_MCAST = &h0200
	MSG_EOF = &h0400
	MSG_NOSIGNAL = &h0800
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

enum
	SCM_RIGHTS = &h01
end enum

#define SOL_SOCKET -1
#define SO_ACCEPTCONN &h00000001
#define SO_BROADCAST &h00000002
#define SO_DEBUG &h00000004
#define SO_DONTROUTE &h00000008
#define SO_KEEPALIVE &h00000010
#define SO_OOBINLINE &h00000020
#define SO_REUSEADDR &h00000040
#define SO_REUSEPORT &h00000080
#define SO_USELOOPBACK &h00000100
#define SO_LINGER &h00000200
#define SO_SNDBUF &h40000001
#define SO_SNDLOWAT &h40000002
#define SO_SNDTIMEO &h40000003
#define SO_RCVBUF &h40000004
#define SO_RCVLOWAT &h40000005
#define SO_RCVTIMEO &h40000006
#define SO_ERROR &h40000007
#define SO_TYPE &h40000008
#define SO_NONBLOCK &h40000009
#define SO_BINDTODEVICE &h4000000a
#define SO_PEERCRED &h4000000b
#define SOMAXCONN 32

type linger
	l_onoff as long
	l_linger as long
end type

#endif

'' end of crt/sys/haiku/socket.bi
