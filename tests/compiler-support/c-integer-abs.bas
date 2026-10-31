/'
	Project: FreeBASIC C backend regression controls
	File: c-integer-abs.bas
	Purpose: Verify integer ABS at signed minima and unsigned high-bit values.
	Responsibilities: Explicit expected bit patterns, single evaluation, promoted
	narrow operands and unchanged floating ABS behavior.
	This file intentionally does NOT contain: Platform APIs or 128-bit ABI probes.
'/
#lang "fb"

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_INTEGER_ABS_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as integer calls

private function SignedValue(byval value as longint) as longint
	calls += 1
	return value
end function

private function UnsignedValue(byval value as ulongint) as ulongint
	calls += 1
	return value
end function

private function FloatingValue(byval value as double) as double
	calls += 1
	return value
end function

private sub IntegerValues()
	dim as longint signed_values(0 to 5) = { _
		(-9223372036854775807ll - 1ll), -9223372036854775807ll, -1ll, 0ll, 1ll, 9223372036854775807ll }
	dim as longint signed_expected(0 to 5) = { _
		(-9223372036854775807ll - 1ll), 9223372036854775807ll, 1ll, 0ll, 1ll, 9223372036854775807ll }
	dim as ulongint unsigned_values(0 to 5) = { _
		&h8000000000000000ull, &h8000000000000001ull, &hffffffffffffffffull, 0ull, 1ull, &h7fffffffffffffffull }
	dim as ulongint unsigned_expected(0 to 5) = { _
		&h8000000000000000ull, &h7fffffffffffffffull, 1ull, 0ull, 1ull, &h7fffffffffffffffull }
	for i as integer = 0 to 5
		VERIFY(abs(SignedValue(signed_values(i))) = signed_expected(i))
		VERIFY(abs(UnsignedValue(unsigned_values(i))) = unsigned_expected(i))
	next
	VERIFY(calls = 12)
	'' ABS returns a value even when the minimum's bit pattern is unchanged.
	VERIFY(abs(SignedValue(signed_values(0))) < 0)
	VERIFY(abs(SignedValue(signed_values(1))) > 0)
	VERIFY(calls = 14)
end sub

private sub PromotedValues()
	dim as byte b = -128
	dim as short s = -32768
	dim as ubyte ub = 255
	dim as ushort us = 65535
	VERIFY(abs(b) = 128)
	VERIFY(abs(s) = 32768)
	VERIFY(abs(ub) = 255)
	VERIFY(abs(us) = 65535)
	dim as long value = (-2147483647l - 1l)
#ifdef __FB_64BIT__
	VERIFY(abs(value) = 2147483648ll)
#else
	VERIFY(abs(value) = (-2147483647l - 1l))
#endif
end sub

union DoubleBits
	value as double
	bits as ulongint
end union

private sub FloatingValues()
	dim as DoubleBits source_value, result
	source_value.bits = &h8000000000000000ull
	result.value = abs(FloatingValue(source_value.value))
	VERIFY(result.bits = 0)
	source_value.bits = &hfff8000000000042ull
	result.value = abs(FloatingValue(source_value.value))
	VERIFY(result.bits = &h7ff8000000000042ull)
	VERIFY(abs(FloatingValue(-1.5)) = 1.5)
	VERIFY(calls = 17)
end sub

#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif

IntegerValues()
PromotedValues()
FloatingValues()
print "C_INTEGER_ABS_PASS"

/' end of c-integer-abs.bas '/
