'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-scalar-alignment.bas
'' Purpose: Check scalar transfers through byte buffers and unknown pointers.
'' Responsibilities: Preserve raw bit patterns, packed fields and evaluation counts.
'' This file intentionally does NOT access outside its backing storage.
#lang "fb"

dim shared as integer offset_calls
private function next_offset( byval value as integer ) as integer
	offset_calls += 1
	return value
end function

private sub write_word( byval bytes as ubyte ptr, byval offset as integer, byval value as ulongint )
	*cptr(ulongint ptr, bytes + offset) = value
end sub

private function read_word( byval bytes as ubyte ptr, byval offset as integer ) as ulongint
	return *cptr(ulongint ptr, bytes + offset)
end function

type packed_record field = 1
	padding as ubyte
	value as longint
end type

private sub increment_word( byref value as longint )
	value += 1
end sub

dim bytes(0 to 63) as ubyte
const WORD_VALUE as ulongint = &hfedcba9876543210ull
offset_calls = 0
write_word(@bytes(0), next_offset(1), WORD_VALUE)
if( read_word(@bytes(0), next_offset(1)) <> WORD_VALUE ) then end 1
if( offset_calls <> 2 ) then end 2

'' The pointer variable carries an address, not a promise about its low bits.
dim as longint ptr word_pointer = cptr(longint ptr, @bytes(9))
*word_pointer = -123456789012345ll
if( *word_pointer <> -123456789012345ll ) then end 3

dim as double ptr double_pointer = cptr(double ptr, @bytes(17))
*double_pointer = 1.25
if( *double_pointer <> 1.25 ) then end 4
if( read_word(@bytes(0), 17) <> &h3ff4000000000000ull ) then end 5

dim as packed_record packed
packed.padding = 123
packed.value = 456
increment_word(packed.value)
if( packed.padding <> 123 or packed.value <> 457 ) then end 6

'' Keep aligned scalar/array storage and larger strides as ordinary controls.
dim aligned_values(-1 to 1, 2 to 3) as longint
for i as integer = -1 to 1
	for j as integer = 2 to 3
		aligned_values(i, j) = i * 10 + j
	next
next
for i as integer = -1 to 1
	for j as integer = 2 to 3
		if( aligned_values(i, j) <> i * 10 + j ) then end 7
	next
next
print "c-scalar-alignment-ok"
'' end of c-scalar-alignment.bas
