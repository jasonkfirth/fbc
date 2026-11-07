/'
    Project: FreeBASIC string header regression
    File: c-string-replace-optout.bas
    Purpose: Keep a project Replace helper alongside VB compatibility headers.
    Responsibilities: Check the opt-out and unrelated comparison constants.
    This fixture intentionally does NOT exercise Unicode collation.
'/
#define FB_NO_STRING_REPLACE
private declare function Replace(byref expression as wstring, byref finding as wstring, _
    byref replacement as wstring, byval start as integer = 1, _
    byval count as integer = -1, matchCase as boolean = true, _
    byref countReplaced as integer = 0) as string
#include once "vbcompat.bi"
private function Replace(byref expression as wstring, byref finding as wstring, _
    byref replacement as wstring, byval start as integer, _
    byval count as integer, matchCase as boolean, _
    byref countReplaced as integer) as string
    if expression <> "a" orelse finding <> "b" orelse replacement <> "c" then return ""
    if start <> 1 orelse count <> -1 orelse matchCase = false then return ""
    countReplaced = 1
    return "project"
end function
dim as wstring * 8 a = "a", b = "b", c = "c"
dim as integer replacements
if Replace(a, b, c, 1, -1, true, replacements) <> "project" then end 1
if replacements <> 1 then end 3
if vbBinaryCompare <> 0 or vbTextCompare <> 1 then end 2
end 0
' end of c-string-replace-optout.bas
