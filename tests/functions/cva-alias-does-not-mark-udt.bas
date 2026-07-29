''
'' FreeBASIC Compiler Test Suite
''
'' File: cva-alias-does-not-mark-udt.bas
''
'' Verify that a __builtin_va_list typedef does not change the UDT it names
'' into a va_list type.  CVA_START must reject the original UDT.
''
'' TEST_MODE : COMPILE_ONLY_FAIL

type VA_LAYOUT
	value as integer
end type

type PRIVATE_VA_LIST as VA_LAYOUT alias "__builtin_va_list"

sub variadicProc cdecl( byval fixedarg as integer, ... )
	dim list as VA_LAYOUT
	cva_start( list, fixedarg )
end sub

'' end of cva-alias-does-not-mark-udt.bas
