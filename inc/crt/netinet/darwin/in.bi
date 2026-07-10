''
''
'' netinet\in -- Darwin-specific network declarations
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_netinet_darwin_in_bi__
#define __crt_netinet_darwin_in_bi__

'' Darwin supports the Linux-compatible indexed multicast request in addition
'' to the traditional BSD ip_mreq structure.
type ip_mreqn
	imr_multiaddr as in_addr
	imr_address as in_addr
	imr_ifindex as long
end type

'' IP_PKTINFO uses this structure for both received destination metadata and
'' caller-selected IPv4 source interfaces.
type in_pktinfo
	ipi_ifindex as ulong
	ipi_spec_dst as in_addr
	ipi_addr as in_addr
end type

#define IP_OPTIONS 1
#define IP_HDRINCL 2
#define IP_TOS 3
#define IP_TTL 4
#define IP_RECVOPTS 5
#define IP_RECVRETOPTS 6
#define IP_RECVDSTADDR 7
#define IP_RETOPTS 8
#define IP_MULTICAST_IF 9
#define IP_MULTICAST_TTL 10
#define IP_MULTICAST_LOOP 11
#define IP_ADD_MEMBERSHIP 12
#define IP_DROP_MEMBERSHIP 13
#define IP_MULTICAST_VIF 14
#define IP_RSVP_ON 15
#define IP_RSVP_OFF 16
#define IP_RSVP_VIF_ON 17
#define IP_RSVP_VIF_OFF 18
#define IP_PORTRANGE 19
#define IP_RECVIF 20
#define IP_IPSEC_POLICY 21
#define IP_FAITH 22
#define IP_STRIPHDR 23
#define IP_RECVTTL 24
#define IP_BOUND_IF 25
#define IP_PKTINFO 26
#define IP_RECVPKTINFO IP_PKTINFO
#define IP_RECVTOS 27
#define IP_DONTFRAG 28
#define IP_MULTICAST_IFINDEX 66
#define IP_ADD_SOURCE_MEMBERSHIP 70
#define IP_DROP_SOURCE_MEMBERSHIP 71
#define IP_BLOCK_SOURCE 72
#define IP_UNBLOCK_SOURCE 73
#define IP_MSFILTER 74

#define MCAST_JOIN_GROUP 80
#define MCAST_LEAVE_GROUP 81
#define MCAST_JOIN_SOURCE_GROUP 82
#define MCAST_LEAVE_SOURCE_GROUP 83
#define MCAST_BLOCK_SOURCE 84
#define MCAST_UNBLOCK_SOURCE 85

#define MCAST_UNDEFINED 0
#define MCAST_INCLUDE 1
#define MCAST_EXCLUDE 2

#define IP_DEFAULT_MULTICAST_TTL 1
#define IP_DEFAULT_MULTICAST_LOOP 1
#define IP_PORTRANGE_DEFAULT 0
#define IP_PORTRANGE_HIGH 1
#define IP_PORTRANGE_LOW 2

#define IPV6_SOCKOPT_RESERVED1 3
#define IPV6_UNICAST_HOPS 4
#define IPV6_MULTICAST_IF 9
#define IPV6_MULTICAST_HOPS 10
#define IPV6_MULTICAST_LOOP 11
#define IPV6_JOIN_GROUP 12
#define IPV6_LEAVE_GROUP 13
#define IPV6_PORTRANGE 14
#define IPV6_2292PKTINFO 19
#define IPV6_2292HOPLIMIT 20
#define IPV6_2292NEXTHOP 21
#define IPV6_2292HOPOPTS 22
#define IPV6_2292DSTOPTS 23
#define IPV6_2292RTHDR 24
#define IPV6_2292PKTOPTIONS 25
#define IPV6_CHECKSUM 26
#define IPV6_V6ONLY 27
#define IPV6_BINDV6ONLY IPV6_V6ONLY
#define IPV6_IPSEC_POLICY 28
#define IPV6_FAITH 29
#define IPV6_RECVTCLASS 35
#define IPV6_TCLASS 36
#define IPV6_BOUND_IF 125

'' Apple requires callers to select one advanced IPv6 API before including
'' netinet/in.h.  Mirror that behavior so the shared option names cannot
'' silently select the wrong control-message ABI.
#if defined(__APPLE_USE_RFC_2292) and defined(__APPLE_USE_RFC_3542)
#error __APPLE_USE_RFC_2292 and __APPLE_USE_RFC_3542 cannot both be defined
#endif

#if defined(__APPLE_USE_RFC_2292)
#define IPV6_PKTINFO IPV6_2292PKTINFO
#define IPV6_HOPLIMIT IPV6_2292HOPLIMIT
#define IPV6_NEXTHOP IPV6_2292NEXTHOP
#define IPV6_HOPOPTS IPV6_2292HOPOPTS
#define IPV6_DSTOPTS IPV6_2292DSTOPTS
#define IPV6_RTHDR IPV6_2292RTHDR
#define IPV6_PKTOPTIONS IPV6_2292PKTOPTIONS
#elseif defined(__APPLE_USE_RFC_3542)
#define IPV6_RECVHOPLIMIT 37
#define IPV6_RECVRTHDR 38
#define IPV6_RECVHOPOPTS 39
#define IPV6_RECVDSTOPTS 40
#define IPV6_USE_MIN_MTU 42
#define IPV6_RECVPATHMTU 43
#define IPV6_PATHMTU 44
#define IPV6_3542PKTINFO 46
#define IPV6_3542HOPLIMIT 47
#define IPV6_3542NEXTHOP 48
#define IPV6_3542HOPOPTS 49
#define IPV6_3542DSTOPTS 50
#define IPV6_3542RTHDR 51
#define IPV6_RTHDRDSTOPTS 57
#define IPV6_AUTOFLOWLABEL 59
#define IPV6_RECVPKTINFO 61
#define IPV6_DONTFRAG 62
#define IPV6_PREFER_TEMPADDR 63
#define IPV6_MSFILTER 74
#define IPV6_PKTINFO IPV6_3542PKTINFO
#define IPV6_HOPLIMIT IPV6_3542HOPLIMIT
#define IPV6_NEXTHOP IPV6_3542NEXTHOP
#define IPV6_HOPOPTS IPV6_3542HOPOPTS
#define IPV6_DSTOPTS IPV6_3542DSTOPTS
#define IPV6_RTHDR IPV6_3542RTHDR
#endif

#define IPV6_DEFAULT_MULTICAST_HOPS 1
#define IPV6_DEFAULT_MULTICAST_LOOP 1
#define IPV6_PORTRANGE_DEFAULT 0
#define IPV6_PORTRANGE_HIGH 1
#define IPV6_PORTRANGE_LOW 2

#endif

'' end of crt/netinet/darwin/in.bi
