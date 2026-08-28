/*
    FreeBASIC Sound Library (sfxlib) x86 CPU features
    -------------------------------------------------

    File: sfx_cpu.c

    Purpose:

        Select the safe PCM conversion instruction sets on 32-bit x86.

    Responsibilities:

        - reuse the runtime's established CPUID feature result
        - require both MMX and SSE for the first packed conversion tier
        - report SSE2 separately for the complete conversion backend

    This file intentionally does NOT contain:

        - SIMD intrinsics
        - processor feature overrides
        - conversion arithmetic
*/

#include "../sfx_simd.h"

/* rtlib owns the process-wide x86 feature query used by gfxlib2 as well. */
extern unsigned int fb_CpuDetect(void);

unsigned int fb_sfxSimdCapabilities(void)
{
    const unsigned int mmx = 1u << 23;
    const unsigned int sse = 1u << 25;
    const unsigned int sse2 = 1u << 26;
    unsigned int features = fb_CpuDetect();
    unsigned int capabilities = 0u;

    if ((features & (mmx | sse)) == (mmx | sse))
        capabilities |= FB_SFX_SIMD_MMX_SSE;

    if ((features & (sse | sse2)) == (sse | sse2))
        capabilities |= FB_SFX_SIMD_SSE2;

    return capabilities;
}

/* end of sfx_cpu.c */
