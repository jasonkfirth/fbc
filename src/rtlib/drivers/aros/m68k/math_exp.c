/*
 * Project: FreeBASIC runtime library
 * File: drivers/aros/m68k/math_exp.c
 * Purpose: Provide math exp runtime support.
 * Responsibilities: Target-specific runtime support for aros.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/*
    FreeBASIC runtime library
    -------------------------

    File: drivers/aros/m68k/math_exp.c

    Purpose:

        Select the shared AROS binary64 exp implementation for m68k.

    This file intentionally does NOT contain:

        - the executable approximation algorithm or coefficient tables
        - generic m68k runtime policy
        - an AROS 68000 or soft-float baseline
*/

#include "../softfloat64/math_exp.c"

/* end of drivers/aros/m68k/math_exp.c */
