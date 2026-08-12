/'
    FreeBASIC compiler regression test
    ----------------------------------

    File: gas64-single-branch.bas

    Purpose:

        Exercise a branch that consumes a single-precision comparison.

    Responsibilities:

        - keep the comparison result in branch form
        - cover the gas64 register-width path used without debug information

    This file intentionally does NOT contain:

        - value-producing Boolean comparisons
        - floating-point exception or NaN behavior
'/

' TEST_MODE : COMPILE_ONLY_OK

' The gas64 backend emits x86-64 assembly and is deliberately unavailable on
' ARM64, PowerPC64, and other 64-bit architectures.  Keep this source in the
' portable compile sweep, but request gas64 only where it can exercise the
' intended register-width path.
#if defined( __FB_X86__ ) and defined( __FB_64BIT__ )
	#cmdline "-gen gas64"

	dim as single a = any, b = any, c = any, d = any

	c = a + b

	if( c > d ) then
		print "greater"
	end if
#endif

/' end of gas64-single-branch.bas '/
