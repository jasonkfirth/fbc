/'
    Project: FreeBASIC compiler regression tests
    File: gas64-single-stack-argument.bas
    Purpose: Pass a negative Single after the floating argument registers fill.
    Responsibilities: Check immediate encoding and the received argument values.
    This file intentionally does NOT contain: external library calls.
'/

' TEST_MODE : COMPILE_AND_RUN_OK

#if defined( __FB_X86__ ) and defined( __FB_64BIT__ ) and not defined( __FB_DARWIN__ )
	#cmdline "-gen gas64"

	function receive_values cdecl(byval a as single, byval b as single, byval c as single, _
	                              byval d as single, byval e as single, byval f as single, _
	                              byval g as single, byval h as single, byval i as single) as long
		if a <> 1.0f orElse b <> 2.0f orElse c <> 3.0f then return 1
		if d <> 4.0f orElse e <> 5.0f orElse f <> 6.0f then return 2
		if g <> 7.0f orElse h <> 8.0f orElse i <> -1.0f then return 3
		return 0
	end function

	end receive_values(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, -1.0f)
#endif

/' end of gas64-single-stack-argument.bas '/
