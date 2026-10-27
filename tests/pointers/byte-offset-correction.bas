'' Project: FreeBASIC C-emitter tests
'' File: byte-offset-correction.bas
'' Purpose: Keep corrected byte offsets inside the accessed array object.
'' Responsibilities: Check reads, writes, string indices and native-width offsets.
'' This file intentionally does NOT dereference an out-of-range final address.

'' TEST_MODE : COMPILE_AND_RUN_OK

private function read_tail( byval byte_base as const ubyte ptr, byval index as integer ) as ubyte
	return *(byte_base + index - 2)
end function

private function read_head( byval byte_base as const ubyte ptr, byval index as integer ) as ubyte
	return *(byte_base + index + 2)
end function

private sub write_tail( byval byte_base as ubyte ptr, byval index as integer, byval value as ubyte )
	*(byte_base + index - 2) = value
end sub

private function read_wide( byval byte_base as const ubyte ptr, byval index as longint ) as ubyte
	return *(byte_base + index - 2)
end function

private function read_unsigned( byval byte_base as const ubyte ptr, byval index as uinteger ) as ubyte
	return *(byte_base + index - 2)
end function

private function read_large( byval byte_base as const ubyte ptr, byval index as longint ) as ubyte
#ifdef __FB_64BIT__
	return *(byte_base + index - 4294967298ll)
#else
	'' Pointer offsets are converted to the target width before correction.
	return *(byte_base + index - 2)
#endif
end function

dim shared index_calls as integer

private function next_index() as integer
	index_calls += 1
	return 2
end function

private function read_once( byval byte_base as const ubyte ptr ) as ubyte
	return *(byte_base + next_index() - 2)
end function

private function mutable_view( byval byte_base as const ubyte ptr, byval index as integer ) as ubyte ptr
	'' An explicit mutable view must retain its qualifier conversion.
	return cptr( ubyte ptr, byte_base + index - 2 )
end function

private function character_at( byref text as const string, byval index as integer ) as ubyte
	return asc( text, index )
end function

dim bytes(0 to 7) as ubyte = { 11, 22, 33, 44, 55, 66, 77, 88 }
if( read_tail(@bytes(7), 2) <> 88 ) then end 1
if( read_head(@bytes(0), -2) <> 11 ) then end 2
write_tail( @bytes(7), 2, 99 )
if( bytes(7) <> 99 or bytes(6) <> 77 or bytes(0) <> 11 ) then end 3
if( read_wide(@bytes(7), 2) <> 99 ) then end 6
if( read_unsigned(@bytes(7), 2) <> 99 ) then end 7
if( read_large(@bytes(7), 4294967298ll) <> 99 ) then end 8
if( read_once(@bytes(7)) <> 99 or index_calls <> 1 ) then end 9
dim mutable_bytes as ubyte ptr = mutable_view( @bytes(7), 2 )
*mutable_bytes = 101
if( bytes(7) <> 101 or bytes(6) <> 77 ) then end 10
dim text as string = "abcd"
if( character_at(text, 1) <> asc("a") ) then end 4
if( character_at(text, 4) <> asc("d") ) then end 5
end 0

'' end of byte-offset-correction.bas
