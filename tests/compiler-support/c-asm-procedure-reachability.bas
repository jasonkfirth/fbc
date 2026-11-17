'' Project: FreeBASIC C backend regressions
'' File: c-asm-procedure-reachability.bas
'' Purpose: Prune private code beside fully exposed Intel assembly references.
'' Responsibilities: Exercise local ASM labels and preserve opaque fallbacks.
'' This file intentionally does NOT replace runtime assembly with BASIC code.
#lang "fb"

#if defined(OPAQUE_OPCODE) or defined(SEPARATE_ASM_LABEL)
	#define REACHABILITY_FALLBACK
#endif

#ifndef REACHABILITY_FALLBACK
extern "C"
	declare function fbc_c_asm_unavailable cdecl() as long
end extern
#endif

private function unreachableLeaf() as long
#ifdef REACHABILITY_FALLBACK
	return 91
#else
	return fbc_c_asm_unavailable()
#endif
end function

private function unreachableOwner() as long
	return unreachableLeaf()
end function

#ifdef SYSTEM_REGISTER_ASM
'' A dead ASM caller owns its private data and unavailable dependency.
dim shared inactiveAsmGlobal as long
private function inactiveGlobalLeaf() as long
	asm inc dword ptr [inactiveAsmGlobal]
	return unreachableLeaf()
end function

private function inactiveGlobalOwner() as long
	return inactiveGlobalLeaf()
end function

'' This initializer reaches the procedure before its ASM-only global is emitted.
dim shared initializerOnlyGlobal as long
private function initializerTarget cdecl() as long
	dim observed as long
	asm
		inc dword ptr [initializerOnlyGlobal]
		mov eax, dword ptr [initializerOnlyGlobal]
		mov dword ptr [observed], eax
	end asm
	if observed < 1 then return -1
	return 27
end function

dim shared initializerCallback as function cdecl() as long = @initializerTarget
#endif

'' The separate-template control observes its intervening BASIC statement.
dim shared annotationCount as long
dim shared remainingCount as long

private sub countDown()
#ifdef SEPARATE_ASM_LABEL
	asm
		#ifdef __FB_64BIT__
			lea rax, [rip + later_template]
		#else
			lea eax, [later_template]
		#endif
	end asm
	annotationCount += 1
#endif
	asm
		#ifdef SEPARATE_ASM_LABEL
			later_template:
		#endif
		mov eax, dword ptr [remainingCount]
		#ifdef OPAQUE_OPCODE
			'' BSWAP is valid, but outside the checked instruction subset.
			bswap eax
			bswap eax
		#endif
		count_again:
		dec eax
		jnz count_again
		mov dword ptr [remainingCount], eax
	end asm
end sub

#ifdef SYSTEM_REGISTER_ASM
'' The feature bit gates XGETBV. The x87 scope restores the complete caller
'' control word before BASIC expressions or the runtime can use floating point.
dim shared leafMaximum as ulong, leafFeatures as ulong, xcr0Low as ulong
dim shared controlBefore as ushort, controlExtended as ushort
dim shared controlObserved as ushort, controlRestored as ushort
asm
	#ifdef __FB_64BIT__
		push rbx
	#else
		push ebx
	#endif
	xor eax, eax
	cpuid
	mov dword ptr [leafMaximum], eax
	mov eax, 1
	cpuid
	mov dword ptr [leafFeatures], ecx
	#ifdef __FB_64BIT__
		pop rbx
	#else
		pop ebx
	#endif
	fnstcw [controlBefore]
	mov ax, [controlBefore]
	and ax, 0xfcff
	or ax, 0x0300
	mov [controlExtended], ax
	fldcw [controlExtended]
	fnstcw [controlObserved]
	fldcw [controlBefore]
	fnstcw [controlRestored]
end asm
if leafMaximum < 1 then end 4
if controlBefore <> controlRestored then end 5
if (controlObserved and &h0300) <> &h0300 then end 6
'' CPUID leaf 1 ECX bit 27 reports operating-system XSAVE support.
if (leafFeatures and (1ul shl 27)) <> 0 then
	asm
		xor ecx, ecx
		xgetbv
		mov dword ptr [xcr0Low], eax
	end asm
	'' Architectural XCR0 bit 0 always enables the x87 state component.
	if (xcr0Low and 1) = 0 then end 7
end if
#endif

remainingCount = 3
countDown()
if remainingCount <> 0 then end 1
#ifdef SEPARATE_ASM_LABEL
if annotationCount <> 1 then end 2
#else
if annotationCount <> 0 then end 3
#endif
#ifdef SYSTEM_REGISTER_ASM
if initializerCallback() <> 27 then end 8
#endif
print "ASM_PROCEDURE_REACHABILITY_PASS"

'' end of c-asm-procedure-reachability.bas
