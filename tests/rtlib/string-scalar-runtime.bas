'' Project: FreeBASIC runtime tests
'' File: string-scalar-runtime.bas
'' Purpose: Exercise unsigned parsing and wide ASC through ordinary BASIC calls.
'' Responsibilities: Check string overloads, native result types and raw code units.
'' Exhaustive native-width values belong to string-scalar-values.c.
#lang "fb"

dim values(0 to 5) as string = { "0", "42tail", "4294967295", "&Hffffffff", "&O37777777777", "&B11111111" }
dim expected(0 to 5) as ulong = { 0, 42, &hffffffffu, &hffffffffu, &hffffffffu, 255 }
dim wideValue as wstring * 64
for index as integer = lbound(values) to ubound(values)
    wideValue = values(index)
    if valuint(values(index)) <> expected(index) then end 10
    if valuint(wideValue) <> expected(index) then end 11
next

dim codeUnits as wstring * 4
codeUnits[0] = &h41
codeUnits[1] = &hd800
codeUnits[2] = &hffff
codeUnits[3] = 0
if asc(codeUnits, 1) <> &h41 then end 20
if asc(codeUnits, 2) <> &hd800 then end 21
if asc(codeUnits, 3) <> &hffff then end 22
if asc(codeUnits, 0) <> 0 then end 23
if asc(codeUnits, 4) <> 0 then end 24
#if sizeof(wstring) = 4
codeUnits[0] = &h80000000u
codeUnits[1] = &hffffffffu
codeUnits[2] = 0
if asc(codeUnits, 1) <> &h80000000u then end 25
if asc(codeUnits, 2) <> &hffffffffu then end 26
#endif
print "STRING_SCALAR_RUNTIME_PASS"

'' end of string-scalar-runtime.bas
