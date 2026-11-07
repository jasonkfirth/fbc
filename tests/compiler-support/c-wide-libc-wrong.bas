'' Project: FreeBASIC C backend declaration controls
'' File: c-wide-libc-wrong.bas
'' Purpose: Keep native diagnostics for an incompatible libc declaration.
'' Responsibilities: Emit the original wrong return type without an adapter.
'' This file intentionally does NOT link or execute the invalid declaration.

#include once "crt/stddef.bi"
declare function WrongLength cdecl alias "wcslen"(byval text as const wchar_t ptr) as long
declare function WrongSearch cdecl alias "wcschr"(byval text as const wchar_t ptr, byval wanted as wchar_t) as const wchar_t ptr
dim as wchar_t code_units(0 to 1) = { 65, 0 }
dim as long value = WrongLength(@code_units(0))
dim as const wchar_t ptr location = WrongSearch(@code_units(0), 65)
if location = 0 then value += 1
end value

'' end of c-wide-libc-wrong.bas
