'' Project: FreeBASIC compiler semantic tests
'' File: pointer-distance-links.bas
'' Purpose: Keep executable links after byte-pointer distance normalization.
'' Responsibilities: Cover direct, grouped and aggregate-member distances.
'' This file intentionally does NOT subtract unrelated storage at runtime.
#lang "fb"

type BufferState
	bytes(0 to 15) as ubyte
	cursor as ubyte ptr
end type

function ByteDistance(byval first as ubyte ptr, byval second as ubyte ptr) as integer
	return first - second
end function

sub RebaseBuffer(byval destination as BufferState ptr, byval original as BufferState ptr)
	destination->cursor = @destination->bytes(0) + (original->cursor - @original->bytes(0))
end sub

function BufferOffset(byval buffer as BufferState ptr) as integer
	return buffer->cursor - @buffer->bytes(0)
end function

'' end of pointer-distance-links.bas
