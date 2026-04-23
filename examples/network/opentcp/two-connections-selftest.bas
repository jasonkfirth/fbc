''
'' Self-contained OPEN TCP demo:
'' - opens a listening socket
'' - opens two client connections to that listener
'' - accepts both connections
'' - sends a different line to each client
'' - verifies the clients received the expected text
''
'' This is intentionally written as a single-file example so beginners can
'' see both sides of the conversation in one place:
'' - the server side uses OPEN TCP SERVER and TCP ACCEPT
'' - the client side uses OPEN TCP and LINE INPUT
'' The host defaults to 127.0.0.1, which means "this same machine".
''

#if defined(__FB_DOS__) or defined(__FB_JS__) or defined(__FB_XBOX__)
	print "TCP is not supported on this target"
	end 1
#endif

const DEFAULT_HOST = "127.0.0.1"
const DEFAULT_PORT = "31876"
const FIRST_MESSAGE = "first connection: hello from the OPEN TCP server"
const SECOND_MESSAGE = "second connection: this is a different message"

''
'' Small helper declarations:
'' - hGetArg() lets us override the default host/port from the command line
'' - hRequire() stops the program with a readable error if a check fails
''
declare function hGetArg( byval index as integer, byref default_value as string ) as string
declare sub hRequire( byval ok as integer, byref failure_text as string )

''
'' We keep one file number for each open connection.
'' In FreeBASIC, sockets opened with OPEN TCP are used through file handles,
'' so PRINT #, LINE INPUT #, and CLOSE # all work the same way they do for
'' normal files.
''
dim as string host = hGetArg( 1, DEFAULT_HOST )
dim as string port = hGetArg( 2, DEFAULT_PORT )
dim as integer server_file
dim as integer client_file_1
dim as integer client_file_2
dim as integer accepted_file_1
dim as integer accepted_file_2
dim as string received_1
dim as string received_2

print "OPEN TCP two-connection self-test"
print "Host:"; host
print "Port:"; port

''
'' Step 1:
'' Create the listening socket. backlog=2 tells the server to queue up to
'' two pending connections, which matches this example.
''
server_file = freefile()
if( OPEN TCP SERVER( "host=" & host & ",port=" & port & ",backlog=2" AS #server_file ) <> 0 ) then
	print "OPEN TCP SERVER failed, ERR="; err
	end 1
end if

print "Server socket is listening."

''
'' Step 2:
'' Open the first client connection. Because the host is 127.0.0.1, this
'' client connects right back into the server we just opened above.
''
client_file_1 = freefile()
if( OPEN TCP( "host=" & host & ",port=" & port AS #client_file_1 ) <> 0 ) then
	print "OPEN TCP client 1 failed, ERR="; err
	close #server_file
	end 1
end if

''
'' Step 3:
'' Accept the first queued incoming connection from the listening socket.
'' TCP ACCEPT returns another file number representing the connected client
'' on the server side.
''
accepted_file_1 = TCP ACCEPT( #server_file )
hRequire( accepted_file_1 <> 0, "TCP ACCEPT for client 1 failed, ERR=" & str( err ) )

''
'' Step 4:
'' Send one text line from the server side, then read it from the client
'' side. PRINT # writes a line ending, so LINE INPUT # is a convenient way
'' to read it back.
''
print #accepted_file_1, FIRST_MESSAGE
line input #client_file_1, received_1
close #accepted_file_1
print "Client 1 received:"; received_1
hRequire( received_1 = FIRST_MESSAGE, "client 1 received the wrong message" )

''
'' Step 5:
'' Repeat the same pattern for a second client so the example shows that the
'' listening socket can accept more than one connection.
''
client_file_2 = freefile()
if( OPEN TCP( "host=" & host & ",port=" & port AS #client_file_2 ) <> 0 ) then
	print "OPEN TCP client 2 failed, ERR="; err
	close #client_file_1
	close #server_file
	end 1
end if

accepted_file_2 = TCP ACCEPT( #server_file )
hRequire( accepted_file_2 <> 0, "TCP ACCEPT for client 2 failed, ERR=" & str( err ) )

''
'' This time we deliberately send a different message so we can prove the
'' program is handling two distinct accepted connections, not just echoing
'' the same text twice.
''
print #accepted_file_2, SECOND_MESSAGE
line input #client_file_2, received_2
close #accepted_file_2
print "Client 2 received:"; received_2
hRequire( received_2 = SECOND_MESSAGE, "client 2 received the wrong message" )

''
'' Always close the client and server handles when finished.
'' That releases the sockets cleanly.
''
close #client_file_1
close #client_file_2
close #server_file

print
print "OPEN TCP self-test passed."
print "Two separate localhost connections were accepted and verified."

sub hRequire( byval ok as integer, byref failure_text as string )
	'' A tiny assertion helper keeps the main demo easier to read.
	if( ok = 0 ) then
		print "FAIL:"; failure_text
		end 1
	end if
end sub

function hGetArg( byval index as integer, byref default_value as string ) as string
	'' command(n) returns the nth command-line argument as a string.
	'' If the user did not supply one, fall back to the default.
	if( len( command( index ) ) <> 0 ) then
		function = command( index )
	else
		function = default_value
	end if
end function
