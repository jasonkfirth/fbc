/*
    Project: FreeBASIC gfxlib2 AArch64 SIMD
    ---------------------------------------

    File: gfx_simd.c

    Purpose:

        Provide the AArch64 entry points for gfxlib2's Advanced SIMD drawing
        kernels.

    Responsibilities:

        - declare Advanced SIMD as available on every AArch64 target
        - compile the shared NEON drawing kernels for the AArch64 ABI

    This file intentionally does NOT contain:

        - optional CPU feature probing
        - ARM32 auxiliary-vector handling
        - framebuffer locking or dirty-line management
*/

#include "../gfx_simd.h"

#include <arm_neon.h>

int fb_hSimdAvailable(void)
{
	/* Advanced SIMD is a required component of the AArch64 application ABI. */
	return TRUE;
}

#include "../gfx_simd_neon.inc"

/* end of gfx_simd.c */
