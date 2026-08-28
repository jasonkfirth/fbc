/*
    FreeBASIC Sound Library (sfxlib) x86 MMX/SSE
    --------------------------------------------

    File: sfx_mmx.c

    Purpose:

        Accelerate the useful PCM conversion subset available on processors
        that provide SSE floating-point operations and MMX integer packing.

    Responsibilities:

        - convert signed 16-bit PCM to float PCM in four-sample groups
        - restore the x87 state with EMMS before scalar tail processing

    This file intentionally does NOT contain:

        - float-to-integer conversion, which cannot preserve x87 semantics
        - signed 32-bit conversion, whose exact path requires SSE2 doubles
        - CPU feature probing
*/

#include "../sfx_simd.h"

#include <string.h>
#include <xmmintrin.h>

void fb_sfxConvertS16ToFloatMMX(const short *src, float *dst, int samples)
{
    const __m128 scale = _mm_set1_ps(1.0f / 32768.0f);
    const __m128 positive_full_scale = _mm_set1_ps(32767.0f);
    const __m128 one = _mm_set1_ps(1.0f);
    int i = 0;

    if (!src || !dst || samples <= 0)
        return;

    for (; i <= samples - 4; i += 4)
    {
        unsigned long long packed;
        __m128 high;
        __m128 value;
        __m128 full_scale;

        memcpy(&packed, src + i, sizeof(packed));

        /*
            Some Clang versions lower MMX intrinsics to equivalent SSE2
            instructions when SSE is enabled.  This object must also run on
            the original SSE processors, so keep the integer unpack and the
            MMX-to-SSE transfer explicit.  The build flags disable SSE2 as a
            second, compiler-enforced instruction ceiling.
        */
        __asm__ volatile (
            "movq %2, %%mm0\n\t"
            "movq %%mm0, %%mm1\n\t"
            "psraw $15, %%mm1\n\t"
            "movq %%mm0, %%mm2\n\t"
            "punpcklwd %%mm1, %%mm0\n\t"
            "punpckhwd %%mm1, %%mm2\n\t"
            "xorps %0, %0\n\t"
            "xorps %1, %1\n\t"
            "cvtpi2ps %%mm0, %0\n\t"
            "cvtpi2ps %%mm2, %1\n\t"
            "movlhps %1, %0"
            : "=&x" (value), "=&x" (high)
            : "m" (packed)
            : "mm0", "mm1", "mm2"
        );

        full_scale = _mm_cmpeq_ps(value, positive_full_scale);
        value = _mm_mul_ps(value, scale);
        value = _mm_or_ps(_mm_and_ps(full_scale, one),
            _mm_andnot_ps(full_scale, value));
        _mm_storeu_ps(dst + i, value);
    }

    /* Scalar conversion may use x87 on 32-bit builds. */
    __asm__ volatile ("emms");

    for (; i < samples; ++i)
        dst[i] = fb_sfxS16ToFloat(src[i]);
}

/* end of sfx_mmx.c */
