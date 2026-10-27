'' Project: FreeBASIC compiler tests
'' File: structs/static-aggregate-members.bas
'' Purpose: Preserve defaults in array and anonymous aggregate members.
'' Responsibilities: Check pointer arrays, nested record arrays and promoted fields.
'' This file intentionally does NOT initialize arrays through unsupported record syntax.
' TEST_MODE : COMPILE_AND_RUN_OK

type TokenRow
	kind as integer
	text as const zstring ptr
end type

type RegistryRow
	tag as integer
	names(0 to 3) as const zstring ptr
	entries(0 to 1, 0 to 2) as TokenRow
	label as zstring * 7
end type

type AnonymousRow
	tag as integer
	union
		type
			first as integer
			second as integer
		end type
		alternative as double
	end union
	tail as integer
end type

dim shared rows(0 to 1) as RegistryRow = { (5), (0) }
dim shared omitted as AnonymousRow = (13)
dim shared explicit_values as AnonymousRow = (11, 7, 8, 9)
dim shared dynamic_values() as TokenRow

if rows(0).tag <> 5 or rows(1).tag <> 0 then end 1
for row_index as integer = 0 to 1
	for name_index as integer = 0 to 3
		if rows(row_index).names(name_index) <> 0 then end 2
	next
	for first_index as integer = 0 to 1
		for second_index as integer = 0 to 2
			if rows(row_index).entries(first_index, second_index).kind <> 0 then end 3
			if rows(row_index).entries(first_index, second_index).text <> 0 then end 4
		next
	next
	if len(rows(row_index).label) <> 0 then end 5
next
if omitted.tag <> 13 or omitted.first <> 0 or omitted.second <> 0 or omitted.tail <> 0 then end 6
if explicit_values.tag <> 11 or explicit_values.first <> 7 or explicit_values.second <> 8 or explicit_values.tail <> 9 then end 7
if ubound(dynamic_values) >= lbound(dynamic_values) then end 8
redim dynamic_values(-2 to 0, 3 to 5)
if lbound(dynamic_values, 1) <> -2 or ubound(dynamic_values, 1) <> 0 then end 9
if lbound(dynamic_values, 2) <> 3 or ubound(dynamic_values, 2) <> 5 then end 10
for first_index as integer = -2 to 0
	for second_index as integer = 3 to 5
		if dynamic_values(first_index, second_index).kind <> 0 or dynamic_values(first_index, second_index).text <> 0 then end 11
	next
next
end 0

'' end of structs/static-aggregate-members.bas
