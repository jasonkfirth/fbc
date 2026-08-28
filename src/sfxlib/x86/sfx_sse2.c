/*
    FreeBASIC Sound Library (sfxlib) x86 SSE2
    -----------------------------------------

    File: sfx_sse2.c

    Purpose:

        Compile the complete conversion and mixer backend for SSE2-capable
        32-bit x86 processors.

    Responsibilities:

        - provide the same conversion entry points as the x86_64 backend
        - keep SSE2 instructions isolated from baseline x86 objects

    This file intentionally does NOT contain:

        - CPU feature probing
        - MMX/SSE fallback conversion
        - audio-driver or high-level mixer policy
*/

#include "../sfx_simd.h"

#include <emmintrin.h>

#include "../sfx_simd_sse2.inc"

/* end of sfx_sse2.c */
