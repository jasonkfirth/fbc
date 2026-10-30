/'
	Project: FreeBASIC C backend regression controls
	File: c-private-procedures.bas
	Purpose: Remove unreachable private bodies without losing ABI entry points.
	Responsibilities: Dead chains and cycles, read-only implementation declarations,
	live function pointers, global initializer roots and conservative storage roots.
	This file intentionally does NOT contain: Platform APIs or threading.
'/
#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_PRIVATE_PROCEDURES_FAILED", __LINE__
		end 1
	end if
#endmacro

private function UnreachableLeaf(byval value as long) as long
	return value + 1
end function

private function UnreachableChain(byval value as long) as long
	return UnreachableLeaf(value)
end function

private function UnreachableSelf(byval value as long) as long
	if value <= 0 then return 0
	return UnreachableSelf(value - 1)
end function

declare function UnreachableCycleB(byval value as long) as long
private function UnreachableCycleA(byval value as long) as long
	if value <= 0 then return 0
	return UnreachableCycleB(value - 1)
end function
private function UnreachableCycleB(byval value as long) as long
	if value <= 0 then return 0
	return UnreachableCycleA(value - 1)
end function

declare function UnreachableReader(byval text as zstring ptr) as long
private function UnreachableCaller() as long
	dim as zstring * 8 text = "unused"
	return UnreachableReader(@text)
end function
private function UnreachableReader(byval text as zstring ptr) as long
	return text[0]
end function

private function ReachableLeaf(byval value as long) as long
	return value + 1
end function
private function ReachableChain(byval value as long) as long
	return ReachableLeaf(value) + 1
end function

private function ReadableEntry(byval text as zstring ptr) as long
	return text[0]
end function

private function InitializerEntry(byval value as long) as long
	return value + 7
end function
dim shared as function(byval as long) as long initializer_callback = @InitializerEntry

private function KeepStatic() as long
	'' Hoisted storage still has a separate ownership pass. Retain this body
	'' until that storage and procedure share a conditional dependency graph.
	static as long value = 5
	if value < 0 then return KeepStatic()
	return value
end function

private function UnreachableRoot(byval value as long) as long
	'' Self-reference makes the frontend retain this body. Its only other
	'' references form a dead call chain, including a forward read-only call.
	if value < 0 then return UnreachableRoot(value + 1)
	return UnreachableChain(value) + UnreachableCaller()
end function

function PublicEntry cdecl alias "r90_public_entry"(byval value as long) as long
	'' An external C object calls this entry. No BASIC expression names it.
	return value + 11
end function
#ifdef CHECK_NATIVE_CALLER
declare function NativeProbe cdecl alias "r90_native_probe"() as long
	VERIFY(NativeProbe() = 42)
#endif

dim as zstring * 8 text = "AZ"
dim reader as function(byval as zstring ptr) as long = @ReadableEntry
VERIFY(ReachableChain(40) = 42)
VERIFY(ReadableEntry(@text) = asc("A"))
VERIFY(reader(@text) = asc("A"))
VERIFY(initializer_callback(35) = 42)
#ifdef CHECK_OPAQUE
asm
	nop
end asm
#endif
print "C_PRIVATE_PROCEDURES_PASS"

'' end of c-private-procedures.bas
