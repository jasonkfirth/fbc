'' Project: FreeBASIC C backend regressions
'' File: c-asm-private-storage.bas
'' Purpose: Keep private objects observable through file-scope naked assembly.
'' Responsibilities: Exercise scalar, array, record and local-static storage,
'' ordinary BYREF updates and repeated native reads and writes.
#lang "fb"

#if not defined(__FB_X86__) and not defined(__FB_X86_64__)
#error Intel assembly target required
#endif

type StoragePair
	first_value as long
	second_value as long
end type

dim shared scalar_value as long = 7
dim shared sample_values(0 to 2) as long = {11, 13, 17}
dim shared pair_value as StoragePair = (19, 23)
dim shared combined_value as long
dim shared asm_only_value as long = 29
dim shared expected_calls as long
dim shared ordinary_value as long = 37

private sub change_storage naked cdecl alias "asm_change_private_storage"()
	type CounterState
		count_value as long
		reserved_value as long
	end type
	static static_counter as CounterState = (11, 0)
	asm
#ifdef __FB_64BIT__
		mov eax, dword ptr [rip + scalar_value]
		add eax, dword ptr [rip + sample_values + 4]
		add eax, dword ptr [rip + pair_value + 4]
		inc dword ptr [rip + asm_only_value]
		add eax, dword ptr [rip + asm_only_value]
		inc dword ptr [rip + static_counter]
		add eax, dword ptr [rip + static_counter]
		mov dword ptr [rip + combined_value], eax
		inc dword ptr [rip + scalar_value]
		add dword ptr [rip + sample_values + 4], 3
		add dword ptr [rip + pair_value + 4], 5
#else
#ifdef __FB_WIN32__
		'' Windows image relocations preserve direct 32-bit storage addresses.
		#define ASM_STORAGE_SLOT(symbol, displacement) [symbol + displacement]
#else
		'' The local PC thunk keeps these accesses valid in 32-bit PIE/PIC.
		call .Lfb_asm_private_storage_pc
		add edx, offset _GLOBAL_OFFSET_TABLE_
		#define ASM_STORAGE_SLOT(symbol, displacement) [edx + symbol@GOTOFF + displacement]
#endif
		mov eax, dword ptr ASM_STORAGE_SLOT(scalar_value, 0)
		add eax, dword ptr ASM_STORAGE_SLOT(sample_values, 4)
		add eax, dword ptr ASM_STORAGE_SLOT(pair_value, 4)
		inc dword ptr ASM_STORAGE_SLOT(asm_only_value, 0)
		add eax, dword ptr ASM_STORAGE_SLOT(asm_only_value, 0)
		inc dword ptr ASM_STORAGE_SLOT(static_counter, 0)
		add eax, dword ptr ASM_STORAGE_SLOT(static_counter, 0)
		mov dword ptr ASM_STORAGE_SLOT(combined_value, 0), eax
		inc dword ptr ASM_STORAGE_SLOT(scalar_value, 0)
		add dword ptr ASM_STORAGE_SLOT(sample_values, 4), 3
		add dword ptr ASM_STORAGE_SLOT(pair_value, 4), 5
		#undef ASM_STORAGE_SLOT
#endif
		ret
#if defined(__FB_X86__) and not defined(__FB_WIN32__)
		.Lfb_asm_private_storage_pc:
		mov edx, dword ptr [esp]
		ret
#endif
	end asm
end sub

private function other_local_type naked cdecl alias "asm_storage_other_type"() as long
	'' Distinct procedure-local types may share a BASIC name.
	type CounterState
		wide_value as longint
		result_value as long
	end type
	static other_counter as CounterState = (83, 47)
	asm
#ifdef __FB_64BIT__
		'' LONGINT occupies the first eight bytes of this local record.
		mov eax, dword ptr [rip + other_counter + 8]
#else
#ifdef __FB_WIN32__
		mov eax, dword ptr [other_counter + 8]
#else
		call .Lfb_asm_other_storage_pc
		add edx, offset _GLOBAL_OFFSET_TABLE_
		mov eax, dword ptr [edx + other_counter@GOTOFF + 8]
#endif
#endif
		ret
#if defined(__FB_X86__) and not defined(__FB_WIN32__)
		.Lfb_asm_other_storage_pc:
		mov edx, dword ptr [esp]
		ret
#endif
	end asm
end function

private sub set_values(byref scalar_slot as long, byref array_slot as long, byval seed as long)
	scalar_slot = seed
	array_slot = seed + 7
end sub

function asm_storage_ordinary cdecl alias "asm_storage_ordinary"() as long
	return ordinary_value
end function

function asm_storage_step cdecl alias "asm_storage_step"(byval seed as long) as long
	set_values(scalar_value, sample_values(1), seed)
	pair_value.second_value = seed + 9
	expected_calls += 1
	change_storage()
	if combined_value <> seed * 3 + 56 + expected_calls * 2 then return 1
	if scalar_value <> seed + 1 then return 2
	if sample_values(1) <> seed + 10 then return 3
	if pair_value.second_value <> seed + 14 then return 4
	if sample_values(0) <> 11 orElse sample_values(2) <> 17 then return 5
	if pair_value.first_value <> 19 then return 6
	if asm_storage_ordinary() <> 37 then return 7
	if other_local_type() <> 47 then return 8
	return 0
end function

#ifndef ASM_STORAGE_EXPORTS
for seed as long = -31 to 31
	dim observed as long = asm_storage_step(seed)
	if observed <> 0 then end observed
next
print "ASM_PRIVATE_STORAGE_PASS"
#endif

'' end of c-asm-private-storage.bas
