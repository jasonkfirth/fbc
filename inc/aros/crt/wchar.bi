''
'' FreeBASIC CRT declarations for AROS wide-character services
'' ------------------------------------------------------------
''
'' File: crt/wchar.bi
''
'' Purpose:
''
''     Define the AROS multibyte conversion state and wide-character API.
''
'' Responsibilities:
''
''     - reproduce the AROS mbstate_t layout
''     - define the unsigned 32-bit AROS WEOF representation
''     - declare wide-character memory and conversion functions
''
'' This file intentionally does NOT contain:
''
''     - FreeBASIC WSTRING implementation details
''     - stdcio wide-stream declarations already owned by crt/stdio.bi
''     - declarations for other operating systems
''
#ifndef __crt_wchar_bi__
#define __crt_wchar_bi__

#include once "crt/stdio.bi"
#include once "crt/stdlib.bi"
#include once "crt/string.bi"
#include once "crt/time.bi"
#include once "crt/sys/types.bi"
#include once "crt/stddef.bi"

type mbstate_t
	__state as long
	__count as ulong
	__value as wchar_t
end type

#ifndef WEOF
#define WEOF &hfffffffful
#endif

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

'' end of crt/wchar.bi
