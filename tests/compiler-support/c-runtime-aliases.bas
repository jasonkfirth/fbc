'' Project: FreeBASIC C backend regression tests
'' File: c-runtime-aliases.bas
'' Purpose: Check shared intrinsic/rtlib declarations and required object aliases.
'' Responsibilities: Preserve Unicode conversion and decorated procedure calls.
'' This file intentionally does NOT depend on foreign runtime execution.

#include once "fbnetwire.bi"

public function keyword_provider cdecl alias "for"(byval value as long) as long
	return value + 17
end function

public function stdcall_provider stdcall alias "stdcall_identity"(byval value as long) as long
	return value + 18
end function

public function plain_provider pascal alias "stdcall_plain"(byval value as long) as long
	return value + 19
end function

public function fastcall_provider __fastcall alias "fastcall_identity"(byval value as long) as long
	return value + 20
end function

dim as wstring * 8 wide = wchr(&hE9, 65)
dim as ustring utf8 = ustring(wide)
dim as wstring * 8 implicit_wide = wstr(utf8)
dim as string bytes = __FbNetWideToUtf8(@wide)
if bytes <> chr(&hC3, &hA9, 65) then end 1
if utf8 <> uchr(&hE9, 65) then end 2
if implicit_wide <> wide then end 9
dim as wstring ptr converted = __FbNetUtf8ToWide(bytes)
if converted = 0 then end 3
dim as integer same_text = (*converted = wide)
deallocate converted
if same_text = 0 then end 4
if keyword_provider(40) <> 57 then end 5
if stdcall_provider(40) <> 58 then end 6
if plain_provider(40) <> 59 then end 7
if fastcall_provider(40) <> 60 then end 8
end 0

'' end of c-runtime-aliases.bas
