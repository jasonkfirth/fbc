'' TEST_MODE : COMPILE_AND_RUN_OK
/'
	Project: FreeBASIC C backend regression controls
	File: c-abi-parameter-opaque.bas
	Purpose: Retain visible diagnostics when inline assembly obscures use analysis.
	Responsibilities: Exercise the conservative final-output boundary.
	This file does not change calling conventions or assembly-visible operands.
'/

#include once "../compiler-support/c-opaque-nop.bi"

#macro CHECK( condition )
	if( (condition) = 0 ) then
		end 1
	end if
#endmacro

function OpaqueIgnored(byval ignored_value as integer, byval result as integer) as integer
	compiler_test_opaque_nop()
	return result
end function

CHECK(OpaqueIgnored(1, 7) = 7)

' end of c-abi-parameter-opaque.bas
