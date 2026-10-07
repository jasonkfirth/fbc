'' Project: FreeBASIC semantic sidecar tests
'' File: numeric-assignment-targets.bas
'' Purpose: Preserve original numeric inputs and their accepted destination types.
'' Responsibilities: Initializers, array elements, fields and ordinary assignments.
'' This file intentionally does NOT execute narrowed or overflowing operations.
#lang "fb"

type NumericTarget
	small_value as byte
end type

sub store_values(byval source_value as long)
	dim small_value as byte = source_value
	dim values(0 to 1) as byte = { source_value, 1 }
	dim record_value as NumericTarget
	record_value.small_value = source_value
	small_value = source_value + 1
	dim wide_value as longint = source_value * source_value
	dim address_value as long ptr = @source_value
	print cint(source_value + 0.5), cbool(1.5), clngint(source_value)
end sub

'' end of numeric-assignment-targets.bas
