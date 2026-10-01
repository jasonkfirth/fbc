' TEST_MODE : COMPILE_AND_RUN_OK

'' FreeBASIC Compiler Test Suite
'' File: llvm-local-names.bas
'' Purpose: Verify local names remain distinct across BASIC scopes.
'' Responsibilities: Check shadowed parameters and sibling local variables.
'' This file does not test global symbol naming or external linkage.

function accumulate( byval value as long ) as long
	dim total as long = value
	scope
		dim value as long = 10
		total += value
	end scope
	scope
		dim value as long = 20
		total += value
	end scope
	return total + value
end function

if( accumulate(3) <> 36 ) then end 1
if( accumulate(7) <> 44 ) then end 2
end 0

'' end of llvm-local-names.bas
