/*
    FreeBASIC Sound Library (sfxlib) ARM NEON
    -----------------------------------------

    File: sfx_simd.c

    Purpose:

        Provide runtime-gated NEON PCM conversion kernels for 32-bit ARMv7
        or newer.

    Responsibilities:

        - check the Linux or Android hardware-capability vector for NEON
        - recognize the AROS ARM port's NEON build baseline
        - compile NEON code separately from the generic ARM sound library

    This file intentionally does NOT contain:

        - assumptions that hard-float ARM always includes NEON
        - AArch64 feature policy
        - audio-driver or mixer logic
*/

#include "../sfx_simd.h"

#include <arm_neon.h>

#if defined(__linux__) || defined(__ANDROID__)
#include <sys/auxv.h>

/* Linux assigns bit 12 of the 32-bit ARM AT_HWCAP word to NEON. */
#define FB_SFX_ARM_HWCAP_NEON (1ul << 12)
#endif

unsigned int fb_sfxSimdCapabilities(void)
{
#if defined(__AROS__)
    /* The AROS ARM toolchain baseline is ARMv7-A with NEON-VFPv4. */
    return FB_SFX_SIMD_NEON;
#elif defined(__linux__) || defined(__ANDROID__)
    if ((getauxval(AT_HWCAP) & FB_SFX_ARM_HWCAP_NEON) != 0ul)
        return FB_SFX_SIMD_NEON;

    return 0u;
#else
    /* No portable feature query is available for this ARM operating system. */
    return 0u;
#endif
}

#include "../sfx_simd_neon.inc"

/* end of sfx_simd.c */
