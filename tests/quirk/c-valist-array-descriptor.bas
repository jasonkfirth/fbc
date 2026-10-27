'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-valist-array-descriptor.bas
'' Purpose: Check array descriptors for native va_list storage.
'' Responsibilities: Initialize and consume separate variadic list elements.
'' This file intentionally does NOT define a platform-specific va_list layout.

sub check_arguments cdecl( byval count as integer, ... )
	dim lists(1 to 2) as cva_list
	for index as integer = 1 to 2
		cva_start( lists(index), count )
		if cva_arg( lists(index), integer ) <> 37 then end 1
		if cva_arg( lists(index), integer ) <> 81 then end 2
		cva_end( lists(index) )
	next
end sub

check_arguments( 2, 37, 81 )

'' end of c-valist-array-descriptor.bas
