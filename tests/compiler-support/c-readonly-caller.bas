/'
	Project: FreeBASIC C backend regression controls
	File: c-readonly-caller.bas
	Purpose: Call the original mutable-pointer ABI from another translation unit.
	Responsibilities: Check direct and indirect calls into one shared body.
	This file intentionally does NOT contain: The implementation under test.
'/

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_READONLY_CONTROL_FAILED", __LINE__
		end 1
	end if
#endmacro

declare function ReadText(byval text as zstring ptr) as string
dim as zstring * 32 buffer = "external"
dim reader as function(byval as zstring ptr) as string = @ReadText
VERIFY(ReadText(@buffer) = "external1")
VERIFY(reader(@buffer) = "external2")
print "C_READONLY_ABI_PASS"

'' end of c-readonly-caller.bas
