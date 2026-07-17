' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC CRT socket binding test
'' ---------------------------------
''
'' File: tests/crt/socket.bas
''
'' Purpose:
''
''     Verify that the shipped Unix socket declarations compile, link, and
''     move data through the host operating system's socket API.
''
'' Responsibilities:
''
''     - include crt/sys/socket.bi exactly as user programs do
''     - verify important platform address-structure layouts
''     - create a local stream connection or socket pair
''     - transfer data and close the descriptors through the binding
''
'' This file intentionally does NOT contain:
''
''     - FreeBASIC OPEN TCP runtime coverage
''     - external network or DNS access
''

#if defined(__FB_LINUX__) or defined(__FB_ANDROID__) or defined(__FB_CYGWIN__) or _
    defined(__FB_FREEBSD__) or defined(__FB_DRAGONFLY__) or defined(__FB_OPENBSD__) or _
    defined(__FB_NETBSD__) or defined(__FB_DARWIN__) or defined(__FB_HAIKU__) or _
    defined(__FB_SOLARIS__)

#include once "crt/sys/socket.bi"
#include once "crt/netinet/in.bi"

#ifdef __FB_FREEBSD__
	if sizeof(sockaddr) <> 16 then end 8
	if sizeof(sockaddr_storage) <> 128 then end 9
	if sizeof(cmsghdr) <> 12 then end 10
	#ifdef __FB_64BIT__
		if sizeof(msghdr) <> 48 then end 11
	#else
		if sizeof(msghdr) <> 28 then end 11
	#endif
	if AF_INET6 <> 28 then end 12
	if sizeof(sockaddr_in) <> 16 then end 13
	if offsetof(sockaddr_in, sin_len) <> 0 then end 14
	if offsetof(sockaddr_in, sin_family) <> 1 then end 15
	if sizeof(sockaddr_in6) <> 28 then end 16
#endif

#ifdef __FB_HAIKU__
	if sizeof(sockaddr) <> 32 then end 13
	if sizeof(sockaddr_storage) <> 128 then end 14
	if sizeof(cmsghdr) <> 12 then end 15
	#ifdef __FB_64BIT__
		if sizeof(msghdr) <> 48 then end 16
	#else
		if sizeof(msghdr) <> 28 then end 16
	#endif
	if AF_INET6 <> 5 then end 17
	if sizeof(sockaddr_in) <> 32 then end 18
	if offsetof(sockaddr_in, sin_len) <> 0 then end 19
	if offsetof(sockaddr_in, sin_family) <> 1 then end 35
	if sizeof(sockaddr_in6) <> 28 then end 36
#endif

#ifdef __FB_SOLARIS__
	if sizeof(sockaddr) <> 16 then end 13
	if sizeof(sockaddr_storage) <> 256 then end 14
	if sizeof(cmsghdr) <> 12 then end 15
	#ifdef __FB_64BIT__
		if sizeof(msghdr) <> 48 then end 16
	#else
		if sizeof(msghdr) <> 28 then end 16
	#endif
	if AF_INET6 <> 26 then end 17
	if sizeof(sockaddr_in) <> 16 then end 18
	if offsetof(sockaddr_in, sin_family) <> 0 then end 19
	if sizeof(sockaddr_in6) <> 32 then end 35
#endif

dim fds(0 to 1) as long
dim sent as string = "FreeBASIC socket.bi smoke"
dim received as string = space(len(sent))

if socketpair(AF_UNIX, SOCK_STREAM, 0, @fds(0)) <> 0 then end 1
if send(fds(0), strptr(sent), len(sent), 0) <> len(sent) then end 2
if recv(fds(1), strptr(received), len(received), 0) <> len(received) then end 3
if received <> sent then end 4
if shutdown(fds(0), SHUT_WR) <> 0 then end 5
if closesocket(fds(0)) <> 0 then end 6
if closesocket(fds(1)) <> 0 then end 7

#elseif defined(__FB_WIN32__)

#include once "win/winsock2.bi"

dim wsa_data as WSADATA
dim listener as SOCKET = INVALID_SOCKET
dim client as SOCKET = INVALID_SOCKET
dim peer as SOCKET = INVALID_SOCKET
dim address as SOCKADDR_IN
dim address_length as long = sizeof(address)
dim sent as string = "FreeBASIC winsock2.bi smoke"
dim received as string = space(len(sent))

if WSAStartup(MAKEWORD(2, 2), @wsa_data) <> 0 then end 20

listener = socket_(AF_INET, SOCK_STREAM, IPPROTO_TCP)
if listener = INVALID_SOCKET then end 21

address.sin_family = AF_INET
address.sin_port = 0
address.sin_addr.s_addr = htonl(INADDR_LOOPBACK)

if bind(listener, cast(SOCKADDR ptr, @address), sizeof(address)) = SOCKET_ERROR then end 22
if getsockname(listener, cast(SOCKADDR ptr, @address), @address_length) = SOCKET_ERROR then end 23
if listen(listener, 1) = SOCKET_ERROR then end 24

client = socket_(AF_INET, SOCK_STREAM, IPPROTO_TCP)
if client = INVALID_SOCKET then end 25
if connect(client, cast(SOCKADDR ptr, @address), sizeof(address)) = SOCKET_ERROR then end 26

peer = accept(listener, NULL, NULL)
if peer = INVALID_SOCKET then end 27

if send(client, strptr(sent), len(sent), 0) <> len(sent) then end 28
if recv(peer, strptr(received), len(received), 0) <> len(received) then end 29
if received <> sent then end 30

if closesocket(peer) = SOCKET_ERROR then end 31
if closesocket(client) = SOCKET_ERROR then end 32
if closesocket(listener) = SOCKET_ERROR then end 33
if WSACleanup() = SOCKET_ERROR then end 34

#endif

'' end of tests/crt/socket.bas
