/*
    FreeBASIC runtime library
    -------------------------

    File: aros/arm/math_exp.c

    Purpose:

        Select the shared AROS binary64 exp implementation for ARM.

    This file intentionally does NOT contain:

        - the executable approximation algorithm or coefficient tables
        - generic ARM runtime policy
        - behavior for non-AROS targets
*/

#include "../softfloat64/math_exp.c"

/* end of aros/arm/math_exp.c */
