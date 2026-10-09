/'
    FreeBASIC compiler regression test
    ----------------------------------

    File: fastcall-empty-parameters.bas

    Purpose:

        Verify register conventions with an empty parameter list.

    Responsibilities:

        - create zero-argument FASTCALL and THISCALL procedure-pointer types
        - define and call zero-argument procedures through those types
        - check that register setup does not dereference a missing parameter

    This file intentionally does NOT contain:

        - platform ABI interoperation
        - nonempty register-argument tests
'/

' TEST_MODE : COMPILE_AND_RUN_OK

type FastCallback as function __fastcall( ) as long
type ThisCallback as function __thiscall( ) as long

private function fastValue __fastcall( ) as long

	return 37
end function

private function thisValue __thiscall( ) as long

	return 41
end function

dim as FastCallback fast_pointer = @fastValue
dim as ThisCallback this_pointer = @thisValue

if( fast_pointer( ) <> 37 ) then
	end 1
end if

if( this_pointer( ) <> 41 ) then
	end 2
end if

end 0

/' end of fastcall-empty-parameters.bas '/
