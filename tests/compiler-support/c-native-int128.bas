/'
	Project: FreeBASIC C backend regression controls
	File: c-native-int128.bas
	Purpose: Check native wide integer types without changing their ABI.
	Responsibilities: Static initializers, signed/unsigned calls and callbacks,
	aggregate layout, reserved aliases, and optional native C interoperability.
	This file intentionally does NOT contain: Platform APIs or filesystem work.
'/

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_NATIVE_INT128_FAILED", __LINE__
		end 1
	end if
#endmacro

type WideRecord
	prefix as long
	signed_value as integer<128>
	unsigned_value as uinteger<128>
	suffix as long
end type

dim shared as integer<128> negative_one = -1
dim shared as uinteger<128> all_bits
dim shared as uinteger<128> sign_bit = 1
dim shared as WideRecord record_value = (17, -1, &hffffffffffffffffull, 29)
dim shared as uinteger<128> values(0 to 2) = {0, &hffffffffffffffffull, 17}

all_bits = not all_bits
sign_bit shl= 127

function SignedBoundary cdecl alias "fbc_test_signed128"(byval value as integer<128>) as integer<128>
	return value + 1
end function

function UnsignedBoundary cdecl alias "fbc_test_unsigned128"(byval value as uinteger<128>) as uinteger<128>
	return not value
end function

#ifdef CHECK_TYPE_NAMES
function ReservedSigned cdecl alias "__int128_t"(byval value as integer<128>) as integer<128>
	return value
end function

function ReservedUnsigned cdecl alias "__uint128_t"(byval value as uinteger<128>) as uinteger<128>
	return value
end function
#endif

#ifdef CHECK_NATIVE_CALLER
declare function NativeCaller cdecl alias "fbc_test_native128_caller"() as long
dim as long native_status = NativeCaller()
if native_status <> 0 then
	print "C_NATIVE_INT128_INTEROP_FAILED", native_status
	end 1
end if
#endif

type SignedCallback as function cdecl(byval value as integer<128>) as integer<128>
type UnsignedCallback as function cdecl(byval value as uinteger<128>) as uinteger<128>
dim as SignedCallback signed_call = @SignedBoundary
dim as UnsignedCallback unsigned_call = @UnsignedBoundary
dim as integer<128> signed_value = -cast(integer<128>, cast(uinteger<128>, 1) shl 100)
dim as uinteger<128> unsigned_value = (cast(uinteger<128>, 1) shl 100) or 37
VERIFY(sizeof(integer<128>) = 16)
VERIFY(sizeof(uinteger<128>) = 16)
VERIFY(negative_one = -1)
VERIFY(all_bits + 1 = 0)
VERIFY((all_bits shr 64) = &hffffffffffffffffull)
VERIFY((sign_bit shr 127) = 1)
VERIFY(record_value.prefix = 17)
VERIFY(record_value.signed_value = -1)
VERIFY(record_value.unsigned_value = &hffffffffffffffffull)
VERIFY(record_value.suffix = 29)
VERIFY(values(0) = 0)
VERIFY(values(1) = &hffffffffffffffffull)
VERIFY(values(2) = 17)
VERIFY(SignedBoundary(signed_value) = signed_value + 1)
VERIFY(signed_call(signed_value) = signed_value + 1)
VERIFY(UnsignedBoundary(unsigned_value) = (unsigned_value xor all_bits))
VERIFY(unsigned_call(unsigned_value) = (unsigned_value xor all_bits))
#ifdef CHECK_TYPE_NAMES
VERIFY(ReservedSigned(signed_value) = signed_value)
VERIFY(ReservedUnsigned(unsigned_value) = unsigned_value)
#endif
print "C_NATIVE_INT128_PASS"

'' end of c-native-int128.bas
