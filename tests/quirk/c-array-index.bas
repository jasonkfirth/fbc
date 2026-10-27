'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-array-index.bas
'' Purpose: Check typed C indexing against BASIC array bounds and byte views.
'' Responsibilities: Cover signed indices, side effects, members and addresses.
'' This file intentionally does NOT dereference outside its declared arrays.
#lang "fb"

type index_record
	first_value as integer
	second_value as longint
end type

dim shared records(-3 to 3) as index_record
dim shared scalars(5 to 8) as longint
dim shared distant(-20 to -17) as longint
dim shared as integer index_calls

private function next_index( byval value as integer ) as integer
	index_calls += 1
	return value
end function

for i as integer = -3 to 3
	records(i).first_value = i + 10
	records(i).second_value = i + 1000
next
for i as integer = -3 to 3
	if( records(i).first_value <> i + 10 ) then end 1
	if( records(i).second_value <> i + 1000 ) then end 2
next

for i as integer = -2 to 2
	if( records(i - 1).second_value <> i + 999 ) then end 14
	if( records(i + 1).first_value <> i + 11 ) then end 15
next

index_calls = 0
records(next_index(-1)).second_value += 1
if( index_calls <> 1 or records(-1).second_value <> 1000 ) then end 3
index_calls = 0
dim as longint ptr member_address = @records(next_index(0)).second_value
*member_address = 1234
if( index_calls <> 1 or records(0).second_value <> 1234 ) then end 4

for i as integer = 5 to 8
	scalars(i) = i * 10
next
for i as integer = 5 to 8
	if( scalars(i) <> i * 10 ) then end 5
next
for i as integer = -20 to -17
	distant(i) = -i
next
for i as integer = -20 to -17
	if( distant(i) <> -i ) then end 6
next

index_calls = 0
dim as uinteger unsigned_index = cuint(next_index(-2))
if( records(unsigned_index).first_value <> 8 or index_calls <> 1 ) then end 16
index_calls = 0
dim as longint ptr virtual_origin = @scalars(next_index(0))
if( index_calls <> 1 ) then end 17
if( cuint(virtual_origin) + 5 * sizeof(longint) <> cuint(@scalars(5)) ) then end 18
index_calls = 0
dim as longint ptr past_end = @scalars(next_index(9))
if( index_calls <> 1 ) then end 19
if( cuint(past_end) <> cuint(@scalars(8)) + sizeof(longint) ) then end 20

'' Bounds near the native minimum must not require signed negation of the bound.
#ifdef __FB_64BIT__
const MIN_INDEX as integer = (-9223372036854775807ll - 1ll)
#else
const MIN_INDEX as integer = (-2147483647 - 1)
#endif
dim at_minimum(MIN_INDEX to MIN_INDEX + 2) as longint
for i as integer = MIN_INDEX to MIN_INDEX + 2
	at_minimum(i) = i - MIN_INDEX + 20
next
for i as integer = MIN_INDEX to MIN_INDEX + 2
	if( at_minimum(i) <> i - MIN_INDEX + 20 ) then end 7
next

dim addresses(0 to 1) as index_record ptr = { @records(-3), @records(3) }
dim as index_record ptr ptr cursor = @addresses(0)
cursor += 1
if( (*cursor)->second_value <> 1003 ) then end 8

scalars(6) = -3
dim as ulongint ptr raw_view = cptr(ulongint ptr, @scalars(6))
if( *raw_view <> culngint(-3ll) ) then end 9

'' Other ranks and string storage retain their separate representation contracts.
dim matrix(-1 to 1, 2 to 4) as integer
for i as integer = -1 to 1
	for j as integer = 2 to 4
		matrix(i, j) = i * 10 + j
	next
next
for i as integer = -1 to 1
	for j as integer = 2 to 4
		if( matrix(i, j) <> i * 10 + j ) then end 10
	next
next
dim texts(-1 to 1) as string * 8
texts(-1) = "left"
texts(0) = "middle"
texts(1) = "right"
if( texts(-1) <> "left    " or texts(1) <> "right   " ) then end 11

private sub inspect_indices cdecl( byval sentinel as long, ... )
	dim as cva_list arguments = any
	cva_start( arguments, sentinel )
	if( records(cva_arg(arguments, integer)).first_value <> 8 ) then end 12
	if( cva_arg(arguments, integer) <> 99 ) then end 13
	cva_end( arguments )
end sub
inspect_indices( 0, cint(-2), cint(99) )

print "c-array-index-ok"
'' end of c-array-index.bas
