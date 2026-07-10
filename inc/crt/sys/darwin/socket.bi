''
''
'' sys\socket -- Darwin CRT socket declarations
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_sys_darwin_socket_bi__
#define __crt_sys_darwin_socket_bi__

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
#define PF_COIP 20
#define PF_CNT 21
#define PF_IPX 23
#define PF_SIP 24
#define PF_NDRV 27
#define PF_ISDN 28
#define PF_E164 PF_ISDN
#define PF_KEY 29
#define PF_INET6 30
#define PF_NATM 31
#define PF_SYSTEM 32
#define PF_NETBIOS 33
#define PF_PPP 34
#define PF_RESERVED_36 36
#define PF_IEEE80211 37
#define PF_UTUN 38
#define PF_VSOCK 40
#define PF_MAX 41

#define AF_UNSPEC PF_UNSPEC
#define AF_LOCAL PF_LOCAL
#define AF_UNIX PF_UNIX
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
#define AF_COIP PF_COIP
#define AF_CNT PF_CNT
#define AF_IPX PF_IPX
#define AF_SIP PF_SIP
#define AF_NDRV PF_NDRV
#define AF_ISDN PF_ISDN
#define AF_E164 PF_E164
#define AF_KEY PF_KEY
#define AF_INET6 PF_INET6
#define AF_NATM PF_NATM
#define AF_SYSTEM PF_SYSTEM
#define AF_NETBIOS PF_NETBIOS
#define AF_PPP PF_PPP
#define AF_RESERVED_36 PF_RESERVED_36
#define AF_IEEE80211 PF_IEEE80211
#define AF_UTUN PF_UTUN
#define AF_VSOCK PF_VSOCK
#define AF_MAX PF_MAX

#ifndef sockaddr
	type sockaddr
		sa_len as ubyte
		sa_family as sa_family_t
		sa_data(0 to 14-1) as byte
	end type
#endif

#define _SS_MAXSIZE 128
#define SOCK_MAXADDRLEN 255

type sockaddr_storage
	ss_len as ubyte
	ss_family as sa_family_t
	__ss_pad1(0 to 6-1) as ubyte
	__ss_align as longint
	__ss_pad2(0 to 112-1) as ubyte
end type

enum
	MSG_OOB = &h000001
	MSG_PEEK = &h000002
	MSG_DONTROUTE = &h000004
	MSG_EOR = &h000008
	MSG_TRUNC = &h000010
	MSG_CTRUNC = &h000020
	MSG_WAITALL = &h000040
	MSG_DONTWAIT = &h000080
	MSG_EOF = &h000100
	MSG_FLUSH = &h000400
	MSG_HOLD = &h000800
	MSG_SEND = &h001000
	MSG_HAVEMORE = &h002000
	MSG_RCVMORE = &h004000
	MSG_NEEDSA = &h010000
	MSG_NOSIGNAL = &h080000
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
	SCM_TIMESTAMP = &h02
	SCM_CREDS = &h03
	SCM_TIMESTAMP_MONOTONIC = &h04
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
#define SO_LINGER_SEC &h1080
#define SO_OOBINLINE &h0100
#define SO_REUSEPORT &h0200
#define SO_TIMESTAMP &h0400
#define SO_TIMESTAMP_MONOTONIC &h0800
#define SO_DONTTRUNC &h2000
#define SO_WANTMORE &h4000
#define SO_WANTOOBFLAG &h8000
#define SO_SNDBUF &h1001
#define SO_RCVBUF &h1002
#define SO_SNDLOWAT &h1003
#define SO_RCVLOWAT &h1004
#define SO_SNDTIMEO &h1005
#define SO_RCVTIMEO &h1006
#define SO_ERROR &h1007
#define SO_TYPE &h1008
#define SO_NREAD &h1020
#define SO_NKE &h1021
#define SO_NOSIGPIPE &h1022
#define SO_NOADDRERR &h1023
#define SO_NWRITE &h1024
#define SO_REUSESHAREUID &h1025
#define SO_RANDOMPORT &h1082
#define SO_NP_EXTENSIONS &h1083

#define SOMAXCONN 128

type linger
	l_onoff as long
	l_linger as long
end type

#endif

'' end of crt/sys/darwin/socket.bi
