/*
    FreeBASIC Windows CE runtime
    ----------------------------

    File: math_pow.c

    Purpose:

        Select the Windows CE binary64 pow implementation.

    Responsibilities:

        * replace the low-accuracy Coredll pow entry point
        * expose the shared binary64 implementation to the runtime build

    This file intentionally does NOT contain:

        * approximation code or coefficient tables
        * compiler constant-folding policy
        * implementations for other operating systems
*/

#include "softfloat64/math_pow.c"

/* end of math_pow.c */
