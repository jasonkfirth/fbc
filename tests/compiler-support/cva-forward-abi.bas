'' Project: FreeBASIC compiler regression tests
'' -----------------------------------------
''
'' File: cva-forward-abi.bas
''
'' Purpose:
''
''     Check that C va_list values survive multiple BYVAL procedure calls.
''
'' Responsibilities:
''
''     - copy and consume a variadic string through nested Cva_List parameters
''     - exercise Cva_Start, Cva_Copy, Cva_Arg, and Cva_End with the host ABI
''
'' This file intentionally does NOT contain:
''
''     - platform-specific va_list layouts
''     - C library printf calls

function read_string cdecl( byval args as cva_list ) as integer
	dim as cva_list copied_args
	cva_copy( copied_args, args )
	dim as const zstring ptr value = cva_arg( copied_args, const zstring ptr )
	cva_end( copied_args )
	if( value = 0 ) then return -1
	return len( *value )
end function

function forward_string cdecl( byval args as cva_list ) as integer
	return read_string( args )
end function

function format_string cdecl( byval format as const zstring ptr, ... ) as integer
	dim as cva_list args
	cva_start( args, format )
	dim as integer result = forward_string( args )
	cva_end( args )
	return result
end function

if( format_string( "%s", "MATCH" ) <> 5 ) then end 1

print "C va_list forwarding passed"

'' end of cva-forward-abi.bas
