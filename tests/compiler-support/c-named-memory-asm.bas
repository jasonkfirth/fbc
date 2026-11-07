/'
	Project: FreeBASIC C backend tests
	File: c-named-memory-asm.bas
	Purpose: Retain explicit ASM storage while discarding an unread mnemonic name.
	Responsibilities: Exercise named local and BYVAL memory operands and their
	initial values, without treating arbitrary assembly text as complete.
	This file intentionally does NOT rely on warning suppression or stack offsets.
'/
#lang "fb"

#ifdef __FB_X86__
#if __FB_ASM__ = "intel"
'' Every memory operand is a resolved BASIC symbol. The ASM-only binding must
'' retain its initialization even though no ordinary BASIC expression reads it.
private function namedMemory(byval value as long) as long
	dim inc as long = 123
	dim asm_only as long = 7
	asm
		inc dword ptr [asm_only]
		dec dword ptr [asm_only]
		inc dword ptr [value]
	end asm
	return value
end function
if namedMemory(10) <> 11 then end 7
#endif
#endif

print "C_NAMED_MEMORY_ASM_PASS"

'' end of c-named-memory-asm.bas
