/'
	Project: FreeBASIC C backend diagnostic controls
	File: c-private-readonly-unused.bas
	Purpose: Preserve a source unused-function diagnostic after body separation.
	Responsibilities: Keep a self-referenced private procedure in the emitted C
	without creating a real caller outside that procedure.
	This file intentionally does NOT contain: Calls into the unreachable body.
'/

dim shared as integer want_recursion

private function UnusedReader(byval text as zstring ptr) as string
	if want_recursion then return UnusedReader(0)
	return *text
end function

#ifdef CHECK_OPAQUE
asm
	nop
end asm
#endif
print "C_PRIVATE_READONLY_UNUSED_PASS"

'' end of c-private-readonly-unused.bas
