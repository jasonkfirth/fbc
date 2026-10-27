'' Project: FreeBASIC C-emitter tests
'' File: byte-offset-expressions.bas
'' Purpose: Correct native integer expressions before byte-pointer arithmetic.
'' Responsibilities: Check scaled offsets, field values, wrapping and evaluation.
'' This file intentionally does NOT access an out-of-range final address.

'' TEST_MODE : COMPILE_AND_RUN_OK

private function read_sum( byval byte_base as const ubyte ptr, byval first_index as integer, byval second_index as integer ) as ubyte
	return *(byte_base + (first_index + second_index) - 2)
end function

private function read_product( byval byte_base as const ubyte ptr, byval index as integer, byval stride as integer ) as ubyte
	return *(byte_base + (index * stride) - 2)
end function

private function read_shift( byval byte_base as const ubyte ptr, byval index as integer, byval shift_count as integer ) as ubyte
	return *(byte_base + (index shl shift_count) - 2)
end function

private function read_negative( byval byte_base as const ubyte ptr, byval index as integer ) as ubyte
	return *(byte_base + (-index) - 2)
end function

private function read_mask( byval byte_base as const ubyte ptr, byval index as integer, byval bit_mask as integer ) as ubyte
	return *(byte_base + (index and bit_mask) - 2)
end function

private function read_wrapped( byval byte_base as const ubyte ptr, byval first_index as integer, byval second_index as integer ) as ubyte
	return *(byte_base + (first_index + second_index) + 2)
end function

type offset_index
	value as integer
end type

private function read_member( byval byte_base as const ubyte ptr, byref position as offset_index ) as ubyte
	return *(byte_base + position.value - 2)
end function

type offset_packet
	prefix as integer
	values(-2 to 1) as integer
end type

private function read_element( byref packet as offset_packet, byval index as integer ) as integer
	return packet.values(index)
end function

private sub write_element( byref packet as offset_packet, byval index as integer, byval new_value as integer )
	packet.values(index) = new_value
end sub

dim shared index_calls as integer

private function next_index() as integer
	index_calls += 1
	return 2
end function

private function read_once( byval byte_base as const ubyte ptr, byval extra_offset as integer ) as ubyte
	return *(byte_base + (next_index() + extra_offset) - 2)
end function

dim bytes(0 to 7) as ubyte = { 11, 22, 33, 44, 55, 66, 77, 88 }
if( read_sum(@bytes(7), 1, 1) <> 88 ) then end 1
if( read_product(@bytes(7), 1, 2) <> 88 ) then end 2
if( read_shift(@bytes(7), 1, 1) <> 88 ) then end 3
if( read_negative(@bytes(7), -2) <> 88 ) then end 4
if( read_mask(@bytes(7), 10, 3) <> 88 ) then end 5

'' Two largest signed native values add to -2 under BASIC's wrapping rules.
dim native_max as integer = cint(cuint(-1) shr 1)
if( read_wrapped(@bytes(7), native_max, native_max) <> 88 ) then end 6
dim position as offset_index
position.value = 2
if( read_member(@bytes(7), position) <> 88 ) then end 7
if( read_once(@bytes(7), 0) <> 88 or index_calls <> 1 ) then end 8

dim packet as offset_packet
packet.prefix = 71
packet.values(-2) = 11
packet.values(-1) = 22
packet.values(0) = 33
packet.values(1) = 44
if( read_element(packet, -2) <> 11 or read_element(packet, 1) <> 44 ) then end 9
write_element( packet, 0, 99 )
if( packet.prefix <> 71 or packet.values(-2) <> 11 or packet.values(-1) <> 22 or packet.values(0) <> 99 or packet.values(1) <> 44 ) then end 10
end 0

'' end of byte-offset-expressions.bas
