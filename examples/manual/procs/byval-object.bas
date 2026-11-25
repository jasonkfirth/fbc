'' Project: FreeBASIC manual examples
'' File: procs/byval-object.bas
'' Purpose: Show how value arguments copy a UDT's string and dynamic array.
'' The example uses the type's automatically generated copy constructor.

type Record
	title as string
	scores(any) as integer
end type

sub EditCopy( byval item as Record )
	item.title = "Edited"
	item.scores(0) += 10
	print "Inside = " & item.title & ", " & item.scores(0)
end sub

dim as Record item
item.title = "Original"
redim item.scores(0 to 1)
item.scores(0) = 7

EditCopy( (byval item) )
print "Outside = " & item.title & ", " & item.scores(0)

'' end of procs/byval-object.bas
