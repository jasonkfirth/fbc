'' Project: FreeBASIC C-emitter tests
'' File: static-local-order.bas
'' Purpose: Preserve static storage and call order when C declarations move.
'' Responsibilities: Check persistent scalars, arrays, descriptors and sibling locals.
'' This file intentionally does NOT depend on a replacement runtime or observer.

'' TEST_MODE : COMPILE_AND_RUN_OK

dim shared events as integer

sub touch( byval value as integer )
	events = events * 10 + value
end sub

function visit( byval first as integer ) as integer
	touch( 1 )
	static count as integer = 40
	static values(2 to 4) as integer = { 3, 5, 7 }
	static label as zstring * 7 = "stored"
	if( lbound(values) <> 2 or ubound(values) <> 4 ) then end 1
	if( label <> "stored" ) then end 2
	count += 1
	values(3) += 1
	dim result as integer = count + values(3)
	if( first ) then
		static selected as integer = 100
		selected += 1
		result += selected
	else
		static selected as integer = 200
		selected += 1
		result += selected
	end if
	touch( 2 )
	return result
end function

if( visit(-1) <> 148 or events <> 12 ) then end 3
if( visit(-1) <> 151 or events <> 1212 ) then end 4
if( visit(0) <> 252 or events <> 121212 ) then end 5
if( visit(0) <> 255 or events <> 12121212 ) then end 6
end 0

'' end of static-local-order.bas
