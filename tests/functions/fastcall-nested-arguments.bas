/'
    FreeBASIC compiler regression test
    ----------------------------------

    File: fastcall-nested-arguments.bas

    Purpose:

        Verify that nested calls do not corrupt register arguments.

    Responsibilities:

        - exercise the second 32-bit FASTCALL argument held in EDX
        - force a later argument expression to clobber caller-saved EDX
        - verify calls with one and two nested argument expressions

    This file intentionally does NOT contain:

        - non-x86 calling-convention tests
        - platform ABI interoperation
'/

' TEST_MODE : COMPILE_AND_RUN_OK

#if defined( __FB_WIN32__ ) andalso not defined( __FB_64BIT__ )

private function packValues __fastcall _
	( _
		byval first_value as long, _
		byval second_value as long _
	) as long

	return first_value * 1000 + second_value
end function

private function clobberEdx( byval result as long ) as long
	asm
		mov edx, &h13579BDF
	end asm

	return result
end function

if( packValues( clobberEdx( 7 ), 22 ) <> 7022 ) then
	end 1
end if

if( packValues( clobberEdx( 7 ), clobberEdx( 22 ) ) <> 7022 ) then
	end 1
end if

#endif

end 0

/' end of fastcall-nested-arguments.bas '/
