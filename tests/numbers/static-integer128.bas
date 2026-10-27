'' Project: FreeBASIC compiler tests
'' File: numbers/static-integer128.bas
'' Purpose: Keep wide constants separated inside static aggregate initializers.
'' Responsibilities: Check signed/unsigned arrays, adjacent narrow fields and omitted defaults.
'' This file intentionally does NOT assume that C has a native 128-bit integer type.
' TEST_MODE : COMPILE_AND_RUN_OK

type WideRow
	prefix as long
	value as integer<128>
	suffix as long
end type

dim shared values(0 to 2) as uinteger<128> = { 1, 2, 3 }
dim shared row as WideRow = (7, -1, 9)
dim shared wide_tail as WideRow = (11)

if values(0) <> 1 or values(2) <> 3 then end 1
if values(1) <> 2 then end 2
if row.prefix <> 7 or row.value <> -1 or row.suffix <> 9 then end 3
if wide_tail.prefix <> 11 or wide_tail.value <> 0 or wide_tail.suffix <> 0 then end 4
end 0

'' end of numbers/static-integer128.bas
