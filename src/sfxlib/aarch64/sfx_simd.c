/*
    FreeBASIC Sound Library (sfxlib) AArch64 SIMD
    ---------------------------------------------

    File: sfx_simd.c

    Purpose:

        Provide Advanced SIMD conversion and mixer kernels for AArch64.

    Responsibilities:

        - declare Advanced SIMD available on every AArch64 target
        - compile the shared NEON conversion and mixer kernels

    This file intentionally does NOT contain:

        - optional CPU feature probing
        - ARM32 auxiliary-vector handling
        - audio-driver or high-level mixer policy
*/

#include "../sfx_simd.h"

#include <arm_neon.h>

unsigned int fb_sfxSimdCapabilities(void)
{
    /* Advanced SIMD is a required component of the AArch64 application ABI. */
    return FB_SFX_SIMD_NEON;
}

#include "../sfx_simd_neon.inc"

/* end of sfx_simd.c */
