'' Project: FreeBASIC semantic sidecar tests
'' File: load-actions.bas
'' Purpose: Exercise native check transfers and Unicode access during AST loading.
'' Responsibilities: Fixed/descriptor arrays, pointer guards and scalar runtime calls.
'' This file intentionally does NOT dereference an invalid pointer or index.

private sub UnusedLoadBody( )
	dim unused_load_value as integer = 7
	unused_load_value += 1
end sub

function LoadArrayValue( values() as integer, byval index as integer ) as integer
	return values(index)
end function

function LoadActionProbe( byval index as integer, byval address as integer ptr ) as integer
	dim values(0 to 2) as integer
	values(index) = 3
	dim text_value as ustring = "ab"
	dim scalar_value as ulong = text_value[index]
	text_value[index] = asc("Z")
	return values(index) + *address + scalar_value + LoadArrayValue(values(), index)
end function

dim pointer_value as integer = 7
print LoadActionProbe(1, @pointer_value)

'' end of load-actions.bas
