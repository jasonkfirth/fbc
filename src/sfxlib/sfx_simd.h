/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_simd.h

    Purpose:

        Define the private interface for architecture-specific PCM sample
        conversion kernels.

    Responsibilities:

        - identify builds that contain an sfxlib SIMD backend
        - describe the instruction sets available at run time
        - declare accelerated buffer conversion entry points

    This file intentionally does NOT contain:

        - instruction-set intrinsics
        - platform audio-driver code
        - public FreeBASIC declarations
*/

#ifndef __FB_SFX_SIMD_H__
#define __FB_SFX_SIMD_H__

#include "fb_sfx_internal.h"

#define FB_SFX_SIMD_MMX_SSE 0x01u
#define FB_SFX_SIMD_SSE2    0x02u
#define FB_SFX_SIMD_NEON    0x04u

#if defined(__x86_64__) || defined(__aarch64__) || \
    defined(FB_SFX_X86_SIMD) || defined(FB_SFX_ARM_NEON)
#define FB_SFX_HAS_SIMD
#endif

#ifdef FB_SFX_HAS_SIMD

unsigned int fb_sfxSimdCapabilities(void);

void fb_sfxConvertFloatToS16SIMD(const float *src, short *dst, int samples);
void fb_sfxConvertS16ToFloatSIMD(const short *src, float *dst, int samples);

#if defined(__x86_64__) || defined(__aarch64__) || \
    defined(FB_SFX_X86_SIMD)
void fb_sfxConvertFloatToS32SIMD(const float *src, int *dst, int samples);
#endif

#ifdef FB_SFX_X86_SIMD
void fb_sfxConvertS16ToFloatMMX(const short *src, float *dst, int samples);
#endif

#endif

#endif

/* end of sfx_simd.h */
