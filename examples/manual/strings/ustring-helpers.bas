'' Project: FreeBASIC examples
'' File: ustring-helpers.bas
'' Purpose: Use optional string helpers with wide and UTF-8 text.
'' Responsibilities: Demonstrate comparison, replacement, reversal, and formats.
'' This file intentionally does NOT access files or change system settings.

#include once "string.bi"
#include once "fbc-int/string.bi"

dim as string bytes = "FreeBASIC"
dim as wstring * 32 wide = wchr(&hE9, &h4E2D, &h1F600)
dim as ustring utf8 = ustring(wide)

print "Reverse STRING: "; StrReverse(bytes)
print "Reverse WSTRING: "; StrReverse(wide)
print "Reverse USTRING: "; StrReverse(utf8)
print "Replace a wide pattern: "; Replace(utf8, wchr(&h4E2D), "X")
print "Unicode case comparison: "; StrComp(uchr(&hDF), "SS", fbTextCompare)
print "Unicode case replacement: "; Replace(ustring("Stra") + uchr(&hDF) + "e", "STRASSE", "street", 1, -1, fbTextCompare)
print "Unicode number mask: "; Format(12.5, ustring("0.0 ") + uchr(&h20AC))
print using "\ \"; utf8
print using ustring("& ") + uchr(&h2713); utf8

'' LEFTSELF changes the length in place and retains the allocation.
FBC.LeftSelf(wide, 2)
FBC.LeftSelf(utf8, 2)
print "Truncated wide text: "; wide
print "Truncated UTF-8 text: "; utf8

'' end of ustring-helpers.bas
