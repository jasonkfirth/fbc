''
''
'' sys\socket -- NuttX CRT declarations
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_sys_nuttx_socket_bi__
#define __crt_sys_nuttx_socket_bi__

#include once "crt/stdint.bi"
#include once "crt/sys/uio.bi"

#ifndef socklen_t
type socklen_t as __socklen_t
#endif

#define PF_UNSPEC 0
#define PF_UNIX 1
#define PF_LOCAL 1
#define PF_INET 2
#define PF_INET6 10
#define PF_NETLINK 16
#define PF_ROUTE PF_NETLINK
#define PF_PACKET 17
#define PF_CAN 29
#define PF_BLUETOOTH 31
#define PF_IEEE802154 36
#define PF_VSOCK 40
#define PF_PKTRADIO 64
#define PF_RPMSG 65

#define AF_UNSPEC PF_UNSPEC
#define AF_UNIX PF_UNIX
#define AF_LOCAL PF_LOCAL
#define AF_INET PF_INET
#define AF_INET6 PF_INET6
#define AF_NETLINK PF_NETLINK
#define AF_ROUTE PF_ROUTE
#define AF_PACKET PF_PACKET
#define AF_CAN PF_CAN
#define AF_BLUETOOTH PF_BLUETOOTH
#define AF_IEEE802154 PF_IEEE802154
#define AF_VSOCK PF_VSOCK
#define AF_PKTRADIO PF_PKTRADIO
#define AF_RPMSG PF_RPMSG

#define SOCK_UNSPEC 0
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOCK_RAW 3
#define SOCK_RDM 4
#define SOCK_SEQPACKET 5
#define SOCK_CTRL 6
#define SOCK_SMS 7
#define SOCK_PACKET 10
#define SOCK_CLOEXEC 524288
#define SOCK_NONBLOCK 2048
#define SOCK_MAX (SOCK_PACKET + 1)
#define SOCK_TYPE_MASK &hf

enum
	MSG_OOB = &h000001
	MSG_PEEK = &h000002
	MSG_DONTROUTE = &h000004
	MSG_CTRUNC = &h000008
	MSG_PROXY = &h000010
	MSG_TRUNC = &h000020
	MSG_DONTWAIT = &h000040
	MSG_EOR = &h000080
	MSG_WAITALL = &h000100
	MSG_FIN = &h000200
	MSG_SYN = &h000400
	MSG_CONFIRM = &h000800
	MSG_RST = &h001000
	MSG_ERRQUEUE = &h002000
	MSG_NOSIGNAL = &h004000
	MSG_MORE = &h008000
	MSG_CMSG_CLOEXEC = &h100000
end enum

#define SOL_SOCKET 1

#define SO_ACCEPTCONN 0
#define SO_BROADCAST 1
#define SO_DEBUG 2
#define SO_DONTROUTE 3
#define SO_ERROR 4
#define SO_KEEPALIVE 5
#define SO_LINGER 6
#define SO_OOBINLINE 7
#define SO_RCVBUF 8
#define SO_RCVLOWAT 9
#define SO_RCVTIMEO 10
#define SO_REUSEADDR 11
#define SO_SNDBUF 12
#define SO_SNDLOWAT 13
#define SO_SNDTIMEO 14
#define SO_TYPE 15
#define SO_TIMESTAMP 16
#define SO_BINDTODEVICE 17
#define SO_PEERCRED 18
#define SO_TIMESTAMPNS 20
#define SO_SNDBUFFORCE 32
#define SO_RCVBUFFORCE 33
#define SO_RXQ_OVFL 40

#define __SO_PROTOCOL 16
#define SOL_IP IPPROTO_IP
#define SOL_IPV6 IPPROTO_IPV6
#define SOL_TCP IPPROTO_TCP
#define SOL_UDP IPPROTO_UDP
#define SOL_RAW IPPROTO_RAW
#define SOL_ICMPV6 IPPROTO_ICMPV6

#define SOL_HCI 0
#define SOL_L2CAP 6
#define SOL_SCO 17
#define SOL_RFCOMM 18
#define SOL_PACKET 19

#define SHUT_RD 1
#define SHUT_WR 2
#define SHUT_RDWR 3

#define SOMAXCONN 128

type sa_family_t as uint16_t

#ifndef sockaddr
	type sockaddr
		sa_family as sa_family_t
		sa_data(0 to 14-1) as byte
	end type
#endif

type sockaddr_storage
	ss_family as sa_family_t
	ss_pad1(0 to 6-1) as byte
	ss_align as int64_t
	ss_pad2(0 to 112-1) as byte
end type

type linger
	l_onoff as long
	l_linger as long
end type

type msghdr
	msg_name as any ptr
	msg_namelen as socklen_t
	msg_iov as iovec ptr
	msg_iovlen as culong
	msg_control as any ptr
	msg_controllen as culong
	msg_flags as ulong
end type

type cmsghdr
	cmsg_len as culong
	cmsg_level as long
	cmsg_type as long
end type

type ucred
	pid as pid_t
	uid as uid_t
	gid as gid_t
end type

enum
	SCM_RIGHTS = &h01
	SCM_CREDENTIALS = &h02
	SCM_SECURITY = &h03
end enum

#define SCM_TIMESTAMP SO_TIMESTAMP

#endif
