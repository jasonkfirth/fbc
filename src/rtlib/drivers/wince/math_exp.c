/*
    Project: FreeBASIC Windows CE runtime
    ----------------------------

    File: drivers/wince/math_exp.c

    Purpose:

        Select the Windows CE binary64 exp implementation.

    Responsibilities:

        * replace the low-accuracy Coredll exp entry point
        * expose the shared binary64 implementation to the runtime build

    This file intentionally does NOT contain: * approximation code or coefficient tables
        * compiler constant-folding policy
        * implementations for other operating systems
*/

#include "softfloat64/math_exp.c"

/* end of drivers/wince/math_exp.c */
