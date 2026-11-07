'' Project: FreeBASIC C backend declaration controls
'' File: c-wide-libc-forward.bas
'' Purpose: Check a same-name source definition following its first caller.
'' Responsibilities: Retain source procedure behavior during C call emission.
'' This file intentionally does NOT execute a replacement C library function.

#include once "crt/stddef.bi"
extern "C"
declare function wcslen(byval text as const wchar_t ptr) as size_t
declare function wcschr(byval text as const wchar_t ptr, byval wanted as wchar_t) as wchar_t ptr
function ForwardCaller(byval text as const wchar_t ptr) as size_t
	return wcslen(text)
end function
function ForwardSearch(byval text as const wchar_t ptr, byval wanted as wchar_t) as wchar_t ptr
	return wcschr(text, wanted)
end function
function wcslen(byval text as const wchar_t ptr) as size_t
	return 42 + cuint(text = 0)
end function
function wcschr(byval text as const wchar_t ptr, byval wanted as wchar_t) as wchar_t ptr
	return 0
end function
end extern
dim as wchar_t code_units(0 to 1) = { 65, 0 }
dim as size_t value = ForwardCaller(@code_units(0))
dim as wchar_t ptr location = ForwardSearch(@code_units(0), 65)
if location <> 0 then end 77
end (clng(value))

'' end of c-wide-libc-forward.bas
