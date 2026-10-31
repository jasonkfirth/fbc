/'
	Project: FreeBASIC C backend regression controls
	File: c-fixed-string-bytes.bas
	Purpose: Preserve the exact storage of initialized fixed-length strings.
	Responsibilities: Check padding, truncation, embedded zeroes, byte values,
	arrays, writable static storage and adjacent fields across native backends.
	This file intentionally does NOT contain: Large allocation stress tests or
	changes to the shared literal pool. All mutable state is process-local.
'/
#lang "fb"

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_FIXED_STRING_BYTES_FAILED", __LINE__
		end 1
	end if
#endmacro

type GuardedString
	prefix as ubyte = &hA5
	payload as string * 5 = "aBc"
	suffix as ubyte = &h5A
end type

type GuardedLargeString
	prefix as ubyte = &h39
	payload as string * 8192 = "x"
	suffix as ubyte = &h93
end type

dim shared as GuardedString guarded
dim shared as GuardedLargeString large_guarded
dim shared as string * 1 minimum = "Q"
dim shared as string * 4 padded = "ab"
dim shared as string * 3 truncated = "abcdef"
dim shared as string * 4 empty_value = ""
dim shared as string * 5 binary_value = !"\&h80\&hFF\0Z\1"
dim shared as string * 5 punctuation = chr(34, 92, 63, 63, 47)
dim shared as string * 8192 large_fixed = "x"
dim shared matrix(0 to 2, 0 to 2) as string * 3 = { { "A", "BC" }, { "DEFG", "H" } }
dim shared as zstring * 4 terminated = "abcdef"
dim shared as zstring * 1 minimum_terminated = "Q"

'' Read the object representation through an unsigned byte pointer. This also
'' checks bytes beyond an embedded zero, where string comparison would stop.
sub CheckBytes(byval storage as const ubyte ptr, expected() as ubyte)
	for i as integer = lbound(expected) to ubound(expected)
		if storage[i - lbound(expected)] <> expected(i) then
			print "BYTE_MISMATCH", i, storage[i - lbound(expected)], expected(i)
		end if
		VERIFY(storage[i - lbound(expected)] = expected(i))
	next
end sub

function StaticAddress() as ubyte ptr
	static as string * 5 value = !"A\0B"
	return cptr(ubyte ptr, @value)
end function

function LiteralAddress() as const zstring ptr
	return @"shared literal identity"
end function

#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif

dim as ubyte expected_guarded(0 to 6) = { &hA5, 97, 66, 99, 32, 32, &h5A }
dim as ubyte expected_padded(0 to 3) = { 97, 98, 32, 32 }
dim as ubyte expected_truncated(0 to 2) = { 97, 98, 99 }
dim as ubyte expected_empty(0 to 3) = { 32, 32, 32, 32 }
dim as ubyte expected_binary(0 to 4) = { 128, 255, 0, 90, 1 }
dim as ubyte expected_punctuation(0 to 4) = { 34, 92, 63, 63, 47 }
dim as ubyte expected_matrix(0 to 26) = { _
	65, 32, 32, 66, 67, 32, 0, 0, 0, _
	68, 69, 70, 72, 32, 32, 0, 0, 0, _
	0, 0, 0, 0, 0, 0, 0, 0, 0 }
dim as ubyte expected_terminated(0 to 3) = { 97, 98, 99, 0 }
dim as ubyte expected_static(0 to 4) = { 65, 0, 66, 32, 32 }

VERIFY(sizeof(GuardedString) = 7)
VERIFY(sizeof(GuardedLargeString) = 8194)
VERIFY(sizeof(minimum) = 1)
VERIFY(minimum = "Q")
CheckBytes(cptr(const ubyte ptr, @guarded), expected_guarded())
CheckBytes(cptr(const ubyte ptr, @padded), expected_padded())
CheckBytes(cptr(const ubyte ptr, @truncated), expected_truncated())
CheckBytes(cptr(const ubyte ptr, @empty_value), expected_empty())
CheckBytes(cptr(const ubyte ptr, @binary_value), expected_binary())
CheckBytes(cptr(const ubyte ptr, @punctuation), expected_punctuation())
CheckBytes(cptr(const ubyte ptr, @matrix(0, 0)), expected_matrix())
CheckBytes(cptr(const ubyte ptr, @terminated), expected_terminated())
VERIFY(cptr(const ubyte ptr, @minimum_terminated)[0] = 0)

'' Static storage must keep its identity and remain writable after emission.
dim as ubyte ptr first_address = StaticAddress()
CheckBytes(first_address, expected_static())
first_address[4] = 255
VERIFY(StaticAddress() = first_address)
VERIFY(StaticAddress()[4] = 255)
VERIFY(LiteralAddress() = LiteralAddress())
VERIFY(*LiteralAddress() = "shared literal identity")

static as string * 5 local_value = !"\&h80\&hFF\0Z\1"
CheckBytes(cptr(const ubyte ptr, @local_value), expected_binary())
dim as string * 4 local_padded = "ab"
CheckBytes(cptr(const ubyte ptr, @local_padded), expected_padded())
'' Both records have byte alignment. The payload follows the one-byte prefix.
dim as ubyte ptr guarded_bytes = cptr(ubyte ptr, @guarded)
dim as ubyte ptr large_guarded_bytes = cptr(ubyte ptr, @large_guarded)
guarded_bytes[2] = 255
VERIFY(guarded.prefix = &hA5)
VERIFY(guarded.suffix = &h5A)
VERIFY(guarded_bytes[2] = 255)

'' Keep this bounded while exercising the initializer builder's growth path.
'' The sentinel fields prove that padding does not append a terminator.
VERIFY(large_guarded.prefix = &h39)
VERIFY(large_guarded.suffix = &h93)
VERIFY(large_guarded_bytes[1] = 120)
VERIFY(cptr(const ubyte ptr, @large_fixed)[0] = 120)
for i as integer = 1 to 8191
	VERIFY(large_guarded_bytes[i + 1] = 32)
	VERIFY(cptr(const ubyte ptr, @large_fixed)[i] = 32)
next
large_guarded_bytes[8192] = 255
VERIFY(large_guarded_bytes[8192] = 255)
VERIFY(large_guarded.suffix = &h93)

print "C_FIXED_STRING_BYTES_PASS"

/' end of c-fixed-string-bytes.bas '/
