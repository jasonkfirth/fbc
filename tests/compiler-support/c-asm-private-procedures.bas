'' Project: FreeBASIC compiler backend regressions
'' File: c-asm-private-procedures.bas
'' Purpose: Retain private procedure bodies and their native ABI for ASM users.
'' Responsibilities: Check ordinary calls, forward targets and naked tail calls.
'' This file intentionally does NOT call the private targets from BASIC.
#lang "fb"

#if not defined(__FB_X86__) and not defined(__FB_X86_64__)
#error Intel assembly target required
#endif

private declare function forwardValue cdecl alias "fb_asm_only_forward"() as long

private function scalarValue cdecl alias "fb_asm_only_scalar"() as long
	return 47
end function

private function readonlyValue alias "fb_asm_only_readonly"(byref value as const long) as long
	return value + 5
end function

'' The tail calls use the caller's real return address and argument locations.
'' Neither target has a BASIC call or procedure-address expression to retain it.
private function callForward naked cdecl() as long
	asm
		jmp forwardValue
	end asm
end function

private function callReadonly naked(byref value as const long) as long
	asm
		jmp readonlyValue
	end asm
end function

private function forwardValue cdecl alias "fb_asm_only_forward"() as long
	return 53
end function

dim observed as long
asm
#if defined(__FB_X86_64__) and defined(__FB_WIN32__)
	'' Windows x64 requires a 32-byte shadow area for the called procedure.
	sub rsp, 32
#endif
	call scalarValue
#if defined(__FB_X86_64__) and defined(__FB_WIN32__)
	add rsp, 32
#endif
	mov [observed], eax
end asm
if observed <> 47 then end 1
if callForward() <> 53 then end 2
dim value as long = 71
if callReadonly(value) <> 76 then end 3
if value <> 71 then end 4
print "ASM_PRIVATE_PROCEDURES_PASS"

'' end of c-asm-private-procedures.bas
