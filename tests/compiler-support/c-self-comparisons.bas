/'
	Project: FreeBASIC C backend regression controls
	File: c-self-comparisons.bas
	Purpose: Check scalar identity folding and the observations it must retain.
	Responsibilities: BASIC boolean results, conditional branches, null pointer
	values, integer aliases, 128-bit values, evaluated calls and unordered floats.
	This file intentionally does NOT contain: Platform APIs or shared-thread tests.
'/
#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_SELF_COMPARISONS_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as integer value_calls, address_calls
dim shared as integer values(0 to 1) = { 11, 29 }

private function NextValue() as integer
	value_calls += 1
	return value_calls
end function

private function NextAddress() as integer ptr
	address_calls += 1
	return @values(address_calls mod 2)
end function

private sub CheckSigned(byval value as integer, byval other as integer)
	#ifdef CHECK_OPAQUE
		asm
			nop
		end asm
	#endif
	VERIFY((value = value) = -1)
	VERIFY((value <> value) = 0)
	VERIFY((value < value) = 0)
	VERIFY((value > value) = 0)
	VERIFY((value <= value) = -1)
	VERIFY((value >= value) = -1)
	VERIFY(value <> other)
	VERIFY(((value and 255) shl 8) = ((value and 255) shl 8))
	VERIFY((value + other) = (value + other))
	if value <> value then end 2
	if value = value then
		VERIFY(clngint(value) = value)
	else
		end 3
	end if
end sub

private function CheckPointer(byval value as integer ptr) as integer ptr
	VERIFY((value = value) = -1)
	VERIFY((value <> value) = 0)
	VERIFY((value < value) = 0)
	VERIFY((value > value) = 0)
	VERIFY((value <= value) = -1)
	VERIFY((value >= value) = -1)
	return value
end function

#if defined(__FB_64BIT__) and not defined(CHECK_NON_C_BACKEND)
private function CheckWide(byval value as integer<128>) as integer<128>
	VERIFY((value = value) = -1)
	VERIFY((value <> value) = 0)
	VERIFY((value < value) = 0)
	VERIFY((value > value) = 0)
	VERIFY((value <= value) = -1)
	VERIFY((value >= value) = -1)
	return value
end function
#endif

type DoubleBits
	union
		bits as ulongint
		value as double
	end union
end type

type RecordValue
	value as integer
end type

dim as DoubleBits unordered
unordered.bits = &h7ff8000000000000ull '' IEEE binary64 quiet NaN.
VERIFY((unordered.value = unordered.value) = 0)
VERIFY((unordered.value <> unordered.value) = -1)
VERIFY((unordered.value <= unordered.value) = 0)
VERIFY((unordered.value >= unordered.value) = 0)

CheckSigned(-23, 71)
CheckSigned(&h7fffffff, -1)
VERIFY(CheckPointer(@values(0)) = @values(0))
VERIFY(CheckPointer(0) = 0)
#if defined(__FB_64BIT__) and not defined(CHECK_NON_C_BACKEND)
	VERIFY(CheckWide(cast(integer<128>, 1) shl 100) = (cast(integer<128>, 1) shl 100))
	VERIFY(CheckWide(-(cast(integer<128>, 1) shl 100)) = -(cast(integer<128>, 1) shl 100))
#endif
VERIFY(@values(0) = @values(0))
dim as RecordValue record_value
VERIFY(@record_value.value = @record_value.value)
dim as integer ptr element_address = @values(0)
VERIFY(@element_address[1] = element_address + 1)
VERIFY(*element_address = 11)
VERIFY(NextValue() <> NextValue())
VERIFY(value_calls = 2)
VERIFY(NextAddress() <> NextAddress())
VERIFY(address_calls = 2)
print "C_SELF_COMPARISONS_PASS"

/' end of c-self-comparisons.bas '/
