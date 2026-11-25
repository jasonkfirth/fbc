'' Project: FreeBASIC manual examples
'' File: procs/byval-argument.bas
'' Purpose: Use an explicit BYVAL argument for a default value parameter.
'' The BYREF parameter provides a visible change in the caller.

sub AddTo( byref total as integer, amount as integer )
	total += amount
	amount = 0
end sub

dim as integer total = 10, amount = 5
AddTo total, (byval amount)

print "Total = " & total
print "Amount = " & amount

'' end of procs/byval-argument.bas
