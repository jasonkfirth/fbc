/'
	Project: FreeBASIC C backend regression tests
	File: c-indirect-integer-ranges.bas
	Purpose: Fold fixed integer comparisons without dropping indirect evaluation.
	Responsibilities: Check member, pointer, array and reference reads, side effects,
	short-circuit branches, integer/Boolean results and conversion boundaries.
	This file contains no graphics, audio or external library dependencies.
'/
#lang "fb"
#if (__FB_BACKEND__ = "gcc") or (__FB_BACKEND__ = "clang")
	#cmdline "-Wc -Werror=type-limits"
#endif

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_INDIRECT_INTEGER_RANGES_FAILED", __LINE__
		end 1
	end if
#endmacro

type SampleValues
	unsigned_byte as ubyte
	signed_byte as byte
	unsigned_long as ulong
end type

dim shared as SampleValues sample
dim shared as ubyte values(0 to 1)
dim shared as integer address_calls, index_calls, arm_calls

private function NextSample() as SampleValues ptr
	address_calls += 1
	return @sample
end function

private function NextIndex() as integer
	index_calls += 1
	return index_calls mod 2
end function

private function FixedReference(byref value as ubyte) as integer
	return value >= 0
end function

private function RuntimeGuard(byval value as integer) as integer
	return value
end function

private function ChosenArm(byval value as integer) as integer
	arm_calls += 1
	return value
end function

private sub CheckValues(byval unsigned_value as ubyte, byval signed_value as byte)
	sample.unsigned_byte = unsigned_value
	sample.signed_byte = signed_value
	sample.unsigned_long = &hfffffffful
	values(0) = unsigned_value
	values(1) = unsigned_value
	address_calls = 0
	index_calls = 0
	arm_calls = 0

	dim as integer result = NextSample()->unsigned_byte < 0
	VERIFY(result = 0)
	VERIFY(address_calls = 1)
	result = NextSample()->unsigned_byte <= 255
	VERIFY(result = -1)
	VERIFY(address_calls = 2)
	dim as boolean flag = NextSample()->unsigned_byte >= 0
	VERIFY(flag = true)
	VERIFY(address_calls = 3)
	flag = NextSample()->unsigned_byte > 255
	VERIFY(flag = false)
	VERIFY(address_calls = 4)

	if NextSample()->signed_byte < -128 then
		VERIFY(0)
	end if
	VERIFY(address_calls = 5)
	if NextSample()->signed_byte <= 127 then
		VERIFY(address_calls = 6)
	else
		VERIFY(0)
	end if
	if not (NextSample()->unsigned_byte >= 0) then
		VERIFY(0)
	end if
	VERIFY(address_calls = 7)

	VERIFY((values(NextIndex()) < 0) = 0)
	VERIFY(index_calls = 1)
	VERIFY((values(NextIndex()) <= 255) = -1)
	VERIFY(index_calls = 2)
	VERIFY(FixedReference(values(NextIndex())) = -1)
	VERIFY(index_calls = 3)

	'' A fixed comparison in a skipped arm must not evaluate its address.
	if RuntimeGuard(0) andalso (NextSample()->unsigned_byte < 0) then
		VERIFY(0)
	end if
	VERIFY(address_calls = 7)
	if RuntimeGuard(-1) orelse (NextSample()->unsigned_byte > 255) then
		VERIFY(address_calls = 7)
	else
		VERIFY(0)
	end if

	VERIFY((clngint(NextSample()->unsigned_byte) < 0ll) = 0)
	VERIFY((culngint(NextSample()->signed_byte) >= 0ull) = -1)
	VERIFY(address_calls = 9)
	'' Narrowing into a signed type is not a value-preserving range proof.
	VERIFY(clng(NextSample()->unsigned_long) = -1)
	VERIFY(address_calls = 10)
	VERIFY((NextSample()->unsigned_byte < 128) = (unsigned_value < 128))
	VERIFY((NextSample()->signed_byte < 0) = (signed_value < 0))
	VERIFY(address_calls = 12)

	dim as integer choice = iif(NextSample()->unsigned_byte < 0, ChosenArm(11), ChosenArm(22))
	VERIFY(choice = 22)
	VERIFY(address_calls = 13 and arm_calls = 1)
	choice = iif(NextSample()->unsigned_byte <= 255, ChosenArm(33), ChosenArm(44))
	VERIFY(choice = 33)
	VERIFY(address_calls = 14 and arm_calls = 2)
	choice = iif(NextSample()->unsigned_byte >= 0, _
		iif(values(NextIndex()) <= 255, ChosenArm(55), ChosenArm(66)), ChosenArm(77))
	VERIFY(choice = 55)
	VERIFY(address_calls = 15 and index_calls = 4 and arm_calls = 3)
	choice = iif(NextSample()->unsigned_byte < 128, ChosenArm(88), ChosenArm(99))
	VERIFY(choice = iif(unsigned_value < 128, 88, 99))
	VERIFY(address_calls = 16 and arm_calls = 4)

	'' Ordinary bitwise operators must evaluate both operands even when one
	'' fixed comparison determines their result.
	if (NextSample()->unsigned_byte <= 255) or (values(NextIndex()) > 0) then
		VERIFY(address_calls = 17 and index_calls = 5)
	else
		VERIFY(0)
	end if
	if (NextSample()->unsigned_byte > 255) and (values(NextIndex()) > 0) then
		VERIFY(0)
	end if
	VERIFY(address_calls = 18 and index_calls = 6)
end sub

for value as integer = 0 to 255
	CheckValues(cubyte(value), cbyte(value - 128))
next
print "C_INDIRECT_INTEGER_RANGES_PASS"

'' end of c-indirect-integer-ranges.bas
