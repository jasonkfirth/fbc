'' Project: FreeBASIC network examples
'' -----------------------------------
''
'' File: examples/network/http-get.bas
''
'' Purpose:
''
''     Send a simple HTTP GET request and print the response.
''
'' Responsibilities:
''
''     - use the DOS TCP file device or native Windows/Unix sockets
''     - parse the host and optional path from the command line
''     - bound response reads and waiting on DOS
''
'' This file intentionally does NOT contain:
''
''     - HTTPS, redirects, or general HTTP response parsing
''     - DOS packet-driver installation or Watt-32 configuration
''
'' DOS uses FreeBASIC's TCP file device and needs a Watt-32-enabled runtime.

#ifdef __FB_DOS__

'' -------------------------------------------------------------------------
'' DOS TCP file-device implementation
'' -------------------------------------------------------------------------

const DEFAULT_HOST = "www.freebasic.net"
'' HTTP's standard TCP service port.
const HTTP_PORT = "80"
'' Keep reads under 4 KiB and cap idle polling. Each poll requests a 10 ms sleep.
const RECVBUFFLEN = 4096
const WAIT_SLICE_MS = 10
const MAX_IDLE_TICKS = 1000

declare sub getHostAndPath _
	( _
		byref src as string, _
		byref hostname as string, _
		byref path as string _
	)

dim hostname as string
dim path as string
dim chunk as string
dim source as string = command( )
dim f as integer
dim idle_ticks as integer = 0
dim saw_data as integer = FALSE

getHostAndPath( source, hostname, path )
if( len( hostname ) = 0 ) then
	hostname = DEFAULT_HOST
end if

f = freefile( )
if( OPEN TCP( "host=" & hostname & ",port=" & HTTP_PORT AS #f ) <> 0 ) then
	print "OPEN TCP failed, ERR="; err
	end 1
end if

print #f, "GET /" & path & " HTTP/1.0"
print #f, "Host: " & hostname
print #f, "Connection: close"
print #f, "User-Agent: FreeBASIC DOS HTTP example"
print #f,

do
	if( eof( f ) = 0 ) then
		chunk = input( RECVBUFFLEN, #f )
		if( len( chunk ) <> 0 ) then
			print chunk;
			saw_data = TRUE
			idle_ticks = 0
		else
			sleep WAIT_SLICE_MS, 1
			idle_ticks += 1
		end if
	elseif( eoc( f ) <> 0 ) then
		exit do
	else
		sleep WAIT_SLICE_MS, 1
		idle_ticks += 1
	end if
	if( idle_ticks >= MAX_IDLE_TICKS ) then
		print "Timed out waiting for HTTP response"
		close #f
		end 1
	end if
loop

close #f
if( saw_data = FALSE ) then
	print "No HTTP response received"
	end 1
end if

sub getHostAndPath _
	( _
		byref src as string, _
		byref hostname as string, _
		byref path as string _
	)

	dim p as integer = instr( src, " " )
	if( p = 0 or p = len( src ) ) then
		hostname = trim( src )
		path = ""
	else
		hostname = trim( left( src, p-1 ) )
		path = trim( mid( src, p+1 ) )
	end if

end sub

#else

'' -------------------------------------------------------------------------
'' Native Windows and Unix socket implementation
'' -------------------------------------------------------------------------

#ifdef __FB_WIN32__
#include once "win/winsock2.bi"
#else
#include once "crt/stdio.bi"
#include once "crt/netdb.bi"
#include once "crt/sys/socket.bi"
#include once "crt/netinet/in.bi"
#include once "crt/arpa/inet.bi"
#include once "crt/unistd.bi"
#endif

const RECVBUFFLEN = 8192
const NEWLINE = !"\r\n"
const DEFAULT_HOST = "www.freebasic.net"

declare sub doInit( )
declare sub doShutdown( )
declare sub getHostAndPath _
	( _
		byref src as string, _
		byref hostname as string, _
		byref path as string _
	)
declare function resolveHost( byref hostname as string ) as integer
declare sub reportError( byref msg as string )

	dim hostname as string
	dim path as string

	doInit( )

	'' check command-line
	getHostAndPath( command( ), hostname, path )
	if( len( hostname ) = 0 ) then
		hostname = DEFAULT_HOST
	end if

	'' resolve name
	dim ip as integer
	dim s as SOCKET

	ip = resolveHost( hostname )
	if( ip = 0 ) then
		print "resolveHost(): invalid address"
		end 1
	end if

	'' open socket
	s = opensocket( AF_INET, SOCK_STREAM, IPPROTO_TCP )
	if( s = Cast(SOCKET, SOCKET_ERROR) ) then
		reportError( "socket()" )
		end 1
	end if

	'' connect to host
	dim sa as sockaddr_in
#if defined(__FB_DARWIN__) or defined(__FB_DRAGONFLY__) or defined(__FB_OPENBSD__) or defined(__FB_NETBSD__)
	sa.sin_len         = sizeof( sa )
#endif
	'' HTTP's standard TCP service port.
	const HTTP_PORT = 80
	sa.sin_port        = htons( HTTP_PORT )
	sa.sin_family      = AF_INET
	sa.sin_addr.S_addr = ip

	if( connect( s, cast( PSOCKADDR, @sa ), sizeof( sa ) ) = SOCKET_ERROR ) then
		reportError( "connect()" )
		closesocket( s )
		end 1
	end if

	'' send HTTP request
	dim sendbuffer as string
	sendBuffer = "GET /" + path + " HTTP/1.0" + NEWLINE + _
	             "Host: " + hostname + NEWLINE + _
	             "Connection: close" + NEWLINE + _
	             "User-Agent: GetHTTP 0.0" + NEWLINE + _
	             NEWLINE

	if( send( s, sendBuffer, len( sendBuffer ), 0 ) = SOCKET_ERROR ) then
		reportError( "send()" )
		closesocket( s )
		end 1
	end if

	'' receive until connection is closed
	dim recvbuffer as zstring * RECVBUFFLEN+1
	dim bytes as integer

	do
		bytes = recv( s, recvBuffer, RECVBUFFLEN, 0 )
		if( bytes <= 0 ) then
			exit do
		end if

		'' add the null-terminator
		recvbuffer[bytes] = 0

		'' print buffer as a string
		print recvbuffer
	loop

	print

	'' close socket
#ifdef __FB_WIN32__
	shutdown( s, SD_BOTH )
#else
	shutdown( s, SHUT_RDWR )
#endif
	closesocket( s )

	doShutdown( )

sub getHostAndPath _
	( _
		byref src as string, _
		byref hostname as string, _
		byref path as string _
	)

	dim p as integer = instr( src, " " )
	if( p = 0 or p = len( src ) ) then
		hostname = trim( src )
		path = ""
	else
		hostname = trim( left( src, p-1 ) )
		path = trim( mid( src, p+1 ) )
	end if

end sub

function resolveHost( byref hostname as string ) as integer

	dim ia as in_addr
	dim hostentry as hostent ptr

	'' check if it's an ip address
	ia.S_addr = inet_addr( hostname )
	if ( ia.S_addr = INADDR_NONE ) then

		'' if not, assume it's a name, resolve it
		hostentry = gethostbyname( hostname )
		if ( hostentry = 0 ) then
			exit function
		end if

		function = *cast( integer ptr, *hostentry->h_addr_list )
	else
		'' just return the address
		function = ia.S_addr
	end if

end function

sub reportError( byref msg as string )
#ifdef __FB_WIN32__
	print msg; ": error #" & WSAGetLastError( )
#else
	perror( msg )
#endif
end sub

sub doInit( )
#ifdef __FB_WIN32__
	'' init winsock
	dim wsaData as WSAData
	if( WSAStartup( MAKEWORD( 1, 1 ), @wsaData ) <> 0 ) then
		print "Error: WSAStartup failed"
		end 1
	end if
#endif
end sub

sub doShutdown( )
#ifdef __FB_WIN32__
	'' quit winsock
	WSACleanup( )
#endif
end sub

#endif

'' end of http-get.bas
