'' FreeBASIC CRT declarations for classic Amiga newlib
'' ---------------------------------------------------
''
'' File: amiga/crt/wchar.bi
'' Purpose: Define the SDK's multibyte state and wide character interfaces.
'' Responsibilities: Preserve the eight-byte mbstate_t ABI.
'' This file intentionally does NOT contain FreeBASIC WSTRING implementation.

#ifndef __crt_wchar_bi__
#define __crt_wchar_bi__

#include once "crt/stdio.bi"
#include once "crt/string.bi"
#include once "crt/time.bi"

'' Newlib stores a conversion count followed by a four-byte character union.
type mbstate_t
	__count as long
	__value as ulong
end type

#define WEOF &hfffffffful

extern "c"
declare function btowc (byval character as long) as wint_t
declare function mbrlen (byval source as const zstring ptr, byval bytes as size_t, byval state as mbstate_t ptr) as size_t
declare function mbrtowc (byval result as wchar_t ptr, byval source as const zstring ptr, byval bytes as size_t, byval state as mbstate_t ptr) as size_t
declare function mbsrtowcs (byval result as wchar_t ptr, byval source as const zstring ptr ptr, byval characters as size_t, byval state as mbstate_t ptr) as size_t
declare function wcrtomb (byval result as zstring ptr, byval character as wchar_t, byval state as mbstate_t ptr) as size_t
declare function wcsrtombs (byval result as zstring ptr, byval source as const wchar_t ptr ptr, byval bytes as size_t, byval state as mbstate_t ptr) as size_t
declare function wctob (byval character as wint_t) as long
declare function mbsinit (byval state as const mbstate_t ptr) as long
declare function wmemset (byval destination as wchar_t ptr, byval character as wchar_t, byval count as size_t) as wchar_t ptr
declare function wmemchr (byval source as const wchar_t ptr, byval character as wchar_t, byval count as size_t) as wchar_t ptr
declare function wmemcmp (byval left_value as const wchar_t ptr, byval right_value as const wchar_t ptr, byval count as size_t) as long
declare function wmemmove (byval destination as wchar_t ptr, byval source as const wchar_t ptr, byval count as size_t) as wchar_t ptr
declare function wmemcpy (byval destination as wchar_t ptr, byval source as const wchar_t ptr, byval count as size_t) as wchar_t ptr
end extern

#endif

'' end of amiga/crt/wchar.bi
