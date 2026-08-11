/'
    FreeBASIC compiler regression test
    ----------------------------------

    File: escaped-alias.bas

    Purpose:

        Verify that escape sequences in ALIAS strings name the emitted symbol.

    Responsibilities:

        - cover decimal escapes in an ALIAS string
        - prove that procedure declarations and definitions resolve together

    This file intentionally does NOT contain:

        - C++ name mangling
        - procedure calling-convention coverage
'/

' TEST_MODE : COMPILE_AND_RUN_OK

declare function escaped_reference alias !"\65"( ) as long

function escaped_storage alias "A"( ) as long
	function = 123
end function

if( escaped_reference( ) <> 123 ) then
	end 1
end if

/' end of escaped-alias.bas '/
