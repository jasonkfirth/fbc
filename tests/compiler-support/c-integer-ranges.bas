/'
	Project: FreeBASIC C backend regression controls
	File: c-integer-ranges.bas
	Purpose: Check integer comparison folding at native type boundaries.
	Responsibilities: Widening, narrowing, signedness, booleans, branches, and
	evaluation of calls whose result storage becomes unnecessary.
	This file intentionally does NOT contain: Platform APIs or filesystem work.
'/

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_INTEGER_RANGES_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as integer call_count

private function CountedByte() as byte
	call_count += 1
	return -128
end function

private function UnneededStorage() as integer
	dim as byte value = CountedByte()
	return clngint(value) >= -128ll
end function

private function KeptTarget(byval value as byte, byval selector as integer) as integer
	if clngint(value) < -128ll then goto reached
	if selector <> 0 then goto reached
	return 0
reached:
	return value
end function

private sub CheckByte(byval value as byte, byval expected as integer)
	VERIFY(value = expected)
	VERIFY((clngint(value) >= -128ll) = -1)
	VERIFY((clngint(value) < -128ll) = 0)
	VERIFY((clngint(value) <= 127ll) = -1)
	VERIFY((clngint(value) > 127ll) = 0)
	VERIFY((clngint(value) = 128ll) = 0)
	VERIFY((clngint(value) <> 128ll) = -1)
	VERIFY((culngint(value) >= 0ull) = -1)
	VERIFY(clngint(culngint(value)) = expected)
	VERIFY(cbyte(clngint(value)) = expected)
	dim as boolean flag = clngint(value) >= -128ll
	VERIFY(flag = true)
	flag = not (clngint(value) <= 127ll)
	VERIFY(flag = false)
	if clngint(value) < -128ll then
		VERIFY(0)
	else
		VERIFY(value = expected)
	end if
end sub

private sub CheckUbyte(byval value as ubyte, byval expected as integer)
	VERIFY(value = expected)
	VERIFY((clngint(value) >= 0ll) = -1)
	VERIFY((clngint(value) <= 255ll) = -1)
	VERIFY((clngint(value) < 0ll) = 0)
	VERIFY((clngint(value) > 255ll) = 0)
	VERIFY((clngint(value) <> -1ll) = -1)
	VERIFY((clngint(value) = -1ll) = 0)
	VERIFY(cubyte(clngint(value)) = expected)
end sub

private sub CheckShort(byval value as short, byval expected as longint)
	VERIFY(value = expected)
	VERIFY((clngint(value) >= -32768ll) = -1)
	VERIFY((clngint(value) <= 32767ll) = -1)
	VERIFY((clngint(value) > 32767ll) = 0)
	VERIFY((clngint(value) < -32768ll) = 0)
end sub

private sub CheckUshort(byval value as ushort, byval expected as longint)
	VERIFY(value = expected)
	VERIFY((clngint(value) >= 0ll) = -1)
	VERIFY((clngint(value) <= 65535ll) = -1)
	VERIFY((clngint(value) > 65535ll) = 0)
	VERIFY((clngint(value) < 0ll) = 0)
end sub

private sub CheckLong(byval value as long, byval expected as longint)
	VERIFY(value = expected)
	VERIFY((clngint(value) >= -2147483648ll) = -1)
	VERIFY((clngint(value) <= 2147483647ll) = -1)
	VERIFY((clngint(value) < -2147483648ll) = 0)
	VERIFY((clngint(value) > 2147483647ll) = 0)
end sub

private sub CheckUlong(byval value as ulong, byval expected as ulongint)
	VERIFY(value = expected)
	VERIFY((culngint(value) >= 0ull) = -1)
	VERIFY((culngint(value) <= 4294967295ull) = -1)
	VERIFY((culngint(value) > 4294967295ull) = 0)
	VERIFY((clngint(value) < 0ll) = 0)
end sub

private sub CheckWide(byval signed_value as longint, byval unsigned_value as ulongint, byval expected_lower as integer)
	VERIFY((signed_value >= -9223372036854775808ll) = -1)
	VERIFY((signed_value <= 9223372036854775807ll) = -1)
	VERIFY((unsigned_value >= 0ull) = -1)
	VERIFY((unsigned_value <= 18446744073709551615ull) = -1)
	VERIFY((signed_value < culng(4294967295ull)) = expected_lower)
	VERIFY((unsigned_value > 0ull) = (unsigned_value <> 0ull))
end sub

for value as integer = -128 to 127
	CheckByte(cbyte(value), value)
next
for value as integer = 0 to 255
	CheckUbyte(cubyte(value), value)
next
CheckShort(-32768, -32768ll)
CheckShort(0, 0ll)
CheckShort(32767, 32767ll)
CheckUshort(0, 0ll)
CheckUshort(65535, 65535ll)
CheckLong(-2147483648ll, -2147483648ll)
CheckLong(0, 0ll)
CheckLong(2147483647ll, 2147483647ll)
CheckUlong(0, 0ull)
CheckUlong(4294967295ull, 4294967295ull)
CheckWide(-9223372036854775808ll, 0ull, -1)
CheckWide(-1ll, 9223372036854775808ull, -1)
CheckWide(0ll, 18446744073709551615ull, -1)
CheckWide(9223372036854775807ll, 4294967295ull, 0)
VERIFY(UnneededStorage() = -1)
VERIFY(call_count = 1)
VERIFY(KeptTarget(-128, 0) = 0)
VERIFY(KeptTarget(127, 1) = 127)
dim as byte stored_value = 37
dim as byte ptr storage = @stored_value
VERIFY(*storage = 37)
VERIFY(*storage <> -128)
print "C_INTEGER_RANGES_PASS"

'' end of c-integer-ranges.bas
