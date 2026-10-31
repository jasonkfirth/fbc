/'
	Project: FreeBASIC C backend regression controls
	File: c-libc-atexit.bas
	Purpose: Verify libc callbacks and compiler-generated static destruction.
	Responsibilities: Direct and opaque callback addresses, single evaluation,
	LIFO exit order and one destructor registration for a static object.
	This file intentionally does NOT contain: Replacement libc entry points or
	execution of invalid callbacks. All observations occur on the exit thread.
'/
#lang "fb"

extern "C"
	declare function atexit(byval callback as any ptr) as long
	declare function AliasedExit alias "atexit"(byval callback as any ptr) as long
	declare sub ExitImmediately alias "_Exit"(byval status as long)
end extern

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_LIBC_ATEXIT_FAILED", __LINE__
		ExitImmediately(1)
	end if
#endmacro

dim shared as long events(0 to 3), event_count
dim shared as long constructor_count, destructor_count, opaque_count

private sub RecordEvent(byval identifier as long)
	VERIFY(event_count >= 0 andalso event_count < 4)
	events(event_count) = identifier
	event_count += 1
end sub

private sub FirstCallback cdecl()
	RecordEvent(1)
end sub

private sub SecondCallback cdecl()
	RecordEvent(2)
end sub

private function OpaqueCallback() as any ptr
	opaque_count += 1
	return procptr(SecondCallback)
end function

private function RegisterOpaque(byval callback as any ptr) as long
	return atexit(callback)
end function

type StaticObject
	value as long
	declare constructor()
	declare destructor()
end type

constructor StaticObject()
	constructor_count += 1
	value = 0
end constructor

destructor StaticObject()
	VERIFY(value = 2)
	destructor_count += 1
	RecordEvent(3)
end destructor

private sub TouchStatic()
	static item as StaticObject
	item.value += 1
end sub

private sub VerifyExit cdecl()
	VERIFY(event_count = 4)
	VERIFY(events(0) = 1 andalso events(1) = 3)
	VERIFY(events(2) = 2 andalso events(3) = 1)
	VERIFY(constructor_count = 1 andalso destructor_count = 1)
	VERIFY(opaque_count = 1)
	print "C_LIBC_ATEXIT_PASS"
end sub

#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif

'' Register the verifier first so it observes every later callback at exit.
VERIFY(atexit(procptr(VerifyExit)) = 0)
VERIFY(atexit(procptr(FirstCallback)) = 0)
VERIFY(RegisterOpaque(OpaqueCallback()) = 0)
TouchStatic()
dim as sub cdecl() typed_callback = procptr(FirstCallback)
VERIFY(AliasedExit(typed_callback) = 0)
TouchStatic()
print "C_LIBC_ATEXIT_ARMED"

/' end of c-libc-atexit.bas '/
