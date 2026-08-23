' Project: FreeBASIC TCP runtime tests
' ------------------------------------
'
' File: tests/file/tcp.bas
'
' Purpose:
'
'     Exercise TCP file I/O, connection state, and loopback transfers.
'
' Responsibilities:
'
'     - verify client and server text, binary, EOF, and EOC behavior
'     - verify a sustained byte-at-a-time loopback transfer
'     - verify wildcard listener behavior with an IPv4 client
'
' This file intentionally does NOT contain:
'
'     - build metadata or multithreaded link selection, which remain in tcp.bmk
'     - external network dependencies
'
' Thread synchronization and ownership:
'
'     Worker threads own and close the file handles that they open or accept.
'     The controlling scope waits for both workers before closing each listener.
'     Shared integer flags provide monotonic progress and failure reporting for
'     this bounded loopback test.

#if defined(__FB_DOS__) or defined(__FB_JS__) or defined(__FB_XBOX__)
	end 0
#else

const TEST_PORT = 19091
const BURST_PORT = TEST_PORT + 1
const WILDCARD_PORT = TEST_PORT + 2

' Keep enough one-byte runtime calls to exercise sustained socket traffic
' without turning the correctness test into a performance test under QEMU.
const BURST_BYTES = 16384
const STREAM_WAIT_ITERATIONS = 5000

' The burst performs one runtime call per byte. Allow two minutes of requested
' millisecond sleeps so slower kernels and emulators do not create a false
' timeout while retaining a finite bound for deadlocks.
const BURST_WAIT_ITERATIONS = 120000

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
dim shared as integer burst_listener
dim shared as integer burst_server_done
dim shared as integer burst_server_error
dim shared as integer burst_client_done
dim shared as integer burst_client_error
dim shared as integer burst_server_step
dim shared as integer burst_client_step
dim shared as integer burst_stop

' TCP EOF reports whether bytes are available immediately; it can therefore
' be true while a live connection is waiting for its next packet.  EOC is the
' distinct peer-closure test.  Poll both with a finite bound before each read
' so scheduler timing cannot turn an otherwise valid exchange into empty data.
function wait_for_tcp_data( byval handle as integer ) as integer
	dim as integer tries = 0

	do while( eof( handle ) <> 0 )
		if( eoc( handle ) <> 0 ) then
			return FALSE
		end if

		if( tries >= STREAM_WAIT_ITERATIONS ) then
			return FALSE
		end if

		sleep 1, 1
		tries += 1
	loop

	return TRUE
end function

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
	' The flag confirms every client write has returned.  The EOF poll below
	' separately confirms that the network stack has made those bytes readable.
	tries = 0
	do while( (client_request_ready = FALSE) andalso _
	          (tries < STREAM_WAIT_ITERATIONS) )
		sleep 1, 1
		tries += 1
	loop
	if( client_request_ready = FALSE ) then
		server_error = 2
		close #client
		close #server
		exit sub
	end if

	if( wait_for_tcp_data( client ) = FALSE ) then
		server_error = 2
		close #client
		close #server
		exit sub
	end if

	server_step = 4
	line input #client, s
	server_line = s

	server_step = 5
	if( wait_for_tcp_data( client ) = FALSE ) then
		server_error = 4
		close #client
		close #server
		exit sub
	end if
	input #client, server_write_num, s
	server_write_text = s

	server_step = 6
	if( wait_for_tcp_data( client ) = FALSE ) then
		server_error = 5
		close #client
		close #server
		exit sub
	end if
	get #client, , client_put_recv()

	server_step = 7
	server_reply_ready = TRUE

	tries = 0
	do while( (saw_client_eof = FALSE) andalso (tries < 2000) )
		sleep 1, 1
		tries += 1
	loop

	print #client, "server-print"
	write #client, 456, "server-write"

	for i = 0 to 3
		payload(i) = 10 + i
	next
	put #client, , payload()

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
	tries = 0
	do
		if( OPEN TCP( "host=127.0.0.1,port=" & str( TEST_PORT ) AS #client ) = 0 ) then
			exit do
		end if

		' Some emulated architectures can briefly refuse a connection even after
		' the listening thread has completed OPEN TCP SERVER.  Use the same
		' bounded retry policy as the burst-transfer test below.
		tries += 1
		if( tries >= 5000 ) then
			client_error = 2
			exit sub
		end if

		sleep 1, 1
	loop

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
	if( wait_for_tcp_data( client ) = FALSE ) then
		client_error = 6
		close #client
		exit sub
	end if
	input #client, client_write_num, s
	client_write_text = s

	client_step = 7
	if( wait_for_tcp_data( client ) = FALSE ) then
		client_error = 7
		close #client
		exit sub
	end if
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

sub burst_server_thread( byval userdata as any ptr )
	dim as integer client
	dim as integer i
	dim as ubyte value

	burst_server_step = 1
	do
		if( burst_stop <> FALSE ) then
			exit sub
		end if
		client = TCP ACCEPT( #burst_listener )
		if( client > 0 ) then
			exit do
		end if
		sleep 1, 1
	loop

	burst_server_step = 2
	do while( eof( client ) <> 0 )
		if( burst_stop <> FALSE ) then
			close #client
			exit sub
		end if
		if( eoc( client ) <> 0 ) then
			burst_server_error = 1
			close #client
			exit sub
		end if
		sleep 1, 1
	loop

	if( get( #client, , value ) <> 0 ) then
		burst_server_error = 2
		close #client
		exit sub
	end if

	burst_server_step = 3
	for i = 0 to BURST_BYTES - 1
		if( burst_stop <> FALSE ) then
			close #client
			exit sub
		end if
		value = i and 255
		if( put( #client, , value ) <> 0 ) then
			burst_server_error = 3
			close #client
			exit sub
		end if
	next

	close #client
	burst_server_step = 4
	burst_server_done = TRUE
end sub

sub burst_client_thread( byval userdata as any ptr )
	dim as integer client
	dim as integer i
	dim as integer tries
	dim as ubyte value
	dim as ubyte expected

	burst_client_step = 1
	client = freefile()
	tries = 0
	do
		if( OPEN TCP( "host=127.0.0.1,port=" & str( BURST_PORT ) AS #client ) = 0 ) then
			exit do
		end if

		tries += 1
		if( tries >= 5000 ) then
			burst_client_error = 1
			exit sub
		end if

		sleep 1, 1
	loop

	value = 101
	if( put( #client, , value ) <> 0 ) then
		burst_client_error = 2
		close #client
		exit sub
	end if

	burst_client_step = 2
	for i = 0 to BURST_BYTES - 1
		do while( eof( client ) <> 0 )
			if( burst_stop <> FALSE ) then
				close #client
				exit sub
			end if
			if( eoc( client ) <> 0 ) then
				burst_client_error = 3
				close #client
				exit sub
			end if
			sleep 1, 1
		loop

		if( get( #client, , value ) <> 0 ) then
			burst_client_error = 4
			close #client
			exit sub
		end if

		expected = i and 255
		if( value <> expected ) then
			burst_client_error = 5
			close #client
			exit sub
		end if
	next

	close #client
	burst_client_step = 3
	burst_client_done = TRUE
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

scope
	dim as any ptr server_id
	dim as any ptr client_id
	dim as integer tries

	burst_listener = freefile()
	burst_stop = FALSE
	if( OPEN TCP SERVER( "host=127.0.0.1,port=" & str( BURST_PORT ) & ",backlog=1,timeout=1" AS #burst_listener ) <> 0 ) then
		print "burst server open failed"
		end 1
	end if

	server_id = threadcreate( @burst_server_thread )
	client_id = threadcreate( @burst_client_thread )

	tries = 0
	do while( (burst_server_done = FALSE or burst_client_done = FALSE) andalso _
	          (burst_server_error = 0) andalso (burst_client_error = 0) andalso _
	          (tries < BURST_WAIT_ITERATIONS) )
		sleep 1, 1
		tries += 1
	loop

	if( burst_server_done = FALSE or burst_client_done = FALSE ) then
		burst_stop = TRUE
		threadwait( server_id )
		threadwait( client_id )
		close #burst_listener
		print "burst timeout server_step="; burst_server_step; " client_step="; burst_client_step; _
		      " server_error="; burst_server_error; " client_error="; burst_client_error
		end 1
	end if

	close #burst_listener
	threadwait( server_id )
	threadwait( client_id )
end scope

ASSERT( burst_server_error = 0 )
ASSERT( burst_client_error = 0 )

scope
	dim as integer server
	dim as integer client

	server = freefile()
	if( OPEN TCP SERVER( "port=" & str( WILDCARD_PORT ) & ",backlog=1" AS #server ) <> 0 ) then
		print "wildcard server open failed"
		end 1
	end if

	client = freefile()
	if( OPEN TCP( "host=127.0.0.1,port=" & str( WILDCARD_PORT ) AS #client ) <> 0 ) then
		print "wildcard IPv4 client open failed"
		close #server
		end 1
	end if

	close #client
	close #server
end scope

#endif

' end of tests/file/tcp.bas
