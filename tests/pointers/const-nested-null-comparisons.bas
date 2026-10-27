/'
    Project: FreeBASIC C backend regression tests
    File: const-nested-null-comparisons.bas
    Purpose: Preserve valid C null comparisons for nested qualified pointers.
    Responsibilities: Check both equality orders and both null/non-null values.
    This file intentionally does NOT dereference null pointers or use graphics.
'/
' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fb"

function nested_flags( byval text as const zstring const ptr ptr ) as integer

	dim as integer flags = 0
	if text = 0 then flags or= 1
	if 0 = text then flags or= 2
	if text <> 0 then flags or= 4
	if 0 <> text then flags or= 8
	return flags

end function

function deeper_flags( byval text as const zstring const ptr const ptr ptr ) as integer

	dim as integer flags = 0
	if text = 0 then flags or= 1
	if 0 = text then flags or= 2
	if text <> 0 then flags or= 4
	if 0 <> text then flags or= 8
	return flags

end function

dim as zstring ptr text = strptr( "qualified" )
dim as const zstring const ptr ptr address = @text

if nested_flags( 0 ) <> 3 then end 1
if nested_flags( @text ) <> 12 then end 2
if deeper_flags( 0 ) <> 3 then end 3
if deeper_flags( @address ) <> 12 then end 4

' end of const-nested-null-comparisons.bas
