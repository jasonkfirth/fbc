'' Project: FreeBASIC compiler assignment qualification
'' File: ustring-scalar-swap.bas
'' Purpose: Keep SWAP type checking separate from indexed assignment lowering.
'' Responsibilities: Verify both scalar values survive compatibility checks.
'' This file intentionally does NOT read raw UTF-8 bytes.
#lang "fb"

const SCALAR_A = asc( "a" )
const SCALAR_B = asc( "b" )
const SMILING_FACE = &h1F642

'' Module state: the test owns this counter for effectful scalar indices.
dim shared as integer index_calls
function NextIndex( ) as integer
	dim as integer result = index_calls
	index_calls += 1
	return result
end function

dim as ustring text_value = "ab"
swap text_value[0], text_value[1]
if( text_value[0] <> SCALAR_B ) orelse (text_value[1] <> SCALAR_A ) then end 1

'' Swapping unequal encoded widths must preserve scalar indices after the
'' first indexed write changes the descriptor's byte buffer.
text_value = uchr( SMILING_FACE, SCALAR_B )
swap text_value[0], text_value[1]
if( text_value[0] <> SCALAR_B ) orelse (text_value[1] <> SMILING_FACE ) then end 2
if( len( text_value ) <> 2 ) then end 3

dim as ulong scalar_value = SCALAR_A
swap text_value[0], scalar_value
if( text_value[0] <> SCALAR_A ) orelse (scalar_value <> SCALAR_B ) then end 4

text_value = "ab"
swap text_value[NextIndex( )], text_value[NextIndex( )]
if( index_calls <> 2 ) then end 5
if( text_value[0] <> SCALAR_B ) orelse (text_value[1] <> SCALAR_A ) then end 6
print "USTRING_SCALAR_SWAP_PASS"

'' end of ustring-scalar-swap.bas
