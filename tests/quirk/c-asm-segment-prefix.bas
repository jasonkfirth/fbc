' TEST_MODE : COMPILE_ONLY_OK

'' Project: FreeBASIC compiler regression tests
'' File: c-asm-segment-prefix.bas
'' Purpose: Check Intel segment operands in the GCC and Clang C backends.
'' Responsibilities: Cover register addresses, C memory operands and quoted data.
'' This file intentionally does NOT execute segment-dependent instructions.

#if defined(__FB_X86__) and ((__FB_BACKEND__ = "gcc") or (__FB_BACKEND__ = "clang"))
#cmdline "-asm intel"

sub test_asm_segment_operands()
	dim as ubyte byte_slot = 1
	asm
		mov al, [cs:16]
		mov al, [ds:16]
		mov al, [es:16]
		mov al, [fs:16]
		mov al, [gs:16]
		mov al, [ss:16]
		mov al, [ gS : 16 ]
		mov al, [	gS	:	16 ]
		mov al, [gs:byte_slot]
		mov [fS:byte_slot], al
		mov al, [ gs : byte_slot ]
		mov [	fs	:	byte_slot	], al
		mov al, [ byte_slot ]
		#ifdef __FB_64BIT__
			mov al, [gs:16+rbx]
		#else
			mov al, [gs:16+ebx]
		#endif
		'' GNU as rejects a register-first address after an in-bracket segment.
		'' The Clang conversion also accepts register-first source addresses.
		#if __FB_BACKEND__ = "clang"
			#ifdef __FB_64BIT__
				mov al, [gs:rbx+16]
			#else
				mov al, [gs:ebx+16]
			#endif
		#endif
	end asm
end sub

sub test_asm_segment_data()
	'' These are assembler bytes, so bracket text must not be rewritten.
	asm
		.ascii "[gs:16]"
		.ascii $"prefix \""[fs:20]\"" suffix\0"
	end asm
end sub

#endif

'' end of c-asm-segment-prefix.bas
