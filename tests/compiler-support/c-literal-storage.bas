/'
	Project: FreeBASIC C backend regression tests
	File: c-literal-storage.bas
	Purpose: Check literal identity, persistent references and writable addresses.
	Responsibilities: Exercise narrow and wide backing data, global pointer
	initializers, embedded zero bytes, native calls and unused private bodies.
	This file intentionally does NOT contain: Native diagnostic suppression or
	assumptions about the target's wide character width.
'/
#lang "fb"
#include once "crt/stdio.bi"

extern "c"
	declare sub NativeWrite alias "literal_storage_write"(byval value as zstring ptr)
	declare function NativeClobber alias "literal_storage_clobber"() as ulong
end extern

dim shared as zstring ptr names(0 to 1) = {@"ascii", @"utf8"}
dim shared as any ptr observed_slot

#ifdef CHECK_OPAQUE
	private sub OpaqueBoundary()
		asm
			nop
		end asm
	end sub
	OpaqueBoundary()
#endif

private function FirstAddress() as any ptr
	return @"pool-identity"
end function

private function SecondAddress() as any ptr
	return @"pool-identity"
end function

private function NarrowReference() byref as zstring
	return *cast(zstring ptr, @"persistent narrow")
end function

private function WideReference() byref as wstring
	return wstr("persistent wide")
end function

private function CheckConstView(byval value as const zstring ptr) as boolean
	return value = cast(const zstring ptr, @"pool-identity")
end function

private sub ObserveSlot(byref value as zstring ptr)
	'' The caller's BYVAL override supplies the slot address directly. Observe
	'' its address without interpreting the literal bytes as a stored pointer.
	observed_slot = @value
end sub

private function UnusedAddress() as any ptr
	return @"unused-literal-storage"
end function

private function UnusedCaller() as any ptr
	return UnusedAddress()
end function

dim narrow_value as zstring ptr = @(NarrowReference())
dim wide_value as wstring ptr = @(WideReference())
if( NativeClobber() = 0 ) then end 1
if( *narrow_value <> "persistent narrow" ) then end 2
if( *wide_value <> wstr("persistent wide") ) then end 3
if( FirstAddress() <> SecondAddress() ) then end 4
if( CheckConstView(cast(const zstring ptr, FirstAddress())) = FALSE ) then end 5
if( *names(0) <> "ascii" or *names(1) <> "utf8" ) then end 6
ObserveSlot(byval @"slot bytes")
if( observed_slot <> cast(any ptr, @"slot bytes") ) then end 13

dim embedded as ubyte ptr = cast(ubyte ptr, @!"A\0B")
if( embedded[0] <> 65 or embedded[1] <> 0 or embedded[2] <> 66 or embedded[3] <> 0 ) then end 7
if( printf(!"literal=%s %d\n", cast(zstring ptr, FirstAddress()), clng(42)) < 0 ) then end 8

#ifdef CHECK_LITERAL_WRITES
	'' The native backend stores these bytes in .data. A C literal expression
	'' cannot provide the writable, persistent address used by the same ABI.
	NativeWrite(cast(zstring ptr, FirstAddress()))
	if( *cast(zstring ptr, SecondAddress()) <> "Pool-identity" ) then end 9
	(*wide_value)[0] = asc("P")
	if( WideReference() <> wstr("Persistent wide") ) then end 10
	embedded[2] = 67
	dim again as ubyte ptr = cast(ubyte ptr, @!"A\0B")
	if( again <> embedded or again[2] <> 67 ) then end 11
	NativeWrite(names(0))
	if( *names(0) <> "Ascii" ) then end 12
#endif

print "C_LITERAL_STORAGE_PASS"

/' end of c-literal-storage.bas '/
