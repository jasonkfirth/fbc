' TEST_MODE : COMPILE_AND_RUN_OK

'' Project: FreeBASIC compiler regression tests
'' -----------------------------------------
''
'' File: inline-asm-registers.bas
''
'' Purpose:
''
''     Check x86 register values and balanced stack operations in inline asm.
''
'' Responsibilities:
''
''     - preserve values between consecutive assembly instructions
''     - store function results without reusing the source register for an address
''     - restore the original flags after a harmless CPUID capability check
''
'' This file intentionally does NOT contain:
''
''     - CPU-specific feature requirements or arbitrary privileged flag changes
''

#ifdef __FB_X86__

'' The fixture uses Intel operands, including on Darwin where AT&T is default.
#cmdline "-asm intel"

function read_flags() as uinteger
	asm
		#ifdef __FB_64BIT__
			pushfq
			pop rax
			mov [function], rax
		#else
			pushfd
			pop eax
			mov [function], eax
		#endif
	end asm
end function

sub write_flags( byval value as uinteger )
	asm
		#ifdef __FB_64BIT__
			mov rax, [value]
			push rax
			popfq
		#else
			mov eax, [value]
			push eax
			popfd
		#endif
	end asm
end sub

function assembly_result( byval value as uinteger ) as uinteger
	'' Separate single-line ASM statements are also one instruction sequence.
	#ifdef __FB_64BIT__
		asm mov rax, [value]
		asm add rax, 7
		asm mov [function], rax
	#else
		asm mov eax, [value]
		asm add eax, 7
		asm mov [function], eax
	#endif
end function

if( assembly_result(123) <> 130 ) then end 1
dim as uinteger original = read_flags()
'' EFLAGS bit 1 is fixed at one. TF and AC must not be introduced by emission.
const unsafe_flags = (1 shl 8) or (1 shl 18)
if( (original and 2) = 0 ) then end 2
if( (original and unsafe_flags) <> 0 ) then end 3
'' ID is the CPUID availability bit. It is safe to toggle in user mode; older
'' x86 CPUs may ignore it. Restore the original value before other tests.
const id_flag = 1 shl 21
write_flags( original xor id_flag )
dim as uinteger changed = read_flags()
write_flags( original )
if( (changed and unsafe_flags) <> 0 ) then end 4
if( ((read_flags() xor original) and id_flag) <> 0 ) then end 5

dim as long counter = 0
asm inc Dword Ptr [counter]
if( counter <> 1 ) then end 6

#endif

'' end of inline-asm-registers.bas
