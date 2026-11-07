/'
    Project: FreeBASIC string header regression
    File: c-string-replace-default.bas
    Purpose: Keep the standard Replace declarations available by default.
    Responsibilities: Check byte, wide and UTF-8 replacement overloads.
    This fixture intentionally does NOT opt out of any helper declarations.
'/
#include once "vbcompat.bi"
if Replace("abc", "b", "x") <> "axc" then end 1
dim as wstring * 8 wide = wchr(&hE9, &h4E2D)
if Replace(wide, wchr(&h4E2D), "X") <> wchr(&hE9, 88) then end 2
dim as ustring utf8 = uchr(&hE9, &h4E2D, &h1F600)
if Replace(utf8, uchr(&h4E2D), "X") <> uchr(&hE9, 88, &h1F600) then end 3
end 0
' end of c-string-replace-default.bas
