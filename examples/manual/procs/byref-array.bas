'' Project: FreeBASIC manual examples
'' File: procs/byref-array.bas
'' Purpose: Show explicit BYREF on a dynamic array parameter.
'' Element and bounds changes are visible to the caller.

sub EditOriginal( byref values(any) as integer )
	values(-1) += 10
	redim preserve values(-1 to 2)
end sub

redim values(-1 to 1) as integer
values(-1) = 7

EditOriginal( values() )
print "Outside = " & values(-1) & ", upper bound = " & ubound( values )

'' end of procs/byref-array.bas
