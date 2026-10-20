/*
 * Project: FreeBASIC runtime library
 * File: drivers/aros/arm/math_exp.c
 * Purpose: Provide math exp runtime support.
 * Responsibilities: Target-specific runtime support for aros.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
    FreeBASIC runtime library
    -------------------------

    File: drivers/aros/arm/math_exp.c

    Purpose:

        Select the shared AROS binary64 exp implementation for ARM.

    This file intentionally does NOT contain:

        - the executable approximation algorithm or coefficient tables
        - generic ARM runtime policy
        - behavior for non-AROS targets
*/

#include "../softfloat64/math_exp.c"

/* end of drivers/aros/arm/math_exp.c */
