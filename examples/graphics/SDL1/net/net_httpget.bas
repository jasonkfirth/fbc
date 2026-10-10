'' Project: FreeBASIC SDL examples
'' File: net_httpget.bas
'' Purpose:
''     Fetch an HTTP response using SDL1_net.
'' Responsibilities:
''     Parse a host and optional port, send a request, and close TCP.
'' This file intentionally does NOT contain:
''     HTTPS or an HTML parser.
''
''
'' simple http get example using the SDL_net library
''

#include once "SDL/SDL_net.bi"

const RECVBUFFLEN = 8192
const NEWLINE = !"\r\n"
const DEFAULT_HOST = "www.freebasic.net"

declare sub gethostandpath( byref src as string, byref hostname as string, byref path as string )


	'' globals
	dim hostname as string
	dim path as string

	gethostandpath command, hostname, path

	if( len( hostname ) = 0 ) then
		hostname = DEFAULT_HOST
	end if

	'' Optional host:port supports HTTP servers away from the standard port.
	'' SDL1_net resolves IPv4 hosts, so the colon is a port separator here.
	dim hostheader as string = hostname
	dim port as Uint16 = 80
	dim separator as integer = instr(hostname, ":")
	if separator <> 0 then
		dim porttext as string = mid(hostname, separator + 1)
		if separator = 1 or len(porttext) = 0 or len(porttext) > 5 then
			print "Error: expected hostname[:port] [path]"
			end 1
		end if
		for i as integer = 0 to len(porttext) - 1
			if porttext[i] < asc("0") or porttext[i] > asc("9") then
				print "Error: invalid HTTP port"
				end 1
			end if
		next
		dim value as integer = valint(porttext)
		if value < 1 or value > 65535 then
			print "Error: HTTP port must be between 1 and 65535"
			end 1
		end if
		port = value
		hostname = left(hostname, separator - 1)
	end if
	if instr(hostheader, chr(13)) or instr(hostheader, chr(10)) or _
	   instr(path, chr(13)) or instr(path, chr(10)) then
		print "Error: HTTP arguments must not contain line breaks"
		end 1
	end if
	if left(path, 1) = "/" then path = mid(path, 2)

	'' init
	if( SDLNet_Init <> 0 ) then
		print "Error: SDLNet_Init failed"
		end 1
	end if

	'' resolve
	dim ip as IPAddress
    dim socket as TCPSocket

    if( SDLNet_ResolveHost( @ip, hostname, port ) <> 0 ) then
		print "Error: SDLNet_ResolveHost failed"
		SDLNet_Quit
		end 1
	end if

    '' open
    socket = SDLNet_TCP_Open( @ip )
    if( socket = 0 ) then
		print "Error: SDLNet_TCP_Open failed"
		SDLNet_Quit
		end 1
	end if

    '' send HTTP request
    dim sendbuffer as string

	sendBuffer = "GET /" + path + " HTTP/1.0" + NEWLINE + _
				 "Host: " + hostheader + NEWLINE + _
				 "Connection: close" + NEWLINE + _
				 "User-Agent: GetHTTP 0.0" + NEWLINE + _
				 NEWLINE

    if( SDLNet_TCP_Send( socket, strptr( sendbuffer ), len( sendbuffer ) ) < len( sendbuffer ) ) then
		print "Error: SDLNet_TCP_Send failed"
		SDLNet_TCP_Close( socket )
		SDLNet_Quit
		end 1
	end if

    '' receive til connection is closed
    dim recvbuffer as zstring * RECVBUFFLEN+1
    dim bytes as long

    do
        bytes = SDLNet_TCP_Recv( socket, strptr( recvbuffer ), RECVBUFFLEN )
        if( bytes < 0 ) then
            print "Error: SDLNet_TCP_Recv failed"
            SDLNet_TCP_Close( socket )
            SDLNet_Quit
            end 1
        elseif( bytes = 0 ) then
            exit do
        end if

        '' add the null-terminator
        recvbuffer[bytes] = 0

        '' print it as string
        print recvbuffer;
    loop
    print

	'' close socket
	SDLNet_TCP_Close( socket )

	'' quit
	SDLNet_Quit

'':::::
sub gethostandpath( byref src as string, byref hostname as string, byref path as string )
	dim p as integer

	p = instr( src, " " )
	if( p = 0 or p = len( src ) ) then
		hostname = trim( src )
		path = ""
	else
		hostname = trim( left( src, p-1 ) )
		path = trim( mid( src, p+1 ) )
	end if

end sub

'' End of net_httpget.bas
