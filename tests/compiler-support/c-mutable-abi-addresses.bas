/'
	Project: FreeBASIC C backend regression controls
	File: c-mutable-abi-addresses.bas
	Purpose: Keep recovered const member addresses truthful at mutable ABI calls.
	Responsibilities: Generated assignment, pointer identity, single evaluation,
	checked initialization writes, packed storage and native C interoperability.
	This file intentionally does NOT contain: Display or threading tests.
'/
#lang "fb"

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_MUTABLE_ABI_ADDRESSES_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as integer assignment_calls, address_calls
dim shared as uinteger expected_address

#ifdef CHECK_NATIVE_C
	declare function NativeTouch cdecl alias "fb_c_mutable_touch" _
		(byval value as long ptr, byval expected as uinteger) as long
#endif

type BaseValue
	payload as long
	declare operator let(byref rhs as BaseValue)
end type

type Owner extends BaseValue
	'' A managed field requires a generated memberwise assignment. A trivial
	'' record may instead use a block copy without calling the base operator.
	tail as string
end type

operator BaseValue.let(byref rhs as BaseValue)
	VERIFY(cuint(@rhs) = expected_address)
	assignment_calls += 1
#ifdef CHECK_NATIVE_C
	payload = NativeTouch(@rhs.payload, expected_address)
#else
	payload = rhs.payload
	rhs.payload += 1
#endif
end operator

private function QualifiedAddress(byref value as Owner) as const Owner ptr
	address_calls += 1
	return @value
end function

private sub AssignThroughConstView()
	dim as Owner source_value, destination
	source_value.payload = 41
	source_value.tail = "tail73"
	expected_address = cuint(@source_value)
	destination = *QualifiedAddress(source_value)
	VERIFY(address_calls = 1 and assignment_calls = 1)
	VERIFY(destination.payload = 41 and source_value.payload = 42)
	VERIFY(destination.tail = "tail73")
	destination = *QualifiedAddress(source_value)
	VERIFY(address_calls = 2 and assignment_calls = 2)
	VERIFY(destination.payload = 42 and source_value.payload = 43)
end sub

type PackedValue field = 1
	guard as ubyte
	payload as long
end type

private function ConstResult() as const integer
	function = 163
end function

private sub CheckedInitialization()
	dim as const PackedValue source_value = (19, 12345)
	dim as PackedValue destination = source_value
	VERIFY(destination.guard = 19 and destination.payload = 12345)
	VERIFY(ConstResult() = 163)
	dim as integer choose_first = 0
	destination = iif(choose_first, type<PackedValue>(23, 67890), source_value)
	VERIFY(destination.guard = 19 and destination.payload = 12345)
end sub

#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif

AssignThroughConstView()
CheckedInitialization()
print "C_MUTABLE_ABI_ADDRESSES_PASS"

/' end of c-mutable-abi-addresses.bas '/
