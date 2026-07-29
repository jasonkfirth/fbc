''
'' FreeBASIC Compiler Test Suite
''
'' File: cva-invalid-udt.bas
''
'' Verify that the CVA statements reject ordinary UDTs.  This test does not
'' exercise the target-specific cva_list representations covered by
'' va_cva_api.bas.
''
'' TEST_MODE : COMPILE_ONLY_FAIL

type NOT_A_VA_LIST
	value as integer
end type

sub variadicProc cdecl( byval fixedarg as integer, ... )
	dim list as NOT_A_VA_LIST
	cva_start( list, fixedarg )
end sub

'' end of cva-invalid-udt.bas
