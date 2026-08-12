''
'' FreeBASIC CRT declarations for GCCSDK UnixLib
'' ---------------------------------------------
''
'' File: crt/wchar.bi
''
'' Purpose:
''
''     Define the complete UnixLib wide-character interface.
''
'' Responsibilities:
''
''     - reproduce the UnixLib multibyte conversion state
''     - define the UnixLib WEOF representation
''     - declare UnixLib wide-character memory and conversion functions
''
'' This file intentionally does NOT contain:
''
''     - locale conversion tables
''     - FreeBASIC WSTRING implementation details
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
#include once "crt/stddef.bi"

union mbstate_t__value
	__wch as wint_t
	'' UnixLib stores either one wint_t or the same four bytes in progress.
	__wchb(0 to 4-1) as byte
end union

type mbstate_t
	__count as long
	__value as mbstate_t__value
end type

#ifndef WEOF
#define WEOF &hfffffffful
#endif


extern "c"
declare function btowc (byval as long) as wint_t
declare function mbrlen (byval as const zstring ptr, byval as size_t, byval as mbstate_t ptr) as size_t
declare function mbrtowc (byval as const wchar_t ptr, byval as zstring ptr, byval as size_t, byval as mbstate_t ptr) as size_t
declare function mbsrtowcs (byval as wchar_t ptr, byval as const zstring ptr ptr, byval as size_t, byval as mbstate_t ptr) as size_t
declare function wcrtomb (byval as zstring ptr, byval as wchar_t, byval as mbstate_t ptr) as size_t
declare function wcsrtombs (byval as const zstring ptr, byval as wchar_t ptr ptr, byval as size_t, byval as mbstate_t ptr) as size_t
declare function wctob (byval as wint_t) as integer
declare function fwide (byval stream as FILE ptr, byval mode as long) as long
declare function mbsinit (byval ps as const mbstate_t ptr) as long
declare function wmemset (byval s as wchar_t ptr, byval c as wchar_t, byval n as size_t) as wchar_t ptr
declare function wmemchr (byval s as const wchar_t ptr, byval c as wchar_t, byval n as size_t) as wchar_t ptr
declare function wmemcmp (byval s1 as const wchar_t ptr, byval s2 as const wchar_t ptr, byval n as size_t) as integer
declare function wmemmove (byval s1 as wchar_t ptr, byval s2 as const wchar_t ptr, byval n as size_t) as wchar_t ptr
declare function wmemcpy(byval as wchar_t ptr, byval as const wchar_t ptr, byval as uinteger) as wstring ptr
end extern

#endif

'' end of crt/wchar.bi
