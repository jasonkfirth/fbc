'' Project: FreeBASIC manual examples
'' File: procs/byval-array.bas
'' Purpose: Show how a BYVAL array isolates element and bounds changes.
'' The caller displays its original dynamic array after the procedure exits.

sub EditCopy( byval values(any) as integer )
	values(-1) += 10
	redim preserve values(-1 to 2)
	print "Inside = " & values(-1) & ", upper bound = " & ubound( values )
end sub

redim values(-1 to 1) as integer
values(-1) = 7

EditCopy( values() )
print "Outside = " & values(-1) & ", upper bound = " & ubound( values )

'' end of procs/byval-array.bas
