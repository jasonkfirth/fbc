/'
    FreeBASIC Runtime Tests
    File: qb/vb-string-helpers.bas
    Purpose: Verify string.bi and file.bi declarations in the QB dialect.
    Responsibilities: Calling conventions, pointer-sized indices, and constants.
    This file does not test the VB parser or require filesystem mutation.
'/
' TEST_MODE : COMPILE_AND_RUN_OK
' TEST_LANG : qb

#include once "vbcompat.bi"

dim result as string
result = Replace("abcabc", "a", "x", 4, 1, vbBinaryCompare)
if result <> "xbc" then end 1
if StrComp("AB", "ab", vbTextCompare) <> 0 then end 2
if StrReverse("abc") <> "cba" then end 3
if vbNormal <> 33 then end 4
if fbFileAttrNormal <> 0 then end 5
if GetAttr("") <> -1 then end 6
if SetAttr("", fbFileAttrNormal) <> 1 then end 7
#ifdef __FB_64BIT__
if Replace("abc", "a", "x", 4294967297) <> "" then end 8
if Replace("abc", "a", "x", 1, 4294967296) <> "xbc" then end 9
#endif
if vbCrLf <> chr$(13, 10) then end 10
if vbNullChar <> chr$(0) then end 11
end 0

' end of qb/vb-string-helpers.bas
