'' Project: FreeBASIC compiler tests
'' File: dim/byval-array-rank-error.bas
'' Purpose: Check runtime rank validation before a BYVAL array is used.
'' A forwarded array can carry a rank only known at runtime.

' TEST_MODE : COMPILE_AND_RUN_FAIL

sub needsVector( byval values(any) as integer )
	end 0
end sub

sub forwardArray( values() as integer )
	needsVector( values() )
end sub

dim grid(0 to 1, 0 to 1) as integer
forwardArray( grid() )

'' end of dim/byval-array-rank-error.bas
