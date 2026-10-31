/'
	Project: FreeBASIC unary result regression controls
	File: c-unary-result-types.bas
	Purpose: Preserve signed unary results from unsigned operands.
	Responsibilities: Explicit boundary values, ordered comparisons, shifts,
	conversions, nested negation, lazy evaluation and in-place assignment.
	This file intentionally does NOT contain: Platform APIs, allocation stress
	or 128-bit alternate-backend ABI probes.
'/
#lang "fb"

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_UNARY_RESULT_TYPES_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as integer calls

private function UnsignedValue(byval value as ulongint) as ulongint
	calls += 1
	return value
end function

private sub Check64(byval value as ulongint, byval expected as longint)
	VERIFY(-value = expected)
	VERIFY((-value < 0ll) = (expected < 0ll))
	VERIFY((-value > 0ll) = (expected > 0ll))
	VERIFY((-value <= -1ll) = (expected <= -1ll))
	VERIFY((-value >= 1ll) = (expected >= 1ll))
	VERIFY((-value shr 1) = (expected shr 1))
	VERIFY((-value \ 3ll) = (expected \ 3ll))
	VERIFY(cdbl(-value) = cdbl(expected))
	VERIFY(sgn(-value) = sgn(expected))
	VERIFY(culngint(-(-value)) = value)
	VERIFY(not(-value) = not(expected))
	'' Calls are evaluated once, including when a converted unary result is
	'' consumed directly by another operation rather than stored in a variable.
	calls = 0
	VERIFY(-UnsignedValue(value) = expected)
	VERIFY(calls = 1)
	VERIFY(cdbl(-UnsignedValue(value)) = cdbl(expected))
	VERIFY(calls = 2)
	VERIFY(iif(value = 0, 17ll, -UnsignedValue(value)) = iif(value = 0, 17ll, expected))
	VERIFY(calls = iif(value = 0, 2, 3))
	VERIFY((0 andalso (-UnsignedValue(value) < 0ll)) = 0)
	VERIFY(calls = iif(value = 0, 2, 3))
	'' The assignment optimizer may lower this into an in-place operation.
	value = -value
	VERIFY(clngint(value) = expected)
end sub

private sub CheckNative(byval value as uinteger, byval expected as integer)
	VERIFY(-value = expected)
	VERIFY((-value < 0) = (expected < 0))
	VERIFY((-value > 0) = (expected > 0))
	VERIFY((-value shr 1) = (expected shr 1))
	VERIFY(cdbl(-value) = cdbl(expected))
	VERIFY(cuint(-(-value)) = value)
	value = -value
	VERIFY(cint(value) = expected)
end sub

#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif

dim as ulongint values64(0 to 7) = { _
	0ull, 1ull, &h80000000ull, &hffffffffull, &h8000000000000000ull, _
	&h8000000000000001ull, &hffffffffffffffffull, &h7fffffffffffffffull }
dim as longint expected64(0 to 7) = { _
	0ll, -1ll, -2147483648ll, -4294967295ll, (-9223372036854775807ll - 1ll), _
	9223372036854775807ll, 1ll, -9223372036854775807ll }
for index_value as integer = 0 to 7
	Check64(values64(index_value), expected64(index_value))
next

const native_bits = sizeof(uinteger) * 8
dim as uinteger sign_bit = cuint(1) shl (native_bits - 1)
CheckNative(0, 0)
CheckNative(1, -1)
CheckNative(sign_bit, cint(sign_bit))
CheckNative(sign_bit + 1, cint(sign_bit - 1))
CheckNative(cuint(-1), 1)
CheckNative(cuint(-2), 2)

'' Narrow unsigned operands follow the dialect's native integer promotion.
dim as ubyte small_value = 255
dim as ushort medium_value = 65535
VERIFY(-small_value = -255)
VERIFY(-medium_value = -65535)
dim as ulong fixed_value = &hfffffffful
#ifdef __FB_64BIT__
	VERIFY(-fixed_value = -4294967295ll)
#else
	VERIFY(-fixed_value = 1l)
#endif
print "C_UNARY_RESULT_TYPES_PASS"

/' end of c-unary-result-types.bas '/
