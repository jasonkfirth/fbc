'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-local-type-order.bas
'' Purpose: Keep local type definitions before their C storage declarations.
'' Responsibilities: Exercise procedure locals and sibling nested type scopes.
'' This file intentionally does NOT test public or cross-module type layouts.

sub check_local_types( )
	dim as integer count = 1
	type local_record
		value as integer
	end type
	dim as local_record first = (2)
	if first.value <> 2 then end 1

	scope
		type item
			value as integer
		end type
		dim as item nested = (3)
		count += nested.value
	end scope
	scope
		type item
			value as double
		end type
		dim as item nested = (4.0)
		count += cint(nested.value)
	end scope
	if count <> 8 then end 2
end sub

check_local_types( )

function next_counter( ) as integer
	type counter_record
		value as integer
	end type
	static counter as counter_record = (10)
	counter.value += 1
	return counter.value
end function

if next_counter( ) <> 11 then end 3
if next_counter( ) <> 12 then end 4

'' end of c-local-type-order.bas
