' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC runtime device protocol parser tests
'' ----------------------------------------------
''
'' File: device-protocols.bas
''
'' Purpose:
''
''     Verify the hardware-free parsers behind OPEN COM, OPEN LPT,
''     OPEN TCP, and OPEN TCP SERVER.
''
'' Responsibilities:
''
''     - exercise accepted COM, LPT, and TCP protocol strings
''     - reject malformed and overflowing numeric fields
''     - avoid touching real serial ports, printers, or sockets
''
'' This file intentionally does NOT contain:
''
''     - device open/write/read tests
''     - platform driver validation
''     - network connection attempts
''

#include once "crt.bi"

type DEV_LPT_PROTOCOL
	proto as zstring ptr
	iPort as long
	name as zstring ptr
	title as zstring ptr
	emu as zstring ptr
end type

type DEV_TCP_PROTOCOL
	host as zstring ptr
	port as ulong
	timeout as ulong
	backlog as ulong
	is_server as long
end type

declare function fb_DevComTestProtocolEx cdecl alias "fb_DevComTestProtocolEx" _
	( _
		byval handle as any ptr, _
		byval filename as zstring ptr, _
		byval filename_len as size_t, _
		byval pPort as size_t ptr _
	) as long

declare function fb_DevLptParseProtocol cdecl alias "fb_DevLptParseProtocol" _
	( _
		byval lpt_proto_out as DEV_LPT_PROTOCOL ptr ptr, _
		byval proto_raw as zstring ptr, _
		byval proto_raw_len as size_t, _
		byval subst_prn as long _
	) as long

declare function fb_DevLptTestProtocol cdecl alias "fb_DevLptTestProtocol" _
	( _
		byval handle as any ptr, _
		byval filename as zstring ptr, _
		byval filename_len as size_t _
	) as long

declare function fb_DevTcpParseProtocol cdecl alias "fb_DevTcpParseProtocol" _
	( _
		byval tcp_proto_out as DEV_TCP_PROTOCOL ptr ptr, _
		byval proto_raw as zstring ptr, _
		byval proto_raw_len as size_t, _
		byval is_server as long _
	) as long

function com_accepts( byref text as string, byval expected_port as size_t ) as long
	dim as size_t port = 999
	dim as long ok

	ok = fb_DevComTestProtocolEx( 0, strptr( text ), len( text ), @port )
	if( ok = 0 ) then
		return 0
	end if

	return ( port = expected_port )
end function

function com_rejects( byref text as string ) as long
	dim as size_t port = 999

	return ( fb_DevComTestProtocolEx( 0, strptr( text ), len( text ), @port ) = 0 )
end function

function lpt_parse _
	( _
		byref text as string, _
		byval subst_prn as long, _
		byval expected_port as long, _
		byref expected_name as string, _
		byref expected_title as string, _
		byref expected_emu as string _
	) as long

	dim as DEV_LPT_PROTOCOL ptr proto = 0
	dim as long ok

	ok = fb_DevLptParseProtocol( @proto, strptr( text ), len( text ), subst_prn )
	if( ok = 0 orelse proto = 0 ) then
		if( proto <> 0 ) then
			free( proto )
		end if
		return 0
	end if

	ok = ( proto->iPort = expected_port )

	if( expected_name <> "" ) then
		ok = ok andalso ( *proto->name = expected_name )
	end if

	if( expected_title <> "" ) then
		ok = ok andalso ( *proto->title = expected_title )
	end if

	if( expected_emu <> "" ) then
		ok = ok andalso ( *proto->emu = expected_emu )
	end if

	free( proto )
	return ok
end function

function lpt_rejects( byref text as string ) as long
	dim as DEV_LPT_PROTOCOL ptr proto = 0
	dim as long ok

	ok = fb_DevLptParseProtocol( @proto, strptr( text ), len( text ), 0 )
	if( proto <> 0 ) then
		free( proto )
	end if

	return ( ok = 0 )
end function

function tcp_parse _
	( _
		byref text as string, _
		byval is_server as long, _
		byref expected_host as string, _
		byval expected_port as ulong, _
		byval expected_timeout as ulong, _
		byval expected_backlog as ulong _
	) as long

	dim as DEV_TCP_PROTOCOL ptr proto = 0
	dim as long ok

	ok = fb_DevTcpParseProtocol( @proto, strptr( text ), len( text ), is_server )
	if( ok = 0 orelse proto = 0 ) then
		if( proto <> 0 ) then
			free( proto )
		end if
		return 0
	end if

	ok = ( proto->port = expected_port ) andalso _
	     ( proto->timeout = expected_timeout ) andalso _
	     ( proto->backlog = expected_backlog ) andalso _
	     ( proto->is_server = is_server )

	if( expected_host <> "" ) then
		ok = ok andalso ( *proto->host = expected_host )
	end if

	free( proto )
	return ok
end function

function tcp_rejects( byref text as string, byval is_server as long ) as long
	dim as DEV_TCP_PROTOCOL ptr proto = 0
	dim as long ok

	ok = fb_DevTcpParseProtocol( @proto, strptr( text ), len( text ), is_server )
	if( proto <> 0 ) then
		free( proto )
	end if

	return ( ok = 0 )
end function

assert( com_accepts( "SER:", 1 ) )
assert( com_accepts( "COM:", 0 ) )
assert( com_accepts( "COM1:", 1 ) )
assert( com_accepts( "com12:9600,n,8,1", 12 ) )
assert( com_accepts( "COM2147483647:", 2147483647 ) )
assert( com_rejects( "COM2147483648:" ) )
assert( com_rejects( "COM999999999999999999999:" ) )
assert( com_rejects( "COM1X:" ) )
assert( com_rejects( "COM-1:" ) )
assert( com_rejects( "plain-device" ) )

assert( lpt_parse( "PRN:", 1, 1, "", "", "" ) )
assert( lpt_parse( "LPT:", 0, 0, "", "", "" ) )
assert( lpt_parse( "LPT2: Printer One, TITLE = Job Name, EMU = RAW", 0, 2, "Printer One", "Job Name", "RAW" ) )

dim as string lpt3 = "LPT3:"
assert( fb_DevLptTestProtocol( 0, strptr( lpt3 ), len( lpt3 ) ) <> 0 )

assert( lpt_rejects( "LPT0:" ) )
assert( lpt_rejects( "LPTX:" ) )
assert( lpt_rejects( "LPT999999999999999999999:" ) )
assert( lpt_rejects( "PRINTER:" ) )

assert( tcp_parse( "host=127.0.0.1,port=12345,timeout=20", 0, "127.0.0.1", 12345, 20, 16 ) )
assert( tcp_parse( " port = 54321, backlog = 2 ", -1, "", 54321, 0, 2 ) )
assert( tcp_parse( "host=localhost,port=1,unknown=yes", 0, "localhost", 1, 0, 16 ) )
assert( tcp_rejects( "host=localhost", 0 ) )
assert( tcp_rejects( "port=12345", 0 ) )
assert( tcp_rejects( "host=localhost,port=0", 0 ) )
assert( tcp_rejects( "host=localhost,port=65536", 0 ) )
assert( tcp_rejects( "host=localhost,port=4294967297", 0 ) )
assert( tcp_rejects( "host=localhost,port=123,timeout=4294967297", 0 ) )
assert( tcp_rejects( "host=localhost,port=123,backlog=-1", 0 ) )
assert( tcp_rejects( "host=localhost,port=abc", 0 ) )

'' end of device-protocols.bas
