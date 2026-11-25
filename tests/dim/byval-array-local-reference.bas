'' Project: FreeBASIC compiler tests
'' File: dim/byval-array-local-reference.bas
'' Purpose: Diagnose references returned to a procedure-owned array copy.
'' A normal array parameter can refer to storage that belongs to the caller.

' TEST_MODE : COMPILE_ONLY_FAIL

function element( byval values() as integer ) byref as integer
	return values(0)
end function

'' end of dim/byval-array-local-reference.bas
