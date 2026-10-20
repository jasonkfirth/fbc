/*
 * Project: FreeBASIC runtime library
 * File: drivers/aros/m68k/math_pow_data.c
 * Purpose: Provide math pow data runtime support.
 * Responsibilities: Target-specific runtime support for aros.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
    FreeBASIC runtime library
    -------------------------

    File: drivers/aros/m68k/math_pow_data.c

    Purpose:

        Select the shared AROS binary64 pow tables for m68k.

    This file intentionally does NOT contain:

        - coefficient values or executable math routines
        - generic m68k runtime policy
        - an AROS 68000 or soft-float baseline
*/

#include "../softfloat64/math_pow_data.c"

/* end of drivers/aros/m68k/math_pow_data.c */
