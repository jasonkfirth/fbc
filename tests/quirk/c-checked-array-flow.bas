/'
    Project: FreeBASIC C backend regression tests
    File: c-checked-array-flow.bas
    Purpose: Keep checked array operations valid around indirect error jumps.
    Responsibilities: Exercise procedure entry labels, nested scopes and values.
    This file intentionally does NOT trigger an error or install a handler.
'/
' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fb"

private function checked_sum() as long

	dim values() as long
	redim values(1 to 2)
	values(1) = 10
	values(2) = 20

	scope
		dim as long first_value = values(1)
		function = first_value + values(2)
	end scope

end function

dim values() as long
redim values(1 to 2)
values(1) = 10
values(2) = 20

if lbound(values) <> 1 then end 1
if ubound(values) <> 2 then end 2
if values(1) + values(2) <> 30 then end 3
if checked_sum() <> 30 then end 4
end 0

' end of c-checked-array-flow.bas
