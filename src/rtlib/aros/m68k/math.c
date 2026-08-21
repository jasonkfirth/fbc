/*
    FreeBASIC runtime library
    -------------------------

    File: aros/m68k/math.c

    Purpose:

        Disable the generic AROS wider-long-double math route on m68k.

    Responsibilities:

        - replace aros/math.c through source-graph precedence
        - allow the m68k binary64 exp and pow objects to own their symbols

    This file intentionally does NOT contain:

        - generic m68k math policy
        - an AROS 68000 or soft-float baseline
        - executable math routines or coefficient tables

    Toolchain behavior:

        AROS m68k advertises an 80-bit long double, but its soft-float helpers
        do not provide a usable route through expl() and powl().  The selected
        binary64 implementations live in the sibling m68k files and share
        their algorithms with the AROS ARM port.
*/

#include "../../fb.h"

/* end of aros/m68k/math.c */
