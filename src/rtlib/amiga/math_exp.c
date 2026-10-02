/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/math_exp.c

    Purpose:
        Reuse the verified AROS m68k numeric implementation on classic AmigaOS.

    Responsibilities:
        - select the shared implementation for the same GCC 6.5 toolchain
        - preserve its formatting or binary64 accuracy contract

    This file intentionally does NOT contain:
        - duplicated algorithms or unrelated AmigaOS runtime policy
*/

#include "../aros/m68k/math_exp.c"

/* end of amiga/math_exp.c */

