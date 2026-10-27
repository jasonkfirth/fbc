'' Project: FreeBASIC compiler tests
'' File: structs/static-aggregate-defaults.bas
'' Purpose: Preserve explicit values and omitted members in static aggregates.
'' Responsibilities: Check nested records, unions, array dimensions and zero defaults.
'' This file intentionally does NOT depend on C-only observers or runtime callbacks.
' TEST_MODE : COMPILE_AND_RUN_OK

type OperatorRow
	kind as integer
	flags as integer
	text as const zstring ptr
	self_op as integer
end type

type NestedRow
	item as OperatorRow
	tail as long
end type

union ChoiceRow
	value as long
	alternative as double
end union

type PackedBits
	first:2 as uinteger
	second:4 as uinteger
	third:5 as uinteger
	tail as long
end type

dim shared rows(0 to 2) as OperatorRow = { (2, 5, @"let"), (3, 5, @"+=", 28), (0) }
dim shared nested as NestedRow = ((7, 9, @"nested"))
dim shared choices(0 to 1) as ChoiceRow = { (19), (0) }
dim packed_value as PackedBits = (1, 2)
dim shared grid(0 to 1, 0 to 2) as OperatorRow = { { (11), (12) }, { (21) } }

if rows(0).kind <> 2 or rows(0).flags <> 5 or rows(0).self_op <> 0 then end 1
if *rows(0).text <> "let" then end 2
if rows(1).self_op <> 28 or rows(2).text <> 0 or rows(2).self_op <> 0 then end 3
if nested.item.kind <> 7 or nested.item.self_op <> 0 or nested.tail <> 0 then end 4
if choices(0).value <> 19 or choices(1).value <> 0 then end 6
if packed_value.first <> 1 or packed_value.second <> 2 or packed_value.third <> 0 or packed_value.tail <> 0 then end 7
if grid(0,0).kind <> 11 or grid(0,1).kind <> 12 or grid(0,2).kind <> 0 then end 8
if grid(1,0).kind <> 21 or grid(1,1).kind <> 0 or grid(1,2).kind <> 0 then end 9
if grid(1,0).text <> 0 or grid(1,0).self_op <> 0 then end 10
end 0

'' end of structs/static-aggregate-defaults.bas
