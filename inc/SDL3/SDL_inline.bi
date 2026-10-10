'' Project: FreeBASIC SDL3 bindings
'' File: SDL_inline.bi
'' Purpose: Implement the helpers that SDL supplies as C macros or inline functions.
'' Responsibilities: Match byte order, bit operations, rectangles, and assertions.
'' This file intentionally does NOT contain: SDL runtime or platform implementations.
''
'' These are translations of SDL3's zlib-licensed headers. The complete upstream
'' notice is preserved in SDL.bi. This is an altered source version.

#pragma once

'' Include the public types when this helper header is included directly.
'' SDL.bi includes this file after those types; pragma once breaks the cycle.
#include once "SDL.bi"

'' -------------------------------------------------------------------------
'' Byte order and bit operations
'' -------------------------------------------------------------------------

#ifdef __FB_BIGENDIAN__
	#define SDL_BYTEORDER SDL_BIG_ENDIAN
#else
	#define SDL_BYTEORDER SDL_LIL_ENDIAN
#endif
#define SDL_FLOATWORDORDER SDL_BYTEORDER

private function SDL_Swap16 cdecl(byval value as Uint16) as Uint16
	return (value shl 8) or (value shr 8)
end function

private function SDL_Swap32 cdecl(byval value as Uint32) as Uint32
	return ((value and &h000000FFu) shl 24) or _
	       ((value and &h0000FF00u) shl 8) or _
	       ((value and &h00FF0000u) shr 8) or _
	       ((value and &hFF000000u) shr 24)
end function

private function SDL_Swap64 cdecl(byval value as Uint64) as Uint64
	dim as Uint32 low = value and &hFFFFFFFFull
	dim as Uint32 high = value shr 32
	return (culngint(SDL_Swap32(low)) shl 32) or SDL_Swap32(high)
end function

private function SDL_SwapFloat cdecl(byval value as single) as single
	union SDL3_FloatBits
		value as single
		bits as Uint32
	end union
	dim converted as SDL3_FloatBits
	converted.value = value
	converted.bits = SDL_Swap32(converted.bits)
	return converted.value
end function

#ifdef __FB_BIGENDIAN__
	#define SDL_Swap16LE(value) SDL_Swap16(value)
	#define SDL_Swap32LE(value) SDL_Swap32(value)
	#define SDL_Swap64LE(value) SDL_Swap64(value)
	#define SDL_SwapFloatLE(value) SDL_SwapFloat(value)
	#define SDL_Swap16BE(value) (value)
	#define SDL_Swap32BE(value) (value)
	#define SDL_Swap64BE(value) (value)
	#define SDL_SwapFloatBE(value) (value)
#else
	#define SDL_Swap16LE(value) (value)
	#define SDL_Swap32LE(value) (value)
	#define SDL_Swap64LE(value) (value)
	#define SDL_SwapFloatLE(value) (value)
	#define SDL_Swap16BE(value) SDL_Swap16(value)
	#define SDL_Swap32BE(value) SDL_Swap32(value)
	#define SDL_Swap64BE(value) SDL_Swap64(value)
	#define SDL_SwapFloatBE(value) SDL_SwapFloat(value)
#endif

private function SDL_MostSignificantBitIndex32 cdecl(byval value as Uint32) as long
	dim result as long = -1
	do while value <> 0
		value shr= 1
		result += 1
	loop
	return result
end function

private function SDL_HasExactlyOneBitSet32 cdecl(byval value as Uint32) as boolean
	return (value <> 0) andalso ((value and (value - 1)) = 0)
end function

'' SDL exposes full barriers as functions for compilers without its assembly
'' intrinsics. Use those functions for all backends, including GCC and LLVM.
#define SDL_MemoryBarrierRelease() SDL_MemoryBarrierReleaseFunction()
#define SDL_MemoryBarrierAcquire() SDL_MemoryBarrierAcquireFunction()
#macro SDL_CompilerBarrier()
	SDL_MemoryBarrierReleaseFunction()
	SDL_MemoryBarrierAcquireFunction()
#endmacro
'' Match SDL's spin-loop hints on the supported processor families. They do
'' not replace the acquire/release barriers needed for shared data.
#ifdef __FB_X86__
	#macro SDL_CPUPauseInstruction()
		asm
			pause
		end asm
	#endmacro
#elseif defined(__FB_ARM__) and (defined(__FB_64BIT__) or (__FB_ARCH__ = "armv7-a") or (__FB_ARCH__ = "armv7-a+fp"))
	#macro SDL_CPUPauseInstruction()
		asm
			yield
		end asm
	#endmacro
#elseif defined(__FB_PPC__)
	#macro SDL_CPUPauseInstruction()
		asm
			or 27, 27, 27
		end asm
	#endmacro
#else
	'' Older ARM targets and other unsupported CPUs use SDL's no-op fallback.
	#define SDL_CPUPauseInstruction()
#endif

'' -------------------------------------------------------------------------
'' Size arithmetic and C macro equivalents
'' -------------------------------------------------------------------------

'' BASIC's SizeOf(array) describes one element. C's sizeof(array) describes
'' every element. These array helpers use bounds explicitly and accept the
'' one-dimensional arrays used by SDL's public data structures.
#macro SDL_zeroa(array_value)
	scope
		dim count as integer = ubound(array_value) - lbound(array_value) + 1
		if count > 0 then
			SDL_memset(@array_value(lbound(array_value)), 0, cuint(count) * sizeof(array_value(lbound(array_value))))
		end if
	end scope
#endmacro

private function SDL_size_mul_check_overflow cdecl(byval a as uinteger, byval b as uinteger, byval result as uinteger ptr) as boolean
	if result = 0 then return false
	if (a <> 0) andalso (b > (SDL_SIZE_MAX \ a)) then return false
	*result = a * b
	return true
end function

private function SDL_size_add_check_overflow cdecl(byval a as uinteger, byval b as uinteger, byval result as uinteger ptr) as boolean
	if result = 0 then return false
	if b > (SDL_SIZE_MAX - a) then return false
	*result = a + b
	return true
end function

#macro SDL_COMPILE_TIME_ASSERT(name, expression)
	#assert expression
#endmacro
#macro SDL_copyp(destination, source)
	SDL_COMPILE_TIME_ASSERT(SDL_copyp, sizeof(*(destination)) = sizeof(*(source)))
	SDL_memcpy((destination), (source), sizeof(*(source)))
#endmacro
#define SDL_DEFINE_AUDIO_FORMAT(is_signed, bigendian, flt, bits) ((cushort(is_signed) shl 15) or (cushort(bigendian) shl 12) or (cushort(flt) shl 8) or ((bits) and SDL_AUDIO_MASK_BITSIZE))
#define SDL_iconv_utf8_ucs2(value) cptr(Uint16 ptr, SDL_iconv_string("UCS-2", "UTF-8", (value), SDL_strlen(value) + 1))
#define SDL_iconv_utf8_ucs4(value) cptr(Uint32 ptr, SDL_iconv_string("UCS-4", "UTF-8", (value), SDL_strlen(value) + 1))
#define SDL_iconv_wchar_utf8(value) SDL_iconv_string("UTF-8", "WCHAR_T", cptr(const zstring ptr, (value)), (SDL_wcslen(value) + 1) * sizeof(wstring))

'' -------------------------------------------------------------------------
'' Rectangle helpers
'' -------------------------------------------------------------------------

private sub SDL_RectToFRect cdecl(byval rectangle as const SDL_Rect ptr, byval converted as SDL_FRect ptr)
	if rectangle = 0 orelse converted = 0 then exit sub
	converted->x = rectangle->x
	converted->y = rectangle->y
	converted->w = rectangle->w
	converted->h = rectangle->h
end sub

private function SDL_PointInRect cdecl(byval p as const SDL_Point ptr, byval rectangle as const SDL_Rect ptr) as boolean
	if p = 0 orelse rectangle = 0 then return false
	'' Widen before adding so the helper cannot overflow a signed C int.
	return (p->x >= rectangle->x) andalso _
	       (clngint(p->x) < clngint(rectangle->x) + rectangle->w) andalso _
	       (p->y >= rectangle->y) andalso _
	       (clngint(p->y) < clngint(rectangle->y) + rectangle->h)
end function

private function SDL_RectEmpty cdecl(byval rectangle as const SDL_Rect ptr) as boolean
	return (rectangle = 0) orelse (rectangle->w <= 0) orelse (rectangle->h <= 0)
end function

private function SDL_RectsEqual cdecl(byval a as const SDL_Rect ptr, byval b as const SDL_Rect ptr) as boolean
	if a = 0 orelse b = 0 then return false
	return (a->x = b->x) andalso (a->y = b->y) andalso (a->w = b->w) andalso (a->h = b->h)
end function

'' SDL's floating rectangle API includes the right/bottom boundary and treats
'' a zero-sized rectangle as nonempty. The integer API excludes those edges.
private function SDL_PointInRectFloat cdecl(byval p as const SDL_FPoint ptr, byval rectangle as const SDL_FRect ptr) as boolean
	if p = 0 orelse rectangle = 0 then return false
	return (p->x >= rectangle->x) andalso (p->x <= rectangle->x + rectangle->w) andalso _
	       (p->y >= rectangle->y) andalso (p->y <= rectangle->y + rectangle->h)
end function

private function SDL_RectEmptyFloat cdecl(byval rectangle as const SDL_FRect ptr) as boolean
	return (rectangle = 0) orelse (rectangle->w < 0) orelse (rectangle->h < 0)
end function

private function SDL_RectsEqualEpsilon cdecl(byval a as const SDL_FRect ptr, byval b as const SDL_FRect ptr, byval epsilon as single) as boolean
	if a = 0 orelse b = 0 then return false
	if a = b then return true
	return (SDL_fabsf(a->x - b->x) <= epsilon) andalso _
	       (SDL_fabsf(a->y - b->y) <= epsilon) andalso _
	       (SDL_fabsf(a->w - b->w) <= epsilon) andalso _
	       (SDL_fabsf(a->h - b->h) <= epsilon)
end function

'' Keep this as a function, matching C's addressable inline helper.
private function SDL_RectsEqualFloat cdecl(byval a as const SDL_FRect ptr, byval b as const SDL_FRect ptr) as boolean
	return SDL_RectsEqualEpsilon(a, b, SDL_FLT_EPSILON)
end function

'' -------------------------------------------------------------------------
'' Assertion policy
'' -------------------------------------------------------------------------

#ifndef SDL_ASSERT_LEVEL
	#ifdef __FB_DEBUG__
		#define SDL_ASSERT_LEVEL 2
	#else
		#define SDL_ASSERT_LEVEL 1
	#endif
#endif

extern "C"
	declare sub SDL3_Abort alias "abort"()
	#ifdef __FB_WIN32__
		#inclib "kernel32"
		declare sub SDL3_DebugBreak stdcall alias "DebugBreak"()
	#else
		declare function SDL3_Raise alias "raise"(byval signal_number as long) as long
	#endif
end extern

#ifdef __FB_WIN32__
	#define SDL_TriggerBreakpoint() SDL3_DebugBreak()
#else
	'' SIGTRAP is signal 5 on the supported Linux, BSD, and macOS targets.
	#define SDL_TriggerBreakpoint() SDL3_Raise(5)
#endif
#define SDL_FUNCTION __FUNCTION__
#define SDL_FILE __FILE__
#define SDL_ASSERT_FILE SDL_FILE
#define SDL_LINE __LINE__
#define SDL_disabled_assert(condition)

#macro SDL_enabled_assert(expression)
	scope
		static assertion as SDL_AssertData
		assertion.condition = strptr(#expression)
		do while (expression) = 0
			dim state as SDL_AssertState = SDL_ReportAssertion(@assertion, SDL_FUNCTION, SDL_FILE, SDL_LINE)
			if state = SDL_ASSERTION_RETRY then continue do
			if state = SDL_ASSERTION_BREAK then SDL_TriggerBreakpoint()
			if state = SDL_ASSERTION_ABORT then SDL3_Abort()
			exit do
		loop
	end scope
#endmacro

#if SDL_ASSERT_LEVEL >= 1
	#define SDL_assert_release(expression) SDL_enabled_assert(expression)
#else
	#define SDL_assert_release(expression) SDL_disabled_assert(expression)
#endif
#if SDL_ASSERT_LEVEL >= 2
	#define SDL_assert(expression) SDL_enabled_assert(expression)
#else
	#define SDL_assert(expression) SDL_disabled_assert(expression)
#endif
#if SDL_ASSERT_LEVEL >= 3
	#define SDL_assert_paranoid(expression) SDL_enabled_assert(expression)
#else
	#define SDL_assert_paranoid(expression) SDL_disabled_assert(expression)
#endif
#define SDL_assert_always(expression) SDL_enabled_assert(expression)

'' end of SDL_inline.bi
