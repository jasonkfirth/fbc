'' Project: FreeBASIC compiler tests
'' File: c-temp-assignment.bas
'' Purpose: Keep call evaluation separate from C temporary storage.
'' Responsibilities: Check skipped calls, loops, pointer results and aggregates.
'' This file intentionally does NOT use platform I/O or uninitialized values.
'' TEST_MODE : COMPILE_AND_RUN_OK
#lang "fb"

type AggregateValue
	value as const long
	readonly_target as const long ptr
end type

dim shared call_count as long
dim shared stored_value as long = 27

private function touch( byval value as long ) as long

	call_count += 1
	return value

end function

private function get_pointer( ) as const long ptr

	call_count += 1
	return @stored_value

end function

private function get_aggregate( ) as AggregateValue

	return type<AggregateValue>( touch( 23 ), @stored_value )

end function

private function aggregate_roundtrip( ) as long

	dim as AggregateValue result = get_aggregate( )
	if( *result.readonly_target <> 27 ) then end 20
	return result.value

end function

if( aggregate_roundtrip( ) <> 23 ) then end 1
if( touch( 0 ) ) then
	end touch( 99 )
end if

dim as long total
for index as long = 0 to 4
	if( (index and 1) = 0 ) then
		total += touch( index )
	else
		continue for
	end if
next

if( total <> 6 ) then end 2
if( call_count <> 5 ) then end 3
if( *get_pointer( ) <> 27 ) then end 4
if( call_count <> 6 ) then end 5

dim as integer logical_result
logical_result = (touch( 0 ) andalso touch( 99 ))
if( logical_result <> 0 ) then end 6
logical_result = (touch( 1 ) orelse touch( 99 ))
if( logical_result = 0 ) then end 7
if( call_count <> 8 ) then end 8
logical_result = (touch( 1 ) andalso (*get_pointer( ) = 27))
if( logical_result = 0 ) then end 9
if( call_count <> 10 ) then end 10

logical_result = (touch( 0 ) andalso (get_aggregate( ).value = 99))
if( logical_result <> 0 ) then end 11
if( call_count <> 11 ) then end 12
logical_result = (touch( 1 ) andalso (get_aggregate( ).value = 23))
if( logical_result = 0 ) then end 13
if( call_count <> 13 ) then end 14

print "c-temp-assignment-ok"
end 0

'' end of c-temp-assignment.bas
