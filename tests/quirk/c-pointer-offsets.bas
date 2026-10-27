'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-pointer-offsets.bas
'' Purpose: Verify typed element strides and qualifier-preserving byte fallbacks.
'' Responsibilities: Cover signed offsets, pointer widths and single evaluation.
'' This file intentionally does NOT form addresses outside its test allocations.
#lang "fb"
type step_record
	count as integer
	value as double
end type
dim shared as step_record rows(0 to 3)
dim shared as integer evaluations
function record_address( byval index as integer ) as step_record ptr
	evaluations += 1
	return @rows(index)
end function
function record_offset() as integer
	evaluations += 1
	return 2
end function
for index as integer = 0 to 3
	rows(index).count = index * 11
	rows(index).value = index + 0.25
next

dim as step_record ptr cursor = @rows(0)
cursor += 2
if( cursor->count <> 22 or cursor->value <> 2.25 ) then end 1
cursor -= 1
if( cursor <> @rows(1) ) then end 2
cursor += -1
if( cursor <> @rows(0) ) then end 3
cursor = @rows(3) + 1
cursor -= 1
if( cursor <> @rows(3) or cursor - @rows(0) <> 3 ) then end 4

dim as const step_record ptr readonly_cursor = @rows(0)
readonly_cursor += 2
if( readonly_cursor->count <> 22 ) then end 5
readonly_cursor -= 1
if( readonly_cursor->value <> 1.25 ) then end 6
dim as integer changing_offset = 2
readonly_cursor = @rows(0) + changing_offset
if( readonly_cursor->count <> 22 ) then end 7
cursor = record_address(0) + record_offset()
if( cursor <> @rows(2) or evaluations <> 2 ) then end 8

dim as integer numbers(0 to 3) = { 2, 3, 5, 7 }
dim as const integer ptr number_cursor = @numbers(3)
number_cursor -= 3
if( *number_cursor <> 2 ) then end 9
number_cursor += 2
if( *number_cursor <> 5 ) then end 10

dim as step_record ptr addresses(0 to 1) = { @rows(0), @rows(3) }
dim as step_record ptr ptr address_cursor = @addresses(0)
address_cursor += 1
if( (*address_cursor)->count <> 33 ) then end 11

'' ANY PTR displacements remain bytes rather than C void-pointer arithmetic.
dim as any ptr raw_address = @rows(0)
raw_address += 1
if( cuint(raw_address) - cuint(@rows(0)) <> 1 ) then end 12
raw_address -= 1
raw_address += sizeof(step_record)
if( raw_address <> @rows(1) ) then end 13

print "POINTER_OFFSETS_PASS"
end 0
'' end of c-pointer-offsets.bas
