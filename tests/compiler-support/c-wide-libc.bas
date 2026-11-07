'' Project: FreeBASIC C backend regression tests
'' File: c-wide-libc.bas
'' Purpose: Check packed wide inputs without changing native function addresses.
'' Responsibilities: Exercise direct adapters and the original aligned callback ABI.
'' This file intentionally does NOT call native callbacks with unaligned storage.

#include once "crt/wchar.bi"

type NativeWideLength as function cdecl(byval as const wchar_t ptr) as size_t
declare function native_wide_length cdecl alias "native_wide_length"() as NativeWideLength
type NativeWideSearch as function cdecl(byval as const wchar_t ptr, byval as wchar_t) as wchar_t ptr
declare function native_wide_search cdecl alias "native_wide_search"() as NativeWideSearch

dim as ubyte first_storage(0 to 127), second_storage(0 to 127)
for offset as integer = 0 to 7
	dim first as wchar_t ptr = cptr(wchar_t ptr, @first_storage(offset))
	dim second as wchar_t ptr = cptr(wchar_t ptr, @second_storage(offset))
	first[0] = &hE9
	first[1] = 65
	first[2] = 0
	second[0] = &hE9
	second[1] = 66
	second[2] = 0
	if wcslen(first) <> 2 then end 1
	if wcsncmp(first, second, 0) <> 0 then end 2
	if wcsncmp(first, second, 1) <> 0 then end 3
	if wcsncmp(first, second, 2) >= 0 then end 4
	if wcsncmp(second, first, 2) <= 0 then end 5
	if wcschr(first, &hE9) <> first then end 8
	if wcschr(first, 65) <> first + 1 then end 9
	if wcschr(first, 0) <> first + 2 then end 10
	if wcschr(first, 66) <> 0 then end 11
	first[1] = &hE9
	if wcschr(first, &hE9) <> first then end 12
	first[1] = &hFFFF
	if wcschr(first, &hFFFF) <> first + 1 then end 13
	dim located as wchar_t ptr = wcschr(first, &hFFFF)
	if located = 0 then end 16
	*located = 67
	if first[1] <> 67 then end 17
next

dim as wchar_t aligned_text(0 to 2) = { 65, 66, 0 }
dim reader as NativeWideLength = @wcslen
if reader <> native_wide_length() then end 6
if reader(@aligned_text(0)) <> 2 then end 7
dim searcher as NativeWideSearch = @wcschr
if searcher <> native_wide_search() then end 14
if searcher(@aligned_text(0), 66) <> @aligned_text(1) then end 15
end 0

'' end of c-wide-libc.bas
