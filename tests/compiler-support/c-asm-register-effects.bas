'' Project: FreeBASIC C backend regressions
'' File: c-asm-register-effects.bas
'' Purpose: Observe partial-register, implicit-register and stack-local effects.
'' Responsibilities: Keep native register clobbers and automatic storage valid
'' across checked Intel assembly. Native compiler selection belongs to the runner.
#lang "fb"

#if not defined(__FB_X86__) and not defined(__FB_X86_64__)
#error Intel assembly target required
#endif

function asm_effect_aliases cdecl alias "asm_effect_aliases"(byval seed as long) as long
	dim observed as long
	asm
		mov eax, 0x12340000
		mov ax, word ptr [seed]
		mov ah, 0x5a
		mov dword ptr [observed], eax
	end asm
	return observed
end function

function asm_effect_implicit cdecl alias "asm_effect_implicit"(byval seed as long) as long
	static factor as long = 7
	dim guard as long = seed + 19
	dim observed as long
	asm
		mov eax, dword ptr [seed]
		imul dword ptr [factor]
		mov dword ptr [observed], eax
	end asm
	'' The one-operand IMUL also writes EDX, even though it is unnamed above.
	return observed + guard
end function

function asm_effect_stack cdecl alias "asm_effect_stack"(byval seed as long) as long
	dim observed as long = seed + 7
	asm
#ifdef __FB_64BIT__
		push rbx
#else
		'' Automatic operands need a stable frame while ESP changes.
		push ebx
#endif
		mov eax, dword ptr [observed]
		add eax, dword ptr [seed]
		mov dword ptr [observed], eax
#ifdef __FB_64BIT__
		pop rbx
#else
		pop ebx
#endif
	end asm
	return observed
end function

function asm_effect_spill cdecl alias "asm_effect_spill"(byval seed as long) as long
	dim observed as long = seed + 7
	asm
#ifdef __FB_64BIT__
		push rbx
#else
		push ebx
#endif
		xor eax, eax
		'' An opaque instruction still needs protection for native spills.
		bswap eax
#ifdef __FB_64BIT__
		pop rbx
#else
		pop ebx
#endif
	end asm
	return observed
end function

function asm_effect_branch cdecl alias "asm_effect_branch"(byval seed as long) as long
	dim observed as long = seed + 7
	'' Define the BASIC destination before the assembler resolves its name.
	goto assembly_body
value_ready:
	return observed
assembly_body:
	asm
#ifdef __FB_64BIT__
		push rbx
#endif
		mov eax, dword ptr [observed]
		add eax, dword ptr [seed]
		mov dword ptr [observed], eax
#ifdef __FB_64BIT__
		pop rbx
#endif
		jmp value_ready
	end asm
	return -999
end function

#ifndef ASM_EFFECT_EXPORTS
for seed as long = -31 to 31
	if asm_effect_aliases(seed) <> (&h12345a00 or (seed and 255)) then end 1
	if asm_effect_implicit(seed) <> seed * 8 + 19 then end 2
	if asm_effect_stack(seed) <> seed * 2 + 7 then end 3
	if asm_effect_spill(seed) <> seed + 7 then end 4
	if asm_effect_branch(seed) <> seed * 2 + 7 then end 5
next
print "ASM_REGISTER_EFFECTS_PASS"
#endif

'' end of c-asm-register-effects.bas
