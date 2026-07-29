/'
    FreeBASIC compiler regression test
    ----------------------------------

    File: static-local-access.bas

    Purpose:

        Verify static-local access tracking in procedure ASTs.

    Responsibilities:

        - retain a referenced static local across calls
        - provide an unreferenced static local for HLC emission checks

    This file intentionally does NOT contain:

        - static objects with constructors or destructors
        - shared or common variables
'/

' TEST_MODE : COMPILE_AND_RUN_OK

private function nextValue( ) as integer static
	dim as integer unused_static
	dim as integer used_static

	used_static += 1
	return used_static
end function

if( nextValue( ) <> 1 ) then
	end 1
end if

if( nextValue( ) <> 2 ) then
	end 1
end if

end 0

/' end of static-local-access.bas '/
