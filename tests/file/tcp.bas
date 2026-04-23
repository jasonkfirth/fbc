' TEST_MODE : COMPILE_AND_RUN_OK

#if defined(__FB_DOS__) or defined(__FB_JS__) or defined(__FB_XBOX__)
	end 0
#endif

const TEST_PORT = 19091

dim shared as integer server_ready
dim shared as integer server_open_ok
dim shared as integer server_done
dim shared as integer server_error
dim shared as integer server_step
dim shared as integer client_done
dim shared as integer client_error
dim shared as integer client_step
dim shared as integer client_request_ready
dim shared as integer server_reply_ready
dim shared as integer saw_server_eof
dim shared as integer saw_client_eof
dim shared as integer saw_client_eoc
dim shared as integer client_received_reply
dim shared as integer server_write_num
dim shared as integer client_write_num
dim shared as zstring * 64 server_line
dim shared as zstring * 64 client_line
dim shared as zstring * 64 server_write_text
dim shared as zstring * 64 client_write_text
dim shared as ubyte client_put_recv(0 to 3)
dim shared as ubyte server_put_recv(0 to 3)

sub server_thread( byval userdata as any ptr )
	dim as integer server
	dim as integer client
	dim as string s
	dim as integer i
	dim as integer tries
	dim as ubyte payload(0 to 3)

	server = freefile()
	server_step = 1

	if( OPEN TCP SERVER( "host=127.0.0.1,port=" & str( TEST_PORT ) & ",backlog=1" AS #server ) <> 0 ) then
		server_error = 1
		server_ready = TRUE
		exit sub
	end if

	server_open_ok = TRUE
	saw_server_eof = eof( server )
	server_ready = TRUE
	server_step = 2

	client = TCP ACCEPT( #server )
	if( client = 0 ) then
		server_error = 2
		close #server
		exit sub
	end if

	server_step = 3
	tries = 0
	do while( (client_request_ready = FALSE) andalso (tries < 5000) )
		sleep 1, 1
		tries += 1
	loop
	if( client_request_ready = FALSE ) then
		server_error = 2
		close #client
		close #server
		exit sub
	end if

	server_step = 4
	line input #client, s
	server_line = s

	server_step = 5
	input #client, server_write_num, s
	server_write_text = s

	server_step = 6
	get #client, , client_put_recv()

	server_step = 7
	print #client, "server-print"
	write #client, 456, "server-write"

	for i = 0 to 3
		payload(i) = 10 + i
	next
	put #client, , payload()
	server_reply_ready = TRUE

	server_step = 8
	tries = 0
	do while( (client_received_reply = FALSE) andalso (tries < 5000) )
		sleep 1, 1
		tries += 1
	loop
	if( client_received_reply = FALSE ) then
		server_error = 3
	end if

	close #client
	close #server
	server_step = 9
	server_done = TRUE
end sub

sub client_thread( byval userdata as any ptr )
	dim as integer client
	dim as string s
	dim as integer i
	dim as integer tries
	dim as ubyte payload(0 to 3)

	do while( server_ready = FALSE )
		sleep 1, 1
	loop

	client_step = 1
	if( server_open_ok = FALSE ) then
		client_error = 1
		exit sub
	end if

	client = freefile()
	if( OPEN TCP( "host=127.0.0.1,port=" & str( TEST_PORT ) AS #client ) <> 0 ) then
		client_error = 2
		exit sub
	end if

	client_step = 2
	print #client, "client-print"
	write #client, 123, "client-write"

	for i = 0 to 3
		payload(i) = i + 1
	next
	put #client, , payload()
	client_request_ready = TRUE

	client_step = 3
	tries = 0
	do while( (server_reply_ready = FALSE) andalso (tries < 5000) )
		sleep 1, 1
		tries += 1
	loop
	if( server_reply_ready = FALSE ) then
		client_error = 3
		close #client
		exit sub
	end if

	client_step = 4
	do while( eof( client ) andalso (eoc( client ) = 0) )
		saw_client_eof = TRUE
		sleep 1, 1
	loop

	if( eoc( client ) <> 0 ) then
		client_error = 4
		close #client
		exit sub
	end if

	client_step = 5
	line input #client, s
	client_line = s

	client_step = 6
	input #client, client_write_num, s
	client_write_text = s

	client_step = 7
	get #client, , server_put_recv()
	client_received_reply = TRUE

	client_step = 8
	tries = 0
	do while( (eoc( client ) = 0) andalso (tries < 2000) )
		sleep 1, 1
		tries += 1
	loop
	saw_client_eoc = (eoc( client ) <> 0)
	if( saw_client_eoc = FALSE ) then
		client_error = 5
	end if

	close #client
	client_step = 9
	client_done = TRUE
end sub

scope
	dim as any ptr server_id
	dim as any ptr client_id
	dim as integer tries

	server_id = threadcreate( @server_thread )
	client_id = threadcreate( @client_thread )

	tries = 0
	do while( (server_done = FALSE or client_done = FALSE) andalso _
	          (server_error = 0) andalso (client_error = 0) andalso _
	          (tries < 20000) )
		sleep 1, 1
		tries += 1
	loop

	if( server_done = FALSE or client_done = FALSE ) then
		print "timeout server_step="; server_step; " client_step="; client_step; _
		      " server_error="; server_error; " client_error="; client_error
		end 1
	end if

	threadwait( server_id )
	threadwait( client_id )
end scope

ASSERT( server_error = 0 )
ASSERT( client_error = 0 )
ASSERT( saw_server_eof )
ASSERT( saw_client_eof )
ASSERT( saw_client_eoc )

ASSERT( server_line = "client-print" )
ASSERT( client_line = "server-print" )

ASSERT( server_write_num = 123 )
ASSERT( server_write_text = "client-write" )
ASSERT( client_write_num = 456 )
ASSERT( client_write_text = "server-write" )

ASSERT( client_put_recv(0) = 1 )
ASSERT( client_put_recv(1) = 2 )
ASSERT( client_put_recv(2) = 3 )
ASSERT( client_put_recv(3) = 4 )

ASSERT( server_put_recv(0) = 10 )
ASSERT( server_put_recv(1) = 11 )
ASSERT( server_put_recv(2) = 12 )
ASSERT( server_put_recv(3) = 13 )
