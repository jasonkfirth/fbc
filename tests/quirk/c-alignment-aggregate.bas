'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC C alignment qualification
'' File: c-alignment-aggregate.bas
'' Purpose: Check whole-record copies through an unaligned record pointer.
'' Responsibilities: Cover explicit BYVAL arguments, return values and assignment.
'' This file intentionally does NOT access beyond its byte buffer.
#lang "fb"
type aggregate_record
	marker as ubyte
	value as longint
	amount as double
end type
private function copy_record( byval value as aggregate_record ) as aggregate_record
	return value
end function
dim bytes(0 to 255) as ubyte
dim as aggregate_record ptr record_pointer = cptr(aggregate_record ptr, @bytes(1))
record_pointer->marker = 123
record_pointer->value = 123456789012345ll
record_pointer->amount = 1.25
dim as aggregate_record aligned_copy = copy_record(*record_pointer)
if( aligned_copy.marker <> 123 or aligned_copy.value <> 123456789012345ll ) then end 1
if( aligned_copy.amount <> 1.25 ) then end 2
aligned_copy.value += 1
*record_pointer = aligned_copy
if( record_pointer->value <> 123456789012346ll ) then end 3
print "aggregate-view-ok"
'' end of c-alignment-aggregate.bas
