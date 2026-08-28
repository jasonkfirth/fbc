/*
    FreeBASIC Sound Library (sfxlib) x86_64 SIMD
    --------------------------------------------

    File: sfx_simd.c

    Purpose:

        Provide SSE2 PCM conversion and mixer kernels for the x86_64 ABI.

    Responsibilities:

        - declare SSE2 available on every x86_64 target
        - compile the shared conversion and mixer SSE2 kernels

    This file intentionally does NOT contain:

        - optional post-SSE2 instruction use
        - 32-bit x86 CPU probing
        - audio-driver or high-level mixer policy
*/

#include "../sfx_simd.h"

#include <emmintrin.h>

unsigned int fb_sfxSimdCapabilities(void)
{
    /* SSE2 is a required component of the x86_64 ABI. */
    return FB_SFX_SIMD_SSE2;
}

#include "../sfx_simd_sse2.inc"

/* end of sfx_simd.c */
