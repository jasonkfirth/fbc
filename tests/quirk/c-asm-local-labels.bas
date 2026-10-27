' TEST_MODE : COMPILE_AND_RUN_OK

'' Project: FreeBASIC C assembly qualification
'' File: c-asm-local-labels.bas
'' Purpose: Exercise branch, operand and literal ownership in copied ASM regions.
'' Responsibilities: Check actual writes, repeated calls, local data and LEA.
'' This file intentionally does NOT depend on application libraries.
#if defined(__FB_X86__) and defined(__FB_64BIT__) and ((__FB_BACKEND__ = "gcc") or (__FB_BACKEND__ = "clang"))
#cmdline "-O 2 -asm intel"

private sub count_steps( byval destination as long ptr )
	asm
		mov rax, [destination]
		xor ecx, ecx
		jmp .Llocal_entry
		.Llocal_dead:
		mov ecx, -1
		.Llocal_entry:
		inc ecx
		cmp ecx, 40
		jne .Llocal_entry
		mov dword ptr [rax], ecx
	end asm
end sub

private function many_refs( ) as long
	dim as long total = 0
	asm
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
		inc dword ptr [total]
	end asm
	return total
end function

private function quoted_data_and_lea( ) as long
	dim as long value = 0
	asm
		jmp .Lafter_blob
		.Lblob:
		.ascii "literal .Lblob: %0 %l1"
		.Lafter_blob:
		xor eax, eax
		lea rax, offset [value]
		mov dword ptr [rax], 23
	end asm
	return value
end function

dim as long first_value = 0, second_value = 0
count_steps( @first_value )
count_steps( @second_value )
if first_value <> 40 or second_value <> 40 then end 1
if many_refs( ) <> 40 then end 2
if many_refs( ) <> 40 then end 3
if quoted_data_and_lea( ) <> 23 then end 4
if quoted_data_and_lea( ) <> 23 then end 5

'' Writes preceding a jump to a BASIC label keep output constraints.
dim as long goto_value = 1
goto perform_asm
completed_asm:
if goto_value <> 42 then end 6
goto after_asm_test
perform_asm:
asm
	mov dword ptr [goto_value], 41
	inc dword ptr [goto_value]
	jmp completed_asm
end asm
end 7
after_asm_test:

#endif

end 0

'' end of c-asm-local-labels.bas
