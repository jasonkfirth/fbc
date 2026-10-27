'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression reductions
'' File: c-array-copy-values.bas
'' Purpose: Exercise implicit dynamic-field copies across element lifetimes.
'' Responsibilities: Empty, nonempty, self and multidimensional copies.
'' This file intentionally does NOT inject allocation failures.
#lang "fb"

dim shared as integer live_items
dim shared as integer copied_items

type item
	value as integer
	declare constructor()
	declare destructor()
	declare operator let( byref rhs as const item )
end type

constructor item()
	value = 0
	live_items += 1
end constructor

destructor item()
	live_items -= 1
end destructor

operator item.let( byref rhs as const item )
	value = rhs.value
	copied_items += 1
end operator

type holder
	leading as integer
	numbers(any) as integer
	words(any) as string
	grid(any, any) as integer
	items(any) as item
	trailing as integer
end type

scope
	dim as holder source_value, destination, empty_value
	destination = source_value
	if ubound(destination.numbers) <> -1 then end 1
	if ubound(destination.words) <> -1 then end 2
	if ubound(destination.grid, 1) <> -1 then end 3
	if ubound(destination.items) <> -1 then end 4
	if live_items <> 0 then end 5

	source_value.leading = 17
	source_value.trailing = 23
	redim source_value.numbers(-2 to 0)
	for index as integer = -2 to 0
		source_value.numbers(index) = index + 101
	next
	redim source_value.words(1 to 2)
	source_value.words(1) = "first"
	source_value.words(2) = "second"
	redim source_value.grid(-1 to 0, 4 to 5)
	for row_index as integer = -1 to 0
		for column_index as integer = 4 to 5
			source_value.grid(row_index, column_index) = row_index * 10 + column_index
		next
	next
	redim source_value.items(3 to 4)
	source_value.items(3).value = 31
	source_value.items(4).value = 41
	redim destination.items(9 to 9)
	redim destination.words(0 to 0)
	destination.words(0) = "old"
	if live_items <> 3 then end 6

	destination = source_value
	if live_items <> 4 or copied_items <> 2 then end 7
	if destination.leading <> 17 or destination.trailing <> 23 then end 8
	if lbound(destination.numbers) <> -2 or ubound(destination.numbers) <> 0 then end 9
	for index as integer = -2 to 0
		if destination.numbers(index) <> index + 101 then end 10
	next
	if destination.words(1) <> "first" or destination.words(2) <> "second" then end 11
	for row_index as integer = -1 to 0
		for column_index as integer = 4 to 5
			if destination.grid(row_index, column_index) <> row_index * 10 + column_index then end 12
		next
	next
	if destination.items(3).value <> 31 or destination.items(4).value <> 41 then end 13

	'' Copies own their string storage and element arrays independently.
	destination.words(1) = "changed"
	destination.numbers(-2) = 999
	if source_value.words(1) <> "first" or source_value.numbers(-2) <> 99 then end 14
	destination = destination
	if destination.words(1) <> "changed" or destination.numbers(-2) <> 999 then end 15
	if live_items <> 4 then end 16

	'' Empty assignment must release both string and object elements.
	destination = empty_value
	if live_items <> 2 then end 17
	if ubound(destination.numbers) <> -1 or ubound(destination.words) <> -1 then end 18
	if ubound(destination.grid, 1) <> -1 or ubound(destination.items) <> -1 then end 19
end scope

if live_items <> 0 then end 20
end 0
'' end of c-array-copy-values.bas
