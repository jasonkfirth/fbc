''
'' FreeBASIC macOS socket smoke test
'' ---------------------------------
''
'' File: socket-darwin-smoke.bas
''
'' Purpose:
''
''     Verify that the FreeBASIC CRT networking declarations match the macOS
''     SDK ABI and can drive the native resolver and socket APIs.
''
'' Responsibilities:
''
''     - assert SDK-derived Darwin type sizes, structure offsets, and constants
''     - exercise IPv4 address conversion and localhost name resolution
''     - create a TCP loopback connection and move data through select()
''
'' This file intentionally does NOT contain:
''
''     - internet access
''     - DNS dependencies outside the local resolver
''     - compatibility assertions for non-Darwin targets
''

#include once "crt/sys/types.bi"
#include once "crt/time.bi"
#include once "crt/sys/time.bi"
#include once "crt/sys/select.bi"
#include once "crt/sys/socket.bi"
#include once "crt/netinet/in.bi"
#include once "crt/arpa/inet.bi"
#include once "crt/netdb.bi"
#include once "crt/unistd.bi"

type SockaddrStorageAlignmentProbe
	prefix as ubyte
	value as sockaddr_storage
end type

type MessageHeaderAlignmentProbe
	prefix as ubyte
	value as msghdr
end type

type AddressInfoAlignmentProbe
	prefix as ubyte
	value as addrinfo
end type

type TimeValueAlignmentProbe
	prefix as ubyte
	value as timeval
end type

dim shared as integer failures
dim shared as long descriptor_evaluations

function EvaluatedDescriptor() as long
	descriptor_evaluations += 1
	return 32
end function

sub ExpectEqual _
	( _
		byref label as string, _
		byval actual as longint, _
		byval expected as longint _
	)

	if( actual <> expected ) then
		print "socket-darwin-smoke: "; label; ": expected "; expected; ", got "; actual
		failures += 1
	end if
end sub

sub ExpectTrue( byref label as string, byval condition as integer )
	if( condition = 0 ) then
		print "socket-darwin-smoke: "; label
		failures += 1
	end if
end sub

sub CheckDarwinAbi()
	'' Values in this section come from the corresponding structures and
	'' constants in the macOS SDK shipped with the Xcode command-line tools.
	ExpectEqual "sizeof(sa_family_t)", sizeof( sa_family_t ), 1
	ExpectEqual "sizeof(socklen_t)", sizeof( socklen_t ), 4
	ExpectEqual "sizeof(SOCKET)", sizeof( SOCKET ), 4
	ExpectEqual "sizeof(ssize_t)", sizeof( ssize_t ), 8
	ExpectEqual "sizeof(suseconds_t)", sizeof( suseconds_t ), 4
	ExpectEqual "sizeof(time_t)", sizeof( time_t ), 8
	ExpectEqual "sizeof(__mbstate_t)", sizeof( __mbstate_t ), 128
	ExpectEqual "sizeof(blkcnt_t)", sizeof( blkcnt_t ), 8
	ExpectEqual "sizeof(blksize_t)", sizeof( blksize_t ), 4
	ExpectEqual "sizeof(dev_t)", sizeof( dev_t ), 4
	ExpectEqual "sizeof(fsblkcnt_t)", sizeof( fsblkcnt_t ), 4
	ExpectEqual "sizeof(fsfilcnt_t)", sizeof( fsfilcnt_t ), 4
	ExpectEqual "sizeof(gid_t)", sizeof( gid_t ), 4
	ExpectEqual "sizeof(id_t)", sizeof( id_t ), 4
	ExpectEqual "sizeof(ino_t)", sizeof( ino_t ), 8
	ExpectEqual "sizeof(mode_t)", sizeof( mode_t ), 2
	ExpectEqual "sizeof(nlink_t)", sizeof( nlink_t ), 2
	ExpectEqual "sizeof(off_t)", sizeof( off_t ), 8
	ExpectEqual "sizeof(pid_t)", sizeof( pid_t ), 4
	ExpectEqual "sizeof(uid_t)", sizeof( uid_t ), 4
	ExpectEqual "sizeof(useconds_t)", sizeof( useconds_t ), 4
	ExpectEqual "sizeof(clock_t)", sizeof( clock_t ), 8
	ExpectEqual "sizeof(sigset_t)", sizeof( sigset_t ), 4
	ExpectEqual "sizeof(register_t)", sizeof( register_t ), 8

	ExpectEqual "sizeof(sockaddr)", sizeof( sockaddr ), 16
	ExpectEqual "sockaddr.sa_len", offsetof( sockaddr, sa_len ), 0
	ExpectEqual "sockaddr.sa_family", offsetof( sockaddr, sa_family ), 1
	ExpectEqual "sockaddr.sa_data", offsetof( sockaddr, sa_data(0) ), 2

	ExpectEqual "sizeof(sockaddr_storage)", sizeof( sockaddr_storage ), 128
	ExpectEqual "sockaddr_storage.ss_len", offsetof( sockaddr_storage, ss_len ), 0
	ExpectEqual "sockaddr_storage.ss_family", offsetof( sockaddr_storage, ss_family ), 1
	ExpectEqual "sockaddr_storage.__ss_align", offsetof( sockaddr_storage, __ss_align ), 8
	ExpectEqual "sockaddr_storage alignment", _
		offsetof( SockaddrStorageAlignmentProbe, value ), 8

	ExpectEqual "sizeof(msghdr)", sizeof( msghdr ), 48
	ExpectEqual "msghdr.msg_name", offsetof( msghdr, msg_name ), 0
	ExpectEqual "msghdr.msg_namelen", offsetof( msghdr, msg_namelen ), 8
	ExpectEqual "msghdr.msg_iov", offsetof( msghdr, msg_iov ), 16
	ExpectEqual "msghdr.msg_iovlen", offsetof( msghdr, msg_iovlen ), 24
	ExpectEqual "msghdr.msg_control", offsetof( msghdr, msg_control ), 32
	ExpectEqual "msghdr.msg_controllen", offsetof( msghdr, msg_controllen ), 40
	ExpectEqual "msghdr.msg_flags", offsetof( msghdr, msg_flags ), 44
	ExpectEqual "msghdr alignment", offsetof( MessageHeaderAlignmentProbe, value ), 8

	ExpectEqual "sizeof(cmsghdr)", sizeof( cmsghdr ), 12
	ExpectEqual "cmsghdr.cmsg_len", offsetof( cmsghdr, cmsg_len ), 0
	ExpectEqual "cmsghdr.cmsg_level", offsetof( cmsghdr, cmsg_level ), 4
	ExpectEqual "cmsghdr.cmsg_type", offsetof( cmsghdr, cmsg_type ), 8
	ExpectEqual "sizeof(linger)", sizeof( linger ), 8

	ExpectEqual "sizeof(sockaddr_in)", sizeof( sockaddr_in ), 16
	ExpectEqual "sockaddr_in.sin_len", offsetof( sockaddr_in, sin_len ), 0
	ExpectEqual "sockaddr_in.sin_family", offsetof( sockaddr_in, sin_family ), 1
	ExpectEqual "sockaddr_in.sin_port", offsetof( sockaddr_in, sin_port ), 2
	ExpectEqual "sockaddr_in.sin_addr", offsetof( sockaddr_in, sin_addr ), 4
	ExpectEqual "sockaddr_in.sin_zero", offsetof( sockaddr_in, sin_zero(0) ), 8

	ExpectEqual "sizeof(sockaddr_in6)", sizeof( sockaddr_in6 ), 28
	ExpectEqual "sockaddr_in6.sin6_len", offsetof( sockaddr_in6, sin6_len ), 0
	ExpectEqual "sockaddr_in6.sin6_family", offsetof( sockaddr_in6, sin6_family ), 1
	ExpectEqual "sockaddr_in6.sin6_port", offsetof( sockaddr_in6, sin6_port ), 2
	ExpectEqual "sockaddr_in6.sin6_flowinfo", offsetof( sockaddr_in6, sin6_flowinfo ), 4
	ExpectEqual "sockaddr_in6.sin6_addr", offsetof( sockaddr_in6, sin6_addr ), 8
	ExpectEqual "sockaddr_in6.sin6_scope_id", offsetof( sockaddr_in6, sin6_scope_id ), 24

	ExpectEqual "sizeof(ip_mreq_source)", sizeof( ip_mreq_source ), 12
	ExpectEqual "ip_mreq_source.imr_sourceaddr", offsetof( ip_mreq_source, imr_sourceaddr ), 4
	ExpectEqual "ip_mreq_source.imr_interface", offsetof( ip_mreq_source, imr_interface ), 8
	ExpectEqual "sizeof(in_pktinfo)", sizeof( in_pktinfo ), 12
	ExpectEqual "in_pktinfo.ipi_ifindex", offsetof( in_pktinfo, ipi_ifindex ), 0
	ExpectEqual "in_pktinfo.ipi_spec_dst", offsetof( in_pktinfo, ipi_spec_dst ), 4
	ExpectEqual "in_pktinfo.ipi_addr", offsetof( in_pktinfo, ipi_addr ), 8
	ExpectEqual "sizeof(ipv6_mreq)", sizeof( ipv6_mreq ), 20
	ExpectEqual "sizeof(in6_pktinfo)", sizeof( in6_pktinfo ), 20
	ExpectEqual "sizeof(group_req)", sizeof( group_req ), 132
	ExpectEqual "group_req.gr_group", offsetof( group_req, gr_group ), 4
	ExpectEqual "sizeof(group_source_req)", sizeof( group_source_req ), 260
	ExpectEqual "group_source_req.gsr_group", offsetof( group_source_req, gsr_group ), 4
	ExpectEqual "group_source_req.gsr_source", offsetof( group_source_req, gsr_source ), 132

	ExpectEqual "sizeof(addrinfo)", sizeof( addrinfo ), 48
	ExpectEqual "addrinfo.ai_addrlen", offsetof( addrinfo, ai_addrlen ), 16
	ExpectEqual "addrinfo.ai_canonname", offsetof( addrinfo, ai_canonname ), 24
	ExpectEqual "addrinfo.ai_addr", offsetof( addrinfo, ai_addr ), 32
	ExpectEqual "addrinfo.ai_next", offsetof( addrinfo, ai_next ), 40
	ExpectEqual "addrinfo alignment", offsetof( AddressInfoAlignmentProbe, value ), 8
	ExpectEqual "sizeof(hostent)", sizeof( hostent ), 32
	ExpectEqual "hostent.h_addrtype", offsetof( hostent, h_addrtype ), 16
	ExpectEqual "hostent.h_length", offsetof( hostent, h_length ), 20
	ExpectEqual "hostent.h_addr_list", offsetof( hostent, h_addr_list ), 24
	ExpectEqual "sizeof(netent)", sizeof( netent ), 24
	ExpectEqual "netent.n_net", offsetof( netent, n_net ), 20
	ExpectEqual "sizeof(servent)", sizeof( servent ), 32
	ExpectEqual "servent.s_proto", offsetof( servent, s_proto ), 24
	ExpectEqual "sizeof(protoent)", sizeof( protoent ), 24
	ExpectEqual "protoent.p_proto", offsetof( protoent, p_proto ), 16

	ExpectEqual "sizeof(fd_mask)", sizeof( fd_mask ), 4
	ExpectEqual "sizeof(fd_set)", sizeof( fd_set ), 128
	ExpectEqual "sizeof(timeval)", sizeof( timeval ), 16
	ExpectEqual "timeval.tv_sec", offsetof( timeval, tv_sec ), 0
	ExpectEqual "timeval.tv_usec", offsetof( timeval, tv_usec ), 8
	ExpectEqual "timeval alignment", offsetof( TimeValueAlignmentProbe, value ), 8
	ExpectEqual "sizeof(timespec)", sizeof( timespec ), 16
	ExpectEqual "timespec.tv_sec", offsetof( timespec, tv_sec ), 0
	ExpectEqual "timespec.tv_nsec", offsetof( timespec, tv_nsec ), 8
	ExpectEqual "sizeof(tm)", sizeof( tm ), 56
	ExpectEqual "tm.tm_gmtoff", offsetof( tm, tm_gmtoff ), 40
	ExpectEqual "tm.tm_zone", offsetof( tm, tm_zone ), 48

	ExpectEqual "AF_INET6", AF_INET6, 30
	ExpectEqual "SOL_SOCKET", SOL_SOCKET, &hffff
	ExpectEqual "SO_NOSIGPIPE", SO_NOSIGPIPE, &h1022
	ExpectEqual "MSG_NOSIGNAL", MSG_NOSIGNAL, &h80000
	ExpectEqual "AI_NUMERICSERV", AI_NUMERICSERV, &h1000
	ExpectEqual "EAI_OVERFLOW", EAI_OVERFLOW, 14
	ExpectEqual "NI_NUMERICSCOPE", NI_NUMERICSCOPE, &h100

#if defined(__APPLE_USE_RFC_2292)
	ExpectEqual "RFC 2292 IPV6_PKTINFO", IPV6_PKTINFO, 19
	ExpectEqual "RFC 2292 IPV6_HOPLIMIT", IPV6_HOPLIMIT, 20
#elseif defined(__APPLE_USE_RFC_3542)
	ExpectEqual "RFC 3542 IPV6_PKTINFO", IPV6_PKTINFO, 46
	ExpectEqual "RFC 3542 IPV6_HOPLIMIT", IPV6_HOPLIMIT, 47
	ExpectEqual "RFC 3542 IPV6_RECVPKTINFO", IPV6_RECVPKTINFO, 61
#else
	#ifdef IPV6_PKTINFO
		#error IPV6_PKTINFO requires an Apple RFC 2292 or RFC 3542 API selection
	#endif
#endif

	'' Exercise both sides of a 32-bit fd_set storage boundary.  A 64-bit mask
	'' implementation has the same total structure size but the wrong C layout.
	dim as fd_set descriptor_set
	dim as integer negative_descriptor = -1
	dim as integer past_last_descriptor = FD_SETSIZE
	FD_ZERO( @descriptor_set )
	FD_SET_( 31, @descriptor_set )
	FD_SET_( 32, @descriptor_set )
	ExpectTrue "FD_SET_ lost descriptor 31", FD_ISSET( 31, @descriptor_set )
	ExpectTrue "FD_SET_ lost descriptor 32", FD_ISSET( 32, @descriptor_set )
	descriptor_evaluations = 0
	ExpectTrue "FD_ISSET lost evaluated descriptor", _
		FD_ISSET( EvaluatedDescriptor(), @descriptor_set )
	ExpectEqual "FD_ISSET descriptor evaluation count", descriptor_evaluations, 1
	FD_CLR( 31, @descriptor_set )
	ExpectTrue "FD_CLR left descriptor 31 set", not FD_ISSET( 31, @descriptor_set )
	FD_SET_( negative_descriptor, @descriptor_set )
	FD_SET_( past_last_descriptor, @descriptor_set )
	ExpectTrue "FD_ISSET accepted descriptor -1", _
		not FD_ISSET( negative_descriptor, @descriptor_set )
	ExpectTrue "FD_ISSET accepted FD_SETSIZE", _
		not FD_ISSET( past_last_descriptor, @descriptor_set )
end sub

sub CheckResolver()
	dim as in_addr loopback_address
	dim as addrinfo hints
	dim as addrinfo ptr results
	dim as timespec monotonic_time

	ExpectEqual "clock_gettime monotonic", _
		clock_gettime( CLOCK_MONOTONIC, @monotonic_time ), 0
	ExpectTrue "clock_gettime returned a negative time", monotonic_time.tv_sec >= 0

	ExpectEqual "inet_pton IPv4", inet_pton( AF_INET, "127.0.0.1", @loopback_address ), 1
	ExpectEqual "inet_pton loopback value", ntohl( loopback_address.s_addr ), INADDR_LOOPBACK

	hints.ai_family = AF_UNSPEC
	hints.ai_socktype = SOCK_STREAM

	dim as long resolver_result = getaddrinfo( "localhost", "80", @hints, @results )
	ExpectEqual "getaddrinfo localhost", resolver_result, 0

	if( resolver_result = 0 ) then
		ExpectTrue "getaddrinfo returned no addresses", results <> 0

		if( results <> 0 ) then
			ExpectTrue "getaddrinfo returned an empty socket address", _
				results->ai_addr <> 0 andalso results->ai_addrlen > 0
		end if

		if( results <> 0 ) then
			freeaddrinfo results
		end if
	end if
end sub

sub CheckLoopbackSocket()
	dim as long listener = -1
	dim as long client = -1
	dim as long server = -1
	dim as sockaddr_in address
	dim as socklen_t address_length
	dim as long option_value = 1
	dim as zstring * 5 outbound = "ping"
	dim as zstring * 5 inbound
	dim as fd_set readable
	dim as timeval timeout_value
	dim as timespec pselect_timeout
	dim as long selected
	dim as ssize_t received

	listener = opensocket( AF_INET, SOCK_STREAM, IPPROTO_TCP )
	if( listener < 0 ) then
		ExpectTrue "could not create listener socket", false
		exit sub
	end if

	if( setsockopt( listener, SOL_SOCKET, SO_REUSEADDR, @option_value, sizeof( option_value ) ) <> 0 ) then
		ExpectTrue "setsockopt(SO_REUSEADDR) failed", false
		goto cleanup
	end if

	address.sin_len = sizeof( address )
	address.sin_family = AF_INET
	address.sin_port = 0
	address.sin_addr.s_addr = htonl( INADDR_LOOPBACK )

	if( bind( listener, cptr( sockaddr ptr, @address ), sizeof( address ) ) <> 0 ) then
		ExpectTrue "bind loopback failed", false
		goto cleanup
	end if

	if( listen( listener, 1 ) <> 0 ) then
		ExpectTrue "listen failed", false
		goto cleanup
	end if

	address_length = sizeof( address )
	if( getsockname( listener, cptr( sockaddr ptr, @address ), @address_length ) <> 0 ) then
		ExpectTrue "getsockname failed", false
		goto cleanup
	end if

	ExpectEqual "getsockname address length", address_length, sizeof( address )
	ExpectTrue "getsockname did not assign a port", address.sin_port <> 0

	client = opensocket( AF_INET, SOCK_STREAM, IPPROTO_TCP )
	if( client < 0 ) then
		ExpectTrue "could not create client socket", false
		goto cleanup
	end if

	if( connect( client, cptr( sockaddr ptr, @address ), sizeof( address ) ) <> 0 ) then
		ExpectTrue "connect loopback failed", false
		goto cleanup
	end if

	server = accept( listener, 0, 0 )
	if( server < 0 ) then
		ExpectTrue "accept loopback failed", false
		goto cleanup
	end if

	ExpectEqual "sockatmark without urgent data", sockatmark( server ), 0
	ExpectEqual "send loopback", send( client, @outbound, 4, 0 ), 4

	FD_ZERO( @readable )
	FD_SET_( server, @readable )
	pselect_timeout.tv_sec = 1
	pselect_timeout.tv_nsec = 0
	selected = pselect( server + 1, @readable, 0, 0, @pselect_timeout, 0 )
	ExpectEqual "pselect loopback", selected, 1
	ExpectTrue "pselect did not mark server readable", FD_ISSET( server, @readable )

	FD_ZERO( @readable )
	FD_SET_( server, @readable )
	timeout_value.tv_sec = 1
	timeout_value.tv_usec = 0

	selected = selectsocket( server + 1, @readable, 0, 0, @timeout_value )
	ExpectEqual "select loopback", selected, 1
	ExpectTrue "select did not mark server readable", FD_ISSET( server, @readable )

	if( selected = 1 ) then
		received = recv( server, @inbound, 4, 0 )
		ExpectEqual "recv loopback", received, 4

		if( received = 4 ) then
			inbound[4] = 0
			ExpectTrue "loopback payload mismatch", inbound = "ping"
		end if
	end if

cleanup:
	if( server >= 0 ) then
		closesocket server
	end if

	if( client >= 0 ) then
		closesocket client
	end if

	if( listener >= 0 ) then
		closesocket listener
	end if
end sub

CheckDarwinAbi()
CheckResolver()
CheckLoopbackSocket()

if( failures <> 0 ) then
	end 1
end if

print "socket-darwin-smoke: ok"
end 0

'' end of socket-darwin-smoke.bas
