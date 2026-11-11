/*
    Project: FreeBASIC compiler regression tests
    --------------------------------------------

    File: llvm-duplicate-extern-global.c

    Purpose:

        Provide the C-defined global imported by the LLVM duplicate-global
        smoke test.

    Responsibilities:

        - Define one integer symbol for two BASIC aliases to reference

    This file intentionally does NOT contain:

        - LLVM test driver logic
        - C runtime dependencies
*/

int fb_llvm_duplicate_extern_global = 42;

/* end of llvm-duplicate-extern-global.c */
