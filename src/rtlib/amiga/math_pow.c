/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/math_pow.c

    Purpose:
        Reuse the verified AROS m68k numeric implementation on classic AmigaOS.

    Responsibilities:
        - select the shared implementation for the same GCC 6.5 toolchain
        - preserve its formatting or binary64 accuracy contract

    This file intentionally does NOT contain:
        - duplicated algorithms or unrelated AmigaOS runtime policy
*/

#include "../aros/m68k/math_pow.c"

/* end of amiga/math_pow.c */

