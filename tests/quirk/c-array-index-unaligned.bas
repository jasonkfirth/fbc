'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-array-index-unaligned.bas
'' Purpose: Check byte-array addresses used for unaligned integer access.
'' Responsibilities: Preserve one offset evaluation and a full word round trip.
'' This file intentionally does NOT access outside the byte buffer.
#lang "fb"

dim shared as integer offset_calls
private function byte_offset() as integer
	offset_calls += 1
	return 1
end function

dim bytes(0 to 31) as ubyte
'' A nonzero word distinguishes the write from the buffer's initial zeroes.
const WORD_VALUE as longint = &h0102030405060708ll
*cptr(longint ptr, @bytes(byte_offset())) = WORD_VALUE
if( *cptr(longint ptr, @bytes(byte_offset())) <> WORD_VALUE ) then end 1
if( offset_calls <> 2 ) then end 2
print "c-array-index-unaligned-ok"
'' end of c-array-index-unaligned.bas
