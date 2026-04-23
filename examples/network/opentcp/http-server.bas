''
'' minimal http server example using OPEN TCP SERVER
''

#if defined(__FB_DOS__) or defined(__FB_JS__) or defined(__FB_XBOX__)
	print "TCP is not supported on this target"
	end 1
#endif

const DEFAULT_HOST = "127.0.0.1"
const DEFAULT_PORT = "8087"
const DEFAULT_RESOURCE = "/dummy.html"

declare function hGetArg( byval index as integer, byref default_value as string ) as string

dim host as string = hGetArg( 1, DEFAULT_HOST )
dim port as string = hGetArg( 2, DEFAULT_PORT )
dim server as integer
dim client as integer
dim request_line as string
dim header_line as string
dim response_body as string
dim response_size as string
dim request_lcase as string

server = freefile()

if( OPEN TCP SERVER( "host=" & host & ",port=" & port & ",backlog=1" AS #server ) <> 0 ) then
	print "OPEN TCP SERVER failed, ERR="; err
	end 1
end if

print "Listening on http://" & host & ":" & port & DEFAULT_RESOURCE

client = TCP ACCEPT( #server )
if( client = 0 ) then
	print "TCP ACCEPT failed, ERR="; err
	close #server
	end 1
end if

line input #client, request_line
request_lcase = lcase( request_line )

do
	line input #client, header_line
loop while( len( header_line ) <> 0 )

if( left( request_lcase, 4 ) = "get " andalso _
    instr( request_lcase, lcase( DEFAULT_RESOURCE ) & " " ) > 0 ) then
	response_body = "<html><body><h1>FreeBASIC TCP server</h1><p>dummy html file</p></body></html>"
	print #client, "HTTP/1.0 200 OK"
else
	response_body = "<html><body><h1>404 Not Found</h1></body></html>"
	print #client, "HTTP/1.0 404 Not Found"
end if

response_size = ltrim( str( len( response_body ) ) )

print #client, "Content-Type: text/html"
print #client, "Content-Length: " & response_size
print #client, "Connection: close"
print #client,
put #client, , response_body

close #client
close #server

function hGetArg( byval index as integer, byref default_value as string ) as string
	if( len( command( index ) ) <> 0 ) then
		function = command( index )
	else
		function = default_value
	end if
end function
