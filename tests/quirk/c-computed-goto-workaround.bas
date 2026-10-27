/'
    Project: FreeBASIC C backend regression tests
    File: c-computed-goto-workaround.bas
    Purpose: Keep direct control flow free of the computed-goto workaround.
    Responsibilities: Exercise ordinary functions and direct branches.
    This file intentionally does NOT use runtime error handlers or inline ASM.
'/
' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fb"

private function add_one( byval value as long ) as long

	return value + 1

end function

if( add_one( 41 ) <> 42 ) then end 1
end 0

' end of c-computed-goto-workaround.bas
