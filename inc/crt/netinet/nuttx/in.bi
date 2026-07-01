''
''
'' netinet\in -- NuttX CRT declarations
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_netinet_nuttx_in_bi__
#define __crt_netinet_nuttx_in_bi__

#define IP_MULTICAST_IF (__SO_PROTOCOL + 1)
#define IP_MULTICAST_TTL (__SO_PROTOCOL + 2)
#define IP_MULTICAST_LOOP (__SO_PROTOCOL + 3)
#define IP_ADD_MEMBERSHIP (__SO_PROTOCOL + 4)
#define IP_DROP_MEMBERSHIP (__SO_PROTOCOL + 5)
#define IP_UNBLOCK_SOURCE (__SO_PROTOCOL + 6)
#define IP_BLOCK_SOURCE (__SO_PROTOCOL + 7)
#define IP_ADD_SOURCE_MEMBERSHIP (__SO_PROTOCOL + 8)
#define IP_DROP_SOURCE_MEMBERSHIP (__SO_PROTOCOL + 9)
#define IP_MSFILTER_ (__SO_PROTOCOL + 10)
#define IP_MULTICAST_ALL (__SO_PROTOCOL + 11)
#define IP_PKTINFO (__SO_PROTOCOL + 12)
#define IP_TOS (__SO_PROTOCOL + 13)
#define IP_TTL (__SO_PROTOCOL + 14)

#define IPV6_JOIN_GROUP (__SO_PROTOCOL + 1)
#define IPV6_LEAVE_GROUP (__SO_PROTOCOL + 2)
#define IPV6_MULTICAST_HOPS (__SO_PROTOCOL + 3)
#define IPV6_MULTICAST_IF (__SO_PROTOCOL + 4)
#define IPV6_MULTICAST_LOOP (__SO_PROTOCOL + 5)
#define IPV6_UNICAST_HOPS (__SO_PROTOCOL + 6)
#define IPV6_V6ONLY (__SO_PROTOCOL + 7)
#define IPV6_PKTINFO (__SO_PROTOCOL + 8)
#define IPV6_RECVPKTINFO (__SO_PROTOCOL + 9)
#define IPV6_TCLASS (__SO_PROTOCOL + 10)
#define IPV6_RECVHOPLIMIT (__SO_PROTOCOL + 11)
#define IPV6_HOPLIMIT (__SO_PROTOCOL + 12)

#define MCAST_EXCLUDE 0
#define MCAST_INCLUDE 1

#endif
