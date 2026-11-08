'' Project: FreeBASIC compiler regression tests
'' ---------------------------------------------
''
'' File: inline-asm-att-llvm.bas
''
'' Purpose:
''
''     Ensure the LLVM backend honors AT&T syntax for inline assembly.
''
'' Responsibilities:
''
''     - Compile and execute an equivalent no-op in either assembler dialect.
''
'' This file intentionally does NOT contain:
''
''     - instructions for other CPU families or external library calls.

'' AT&T and Intel register spellings belong to the x86 instruction set.
#if defined(__FB_X86__)
private sub test_register_instruction
#if __FB_ASM__ = "att"
	#if (__FB_BACKEND__ = "gcc") or (__FB_BACKEND__ = "clang")
	asm
		"movl %%eax, %%eax" : : : "eax"
	end asm
	#else
	asm
		movl %eax, %eax
	end asm
	#endif
#else
	asm
		mov eax, eax
	end asm
#endif
end sub

test_register_instruction()
print "PASS: inline assembly dialect"
#else
print "SKIP: x86 inline assembly dialect"
#endif

'' TEST_MODE : COMPILE_AND_RUN_OK

'' end of inline-asm-att-llvm.bas
