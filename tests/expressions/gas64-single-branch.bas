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

#cmdline "-gen gas64"

dim as single a = any, b = any, c = any, d = any

c = a + b

if( c > d ) then
	print "greater"
end if

/' end of gas64-single-branch.bas '/
