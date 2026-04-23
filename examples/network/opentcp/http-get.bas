''
'' simple http-get example using OPEN TCP
''

#if defined(__FB_DOS__) or defined(__FB_JS__) or defined(__FB_XBOX__)
	print "TCP is not supported on this target"
	end 1
#endif

const DEFAULT_HOST = "192.168.250.99"
const DEFAULT_PORT = "80"
const DEFAULT_PATH = "/"

declare function hGetArg( byval index as integer, byref default_value as string ) as string

dim host as string = hGetArg( 1, DEFAULT_HOST )
dim port as string = hGetArg( 2, DEFAULT_PORT )
dim path as string = hGetArg( 3, DEFAULT_PATH )
dim f as integer
dim line_ as string
dim chunk as string
dim saw_data as integer = FALSE
dim idle_ticks as integer = 0
dim headers_done as integer = FALSE
dim content_length as integer = -1
dim body_received as integer = 0

f = freefile()

if( OPEN TCP( "host=" & host & ",port=" & port AS #f ) <> 0 ) then
	print "OPEN TCP failed, ERR="; err
	end 1
end if

print #f, "GET " & path & " HTTP/1.0"
print #f, "Host: " & host
print #f, "Connection: close"
print #f, "User-Agent: FreeBASIC OPEN TCP example"
print #f,

do while( headers_done = FALSE )
	if( eof( f ) = 0 ) then
		line input #f, line_
		print line_
		saw_data = TRUE
		idle_ticks = 0

		if( len( line_ ) = 0 ) then
			headers_done = TRUE
			print
		elseif( lcase( left( line_, 15 ) ) = "content-length:" ) then
			content_length = val( trim( mid( line_, 16 ) ) )
		end if
	elseif( eoc( f ) <> 0 ) then
		exit do
	else
		sleep 10, 1
		idle_ticks += 1
		if( idle_ticks >= 1000 ) then
			print "Timed out waiting for HTTP response headers"
			close #f
			end 1
		end if
	end if
loop

if( content_length > 0 ) then
	do while( body_received < content_length )
		if( eof( f ) = 0 ) then
			dim want as integer = content_length - body_received
			if( want > 256 ) then
				want = 256
			end if

			chunk = input( want, #f )
			if( len( chunk ) <> 0 ) then
				print chunk;
				saw_data = TRUE
				body_received += len( chunk )
				idle_ticks = 0
			end if
		elseif( eoc( f ) <> 0 ) then
			exit do
		else
			sleep 10, 1
			idle_ticks += 1
			if( idle_ticks >= 1000 ) then
				print
				print "Timed out waiting for HTTP response body"
				close #f
				end 1
			end if
		end if
	loop
else
	do
		if( eof( f ) = 0 ) then
			chunk = input( 4096, #f )
			if( len( chunk ) <> 0 ) then
				print chunk;
				saw_data = TRUE
				idle_ticks = 0
			end if
		elseif( eoc( f ) <> 0 ) then
			exit do
		else
			sleep 10, 1
			idle_ticks += 1
			if( idle_ticks >= 1000 ) then
				print
				print "Timed out waiting for HTTP response data"
				close #f
				end 1
			end if
		end if
	loop
end if

close #f

if( saw_data = FALSE ) then
	print "No HTTP response received"
	end 1
end if

if( content_length > 0 andalso body_received <> content_length ) then
	print
	print "Incomplete HTTP response body"
	end 1
end if

function hGetArg( byval index as integer, byref default_value as string ) as string
	if( len( command( index ) ) <> 0 ) then
		function = command( index )
	else
		function = default_value
	end if
end function
