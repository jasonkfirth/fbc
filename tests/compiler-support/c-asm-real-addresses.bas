'' Project: FreeBASIC C backend regressions
'' File: c-asm-real-addresses.bas
'' Purpose: Keep actual storage and live parameter aliases across Intel assembly.
'' Responsibilities: Exercise many operands, repeated static updates and an
'' observer called before the assembly block returns, and floating argument bits.
'' This file contains no graphics code or native tool selection.
#lang "fb"

#if not defined(__FB_X86__) and not defined(__FB_X86_64__)
#error Intel assembly target required
#endif

declare sub asm_publish_home cdecl alias "asm_publish_home"(byval slot as long ptr, byval expected as long)
declare sub asm_observe_home cdecl alias "asm_observe_home"()
declare function asm_home_observed cdecl alias "asm_home_observed"() as long
declare sub asm_clear_home cdecl alias "asm_clear_home"()
declare function asm_call_raw_float cdecl alias "asm_call_raw_float"(byval bits as ulong) as ulong
declare function asm_call_raw_double cdecl alias "asm_call_raw_double"(byval bits as ulongint) as ulongint

function ReadFloatBits cdecl alias "asm_read_float"(byval value as single) as ulong
	static raw as ulong
	asm
		mov eax, [value]
		mov [raw], eax
	end asm
	return raw
end function

function ReadDoubleBits cdecl alias "asm_read_double"(byval value as double) as ulongint
	static low as ulong, high as ulong
	asm
#ifdef __FB_64BIT__
		lea rdx, [value]
		mov eax, [rdx]
		mov ecx, [rdx + 4]
#else
		lea edx, [value]
		mov eax, [edx]
		mov ecx, [edx + 4]
#endif
		mov [low], eax
		mov [high], ecx
	end asm
	return (culngint(high) shl 32) or low
end function

sub UnreachableAssembly cdecl alias "asm_unreachable_assembly"(byval value as long)
	'' The unreachable reference must not materialize an unused source home.
	return
	asm
		inc dword ptr [value]
	end asm
end sub

private function Exercise(byval value as long, byref aliased as long) as long
	static s00 as long = 100
	static s01 as long = 101
	static s02 as long = 102
	static s03 as long = 103
	static s04 as long = 104
	static s05 as long = 105
	static s06 as long = 106
	static s07 as long = 107
	static s08 as long = 108
	static s09 as long = 109
	static s10 as long = 110
	static s11 as long = 111
	static s12 as long = 112
	static s13 as long = 113
	static s14 as long = 114
	static s15 as long = 115
	static s16 as long = 116
	static s17 as long = 117
	static s18 as long = 118
	static s19 as long = 119
	static s20 as long = 120
	static s21 as long = 121
	static s22 as long = 122
	static s23 as long = 123
	dim automatic_value as long = 7
	asm_publish_home(@value, value + 5)
	asm
		add dword ptr [s00], 1
		add dword ptr [s01], 2
		add dword ptr [s02], 3
		add dword ptr [s03], 4
		add dword ptr [s04], 5
		add dword ptr [s05], 6
		add dword ptr [s06], 7
		add dword ptr [s07], 8
		add dword ptr [s08], 9
		add dword ptr [s09], 10
		add dword ptr [s10], 11
		add dword ptr [s11], 12
		add dword ptr [s12], 13
		add dword ptr [s13], 14
		add dword ptr [s14], 15
		add dword ptr [s15], 16
		add dword ptr [s16], 17
		add dword ptr [s17], 18
		add dword ptr [s18], 19
		add dword ptr [s19], 20
		add dword ptr [s20], 21
		add dword ptr [s21], 22
		add dword ptr [s22], 23
		add dword ptr [s23], 24
		add dword ptr [value], 5
		add dword ptr [automatic_value], 3
#ifdef __FB_64BIT__
		mov rax, [aliased]
		add dword ptr [rax], 9
		'' Windows x64 callees require the caller's 32-byte shadow area.
#ifdef __FB_WIN32__
		sub rsp, 32
#endif
		call asm_observe_home
#ifdef __FB_WIN32__
		add rsp, 32
#endif
#else
		mov eax, [aliased]
		add dword ptr [eax], 9
		call asm_observe_home
#endif
	end asm
	if asm_home_observed() <> 1 then end 3
	asm_clear_home()
	if automatic_value <> 10 then end 4
	return value + automatic_value + _
		s00 + s01 + s02 + s03 + s04 + s05 + _
		s06 + s07 + s08 + s09 + s10 + s11 + _
		s12 + s13 + s14 + s15 + s16 + s17 + _
		s18 + s19 + s20 + s21 + s22 + s23
end function

UnreachableAssembly(123)

dim value as long = 10
dim aliased as long = 0
if Exercise(value, aliased) <> 3001 then end 5
if aliased <> 9 or value <> 10 then end 6
if Exercise(20, aliased) <> 3311 then end 7
if aliased <> 18 then end 8

'' These are representations, not floating arithmetic: signaling and quiet
'' NaNs, negative zero and one must reach the assembly reader unchanged.
if asm_call_raw_float(&h7f812345ul) <> &h7f812345ul then end 9
if asm_call_raw_float(&h7fc12345ul) <> &h7fc12345ul then end 10
if asm_call_raw_float(&h80000000ul) <> &h80000000ul then end 11
if asm_call_raw_float(&h3f800000ul) <> &h3f800000ul then end 12
if asm_call_raw_double(&h7ff0000000001234ull) <> &h7ff0000000001234ull then end 13
if asm_call_raw_double(&h7ff8000000001234ull) <> &h7ff8000000001234ull then end 14
if asm_call_raw_double(&h8000000000000000ull) <> &h8000000000000000ull then end 15
if asm_call_raw_double(&h3ff0000000000000ull) <> &h3ff0000000000000ull then end 16
print "ASM_REAL_ADDRESSES_PASS"

'' end of c-asm-real-addresses.bas
