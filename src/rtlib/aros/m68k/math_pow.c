/*
    FreeBASIC runtime library
    -------------------------

    File: aros/m68k/math_pow.c

    Purpose:

        Select the shared AROS binary64 pow implementation for m68k.

    This file intentionally does NOT contain:

        - the executable approximation algorithm or coefficient tables
        - generic m68k runtime policy
        - an AROS 68000 or soft-float baseline
*/

#include "../softfloat64/math_pow.c"

/* end of aros/m68k/math_pow.c */
