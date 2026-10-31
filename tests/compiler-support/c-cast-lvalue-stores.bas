#lang "fb"
/'
	Project: FreeBASIC C backend regression tests
	File: c-cast-lvalue-stores.bas
	Purpose: Check signed and unsigned views of the same integer storage.
	Responsibilities: Preserve high bits, logical shifts, actual storage types,
	packed field writes and single evaluation of indirect destinations.
	This file intentionally does NOT contain: Source ABI changes or diagnostic suppression.
'/

#macro VERIFY(condition)
	if (condition) = 0 then end 1
#endmacro

#macro CHECK_VIEWS(procedure_name, signed_type, unsigned_type)
sub procedure_name()
	#ifdef CHECK_OPAQUE
		asm
			nop
		end asm
	#endif
	dim as unsigned_type top_bit = cast(unsigned_type, 1) shl (sizeof(signed_type) * 8 - 1)
	dim as unsigned_type all_bits = cast(unsigned_type, -1)
	dim as signed_type signed_value = cast(signed_type, top_bit)
	cast(unsigned_type, signed_value) shr= 1
	VERIFY(signed_value = cast(signed_type, top_bit shr 1))
	cast(unsigned_type, signed_value) = all_bits
	VERIFY(signed_value = -1)
	dim as unsigned_type unsigned_value = top_bit
	cast(signed_type, unsigned_value) shr= 1
	VERIFY(unsigned_value = (top_bit or (top_bit shr 1)))
	cast(signed_type, unsigned_value) = -1
	VERIFY(unsigned_value = all_bits)
end sub
#endmacro

CHECK_VIEWS(CheckByte, byte, ubyte)
CHECK_VIEWS(CheckShort, short, ushort)
CHECK_VIEWS(CheckLong, long, ulong)
CHECK_VIEWS(CheckLongint, longint, ulongint)
CHECK_VIEWS(CheckInteger, integer, uinteger)

'' FIELD=1 exposes an unaligned destination. A signedness view must not
'' bypass the existing memcpy store or evaluate its getter twice.
type PackedWord field = 1
	before as ubyte
	value as long
	after as ubyte
end type

dim shared as PackedWord packed_value
dim shared as long address_calls

function WordAddress() as long ptr
	address_calls += 1
	return @packed_value.value
end function

sub CheckPacked()
	packed_value.before = &h5a
	packed_value.value = cast(long, &h80000000u)
	packed_value.after = &ha5
	cast(ulong, *WordAddress()) shr= 1
	VERIFY(packed_value.value = &h40000000)
	cast(ulong, packed_value.value) = &hffffffffu
	VERIFY(packed_value.value = -1)
	VERIFY(address_calls = 1)
	VERIFY(packed_value.before = &h5a and packed_value.after = &ha5)
end sub

CheckByte()
CheckShort()
CheckLong()
CheckLongint()
CheckInteger()
CheckPacked()
print "C_CAST_LVALUE_STORES_PASS"

'' end of c-cast-lvalue-stores.bas
