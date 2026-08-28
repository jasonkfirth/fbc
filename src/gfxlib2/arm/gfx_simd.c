/*
    Project: FreeBASIC gfxlib2 ARM NEON
    -----------------------------------

    File: gfx_simd.c

    Purpose:

        Provide runtime-gated NEON drawing kernels for 32-bit ARMv7 or newer.

    Responsibilities:

        - check the Linux or Android hardware-capability vector for NEON
        - recognize the AROS ARM port's NEON build baseline
        - compile NEON code separately from the generic ARM runtime

    This file intentionally does NOT contain:

        - assumptions that hard-float ARM always includes NEON
        - AArch64 feature policy
        - framebuffer locking or dirty-line management
*/

#include "../gfx_simd.h"

#include <arm_neon.h>

#if defined(HOST_LINUX) || defined(HOST_ANDROID)
#include <sys/auxv.h>

/* Linux assigns bit 12 of the 32-bit ARM AT_HWCAP word to NEON. */
#define FB_GFX_ARM_HWCAP_NEON (1ul << 12)
#endif

int fb_hSimdAvailable(void)
{
#ifdef HOST_AROS
	/* The AROS ARM toolchain baseline is ARMv7-A with NEON-VFPv4. */
	return TRUE;
#elif defined(HOST_LINUX) || defined(HOST_ANDROID)
	return (getauxval(AT_HWCAP) & FB_GFX_ARM_HWCAP_NEON) != 0ul;
#else
	/* No portable feature query is available for this ARM operating system. */
	return FALSE;
#endif
}

#include "../gfx_simd_neon.inc"

/* end of gfx_simd.c */
