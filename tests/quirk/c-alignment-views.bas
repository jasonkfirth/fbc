'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-alignment-views.bas
'' Purpose: Access ordinary records through deliberately unaligned pointers.
'' Responsibilities: Cover member reads/writes, const views, pointer members and
''     local type declarations without changing their original layouts.
'' This file intentionally does NOT access beyond the backing byte buffer.
#lang "fb"

type view_record
	marker as ubyte
	value as longint
	amount as double
	next_record as view_record ptr
end type

private sub write_record( byval record_pointer as view_record ptr )
	record_pointer->marker = 123
	record_pointer->value = 123456789012345ll
	record_pointer->amount = 1.25
	record_pointer->next_record = record_pointer
end sub

private function read_record( byval record_pointer as const view_record ptr ) as integer
	if( record_pointer->marker <> 123 ) then return 1
	if( record_pointer->value <> 123456789012345ll ) then return 2
	if( record_pointer->amount <> 1.25 ) then return 3
	if( record_pointer->next_record->value <> record_pointer->value ) then return 4
	return 0
end function

private function local_view( byval buffer as ubyte ptr ) as longint
	type local_record
		marker as ubyte
		value as longint
	end type
	dim as local_record ptr record_pointer = cptr(local_record ptr, buffer)
	record_pointer->marker = 77
	record_pointer->value = 998877665544ll
	if( record_pointer->marker <> 77 ) then return -1
	return record_pointer->value
end function

dim bytes(0 to 255) as ubyte
for offset as integer = 1 to 9 step 2
	dim as view_record ptr record_pointer = cptr(view_record ptr, @bytes(offset))
	write_record(record_pointer)
	if( read_record(record_pointer) <> 0 ) then end 1
	if( local_view(@bytes(offset)) <> 998877665544ll ) then end 2
next

scope
	type first_local_record
		value as longint
	end type
	dim as first_local_record ptr record_pointer = cptr(first_local_record ptr, @bytes(3))
	record_pointer->value = 17
	if( record_pointer->value <> 17 ) then end 3
end scope
scope
	type second_local_record
		value as longint
	end type
	dim as second_local_record ptr record_pointer = cptr(second_local_record ptr, @bytes(5))
	record_pointer->value = 23
	if( record_pointer->value <> 23 ) then end 4
end scope
print "c-alignment-views-ok"
'' end of c-alignment-views.bas
