/*
 * Project: FreeBASIC runtime library
 * File: drivers/aros/arm/math_exp_data.c
 * Purpose: Provide math exp data runtime support.
 * Responsibilities: Target-specific runtime support for aros.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
    FreeBASIC runtime library
    -------------------------

    File: drivers/aros/arm/math_exp_data.c

    Purpose:

        Select the shared AROS binary64 exp tables for ARM.

    This file intentionally does NOT contain:

        - coefficient values or executable math routines
        - generic ARM runtime policy
        - behavior for non-AROS targets
*/

#include "../softfloat64/math_exp_data.c"

/* end of drivers/aros/arm/math_exp_data.c */
