'' Project: FreeBASIC compiler backend regressions
'' File: c-asm-inactive-procedures.bas
'' Purpose: Keep inactive ASM references from retaining optimized-away code.
'' Responsibilities: Check parsed-only and emitted-but-unreachable ASM users.
'' This file intentionally does NOT provide the unused external dependency.
#lang "fb"

declare sub unavailable cdecl alias "fb_missing_inactive_asm_dependency"()

#ifdef READONLY_INACTIVE_TARGET
private function conditionalTarget(byref enabled as const long) as long
#else
private function conditionalTarget(byval enabled as long) as long
#endif
	if enabled <> 0 then unavailable()
	return 19
end function

private sub inactiveAsm()
	asm
#ifdef __FB_64BIT__
		lea rax, [rip + conditionalTarget]
#else
		mov eax, offset conditionalTarget
#endif
	end asm
end sub

#ifdef EMITTED_INACTIVE_ASM
'' This call makes inactiveAsm an AST-accessed body, but its caller has no
'' live entry. Neither reference should keep the target's general native body.
private sub inactiveOuter()
	inactiveAsm()
end sub
#endif

dim observed as long = 1
asm
	inc dword ptr [observed]
end asm
if observed <> 2 then end 1
'' Native O2 can inline this real call and discard the unavailable branch.
if conditionalTarget(0) <> 19 then end 2
print "ASM_INACTIVE_PROCEDURES_PASS"

'' end of c-asm-inactive-procedures.bas
