'' Project: FreeBASIC compiler backend regression tests
'' -----------------------------------------
''
'' File: backend-abi.bas
''
'' Purpose:
''
''     Check procedure aliases, addresses, and x86 inline assembly at run time.
''
'' Responsibilities:
''
''     - call distinct typed declarations sharing one C symbol
''     - verify procedure addresses and code following inline assembly
''
'' This file intentionally does NOT contain:
''
''     - external test libraries or LLVM inline assembly
''

extern "c"
	function compiler_support_identity cdecl( byval value as any ptr ) as any ptr
		return value
	end function

	declare function integer_identity cdecl alias "compiler_support_identity" _
		( byval value as integer ptr ) as integer ptr
	declare function double_identity cdecl alias "compiler_support_identity" _
		( byval value as double ptr ) as double ptr
end extern

dim as integer i = 123
dim as double d = 4.5
if( integer_identity(@i) <> @i ) then end 1
if( double_identity(@d) <> @d ) then end 2
dim as function cdecl( byval as integer ptr ) as integer ptr fn = @integer_identity
if( fn(@i) <> @i ) then end 3

#if (__FB_BACKEND__ = "gcc") or (__FB_BACKEND__ = "clang")
	#ifdef __FB_X86__
		dim as long counter = 0
		asm inc Dword Ptr [counter]
		if( counter <> 1 ) then end 4
		dim as any ptr address
		asm
			#ifdef __FB_64BIT__
				lea rax, offset compiler_support_identity[rip]
				mov [address], rax
			#else
				mov eax, offset compiler_support_identity
				mov [address], eax
			#endif
		end asm
		if( address <> @compiler_support_identity ) then end 5
	#endif
#endif

print "backend ABI passed"

'' end of backend-abi.bas
