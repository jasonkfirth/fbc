' TEST_MODE : COMPILE_ONLY_OK

'
' C backend const prototype regression test
'
' The C backend must preserve pointee const in external prototypes.  If these
' parameters are emitted without const, GCC reports discarded qualifiers when
' the calls below pass const pointers.
'

#cmdline "-gen gcc"
#cmdline "-Wc -Wno-unknown-warning-option"
#cmdline "-Wc -Werror=discarded-qualifiers"
#cmdline "-restart"

extern "C"
	declare function fb_const_proto_any cdecl alias "fb_const_proto_any" ( byval as const any ptr ) as any ptr
	declare function fb_const_proto_zstring cdecl alias "fb_const_proto_zstring" ( byval as const zstring ptr ) as integer
	declare sub fb_const_proto_ubyte cdecl alias "fb_const_proto_ubyte" ( byval as const ubyte ptr )
end extern

dim as const any ptr any_value = 0
dim as const zstring ptr text_value = @"abcdef"
dim as ubyte bytes(0 to 3) = { 1, 2, 3, 4 }
dim as const ubyte ptr byte_value = @bytes(0)

fb_const_proto_any( any_value )
fb_const_proto_zstring( text_value )
fb_const_proto_ubyte( byte_value )

' end of c-prototype-const.bas
