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
''     - GCC extended-assembly constraints or SQLite implementation code.

private sub test_register_instruction
#if __FB_ASM__ = "att"
	asm
		movl %eax, %eax
	end asm
#else
	asm
		mov eax, eax
	end asm
#endif
end sub

test_register_instruction()
print "PASS: inline assembly dialect"

'' TEST_MODE : COMPILE_AND_RUN_OK

'' end of inline-asm-att-llvm.bas
