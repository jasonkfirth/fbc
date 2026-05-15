#ifndef __crt_haiku_wchar_bi__
#define __crt_haiku_wchar_bi__

#include once "crt/stdio.bi"
#include once "crt/stdarg.bi"
#include once "crt/stddef.bi"
#include once "crt/long.bi"
#include once "crt/stdint.bi"

type wctype_t as long

type mbstate_t
	converter as any ptr
	charset(0 to 63) as byte
	count as ulong
	data(0 to 1031) as byte
end type

#ifndef WEOF
#define WEOF cast(wint_t, -1)
#endif

extern "C"
declare function mbsnrtowcs (byval dest as wchar_t ptr, byval src as const zstring ptr ptr, byval src_length as size_t, byval dest_length as size_t, byval mb_state as mbstate_t ptr) as size_t
declare function wcsnrtombs (byval dest as zstring ptr, byval src as wchar_t ptr ptr, byval src_length as size_t, byval dest_length as size_t, byval mb_state as mbstate_t ptr) as size_t
end extern

#endif
