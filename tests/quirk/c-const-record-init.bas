'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-const-record-init.bas
'' Purpose: Verify runtime initialization of constant records in the C backend.
'' Responsibilities: Check member stores, nested records, and const pointers.
'' This file intentionally does NOT test writes forbidden by the BASIC frontend.

type const_init_point
	x as integer
	y as integer
	z as integer
end type

type const_init_record
	point as const_init_point
	value as const integer ptr
end type

function point_total( byref item as const const_init_point ) as integer
	return item.x + item.y + item.z
end function

sub check_initializers( )
	dim target as integer = 17
	dim as const const_init_point item = (1, 2, 3)
	dim as const const_init_record record = ((4, 5, 6), @target)
	if point_total(item) <> 6 then end 1
	if point_total(record.point) <> 15 then end 2
	if *record.value <> 17 then end 3
end sub

check_initializers( )

'' end of c-const-record-init.bas
