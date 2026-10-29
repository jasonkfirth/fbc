/'
    Project: FreeBASIC C backend regression tests
    File: pointer-result-qualifiers.bas
    Purpose: Check pointed-to const on pointer arithmetic temporaries.
    Responsibilities: Check values, nested pointers and one offset evaluation.
    This file intentionally does NOT write through a read-only pointer.
'/
' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fb"

private function next_offset( byref calls as integer ) as integer
	calls += 1
	return 2
end function

dim as integer values(0 to 7) = { 10, 20, 30, 40, 50, 60, 70, 80 }
dim as const integer ptr first = @values(0), last = @values(7)
dim as integer offset = 2, calls = 0

if( first + offset <> @values(2) ) then end 1
if( offset + first <> @values(2) ) then end 2
if( last - offset <> @values(5) ) then end 3
if( first + 4 > last ) then end 4
if( first + offset >= last ) then end 5
if( first + next_offset( calls ) <> @values(2) ) then end 6
if( calls <> 1 ) then end 7
if( last - first <> 7 ) then end 8
if( *(first + offset) <> 30 ) then end 9

dim as const integer ptr pointers(0 to 1) = { first, last }
dim as const integer ptr const ptr slots = @pointers(0)
if( *(slots + 1) <> last ) then end 10
if( slots + 1 > @pointers(1) ) then end 11

dim as const wstring ptr text = @wstr("AB")
if( *(text + 1) <> asc("B") ) then end 12

type item
	value as integer
end type
dim as item records(0 to 1)
records(1).value = 91
dim as const item ptr record_first = @records(0)
if( (record_first + 1)->value <> 91 ) then end 13
end 0

'' end of pointer-result-qualifiers.bas
