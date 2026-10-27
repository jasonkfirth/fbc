/'
    Project: FreeBASIC C emitter audit
    File: c-data-payload-readonly.bas
    Purpose: Check a readonly C view of the tagged DATA payload.
    Responsibilities: Exercise READ/RESTORE, narrow/wide text, numeric values,
        empty text, block links and an address of writable storage.
    This file intentionally does NOT modify literals or run a legacy game.
'/
' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fb"

const INITIAL_MARKER = 17
const UPDATED_MARKER = 29
const WIDE_TEXT = wstr( "wide" )
dim shared as long marker = INITIAL_MARKER

first_block:
data "alpha", 123, WIDE_TEXT, @marker, 1.5, ""

second_block:
data -7, "omega"

dim as string text
dim as long number
dim as wstring * 16 wide_value
dim as uinteger stored_address
dim as double fraction

restore first_block
read text, number, wide_value, stored_address, fraction
if text <> "alpha" then end 1
if number <> 123 then end 2
if wide_value <> WIDE_TEXT then end 3
if stored_address <> cuint( @marker ) then end 4
dim as long ptr read_pointer = cptr( long ptr, stored_address )
if *read_pointer <> INITIAL_MARKER then end 13

' UINTEGER preserves an address at both target widths without an implicit
' pointer-to-scalar READ conversion. The original object stays writable.
*read_pointer = UPDATED_MARKER
if marker <> UPDATED_MARKER then end 5

' 1.5 has an exact binary representation, so this checks conversion exactly.
if fraction <> 1.5 then end 6
read text
if text <> "" then end 7

' Advancing between separately labelled blocks exercises the descriptor link.
read number, text
if number <> -7 then end 8
if text <> "omega" then end 9
read number
if number <> 0 then end 10

restore first_block
read text
if text <> "alpha" then end 11
restore second_block
read number
if number <> -7 then end 12

end 0

' end of c-data-payload-readonly.bas
