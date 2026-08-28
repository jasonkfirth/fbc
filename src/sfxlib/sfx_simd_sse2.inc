/*
    FreeBASIC Sound Library (sfxlib) SSE2 kernels
    ---------------------------------------------

    File: sfx_simd_sse2.inc

    Purpose:

        Implement exact PCM conversion kernels shared by 32-bit and 64-bit
        x86 SSE2 backends.

    Responsibilities:

        - preserve the target's scalar floating-point evaluation policy
        - preserve the scalar NaN, infinity, and clipping policy
        - convert float PCM to signed 16-bit and signed 32-bit PCM
        - convert signed 16-bit PCM to float PCM
        - handle unaligned buffers and scalar tails

    This file intentionally does NOT contain:

        - CPU feature detection
        - MMX state management
        - public sample conversion dispatch

    Inclusion contract:

        The including source must include sfx_simd.h and emmintrin.h first.
*/

#ifndef __FB_SFX_SIMD_SSE2_INC__
#define __FB_SFX_SIMD_SSE2_INC__

#include <float.h>

/* ------------------------------------------------------------------------- */
/* Shared sample normalization                                               */
/* ------------------------------------------------------------------------- */

static __m128 fb_sfxSimdClampFloat(__m128 value)
{
    const __m128 minimum = _mm_set1_ps(-1.0f);
    const __m128 maximum = _mm_set1_ps(1.0f);
    __m128 ordered = _mm_cmpord_ps(value, value);

    value = _mm_and_ps(value, ordered);
    value = _mm_min_ps(value, maximum);
    return _mm_max_ps(value, minimum);
}

/* ------------------------------------------------------------------------- */
/* Float to signed 16-bit PCM                                                */
/* ------------------------------------------------------------------------- */

void fb_sfxConvertFloatToS16SIMD(const float *src, short *dst, int samples)
{
#if FLT_EVAL_METHOD > 0
    const __m128d scale = _mm_set1_pd(32767.0);
#else
    const __m128 scale = _mm_set1_ps(32767.0f);
#endif
    int i = 0;

    if (!src || !dst || samples <= 0)
        return;

#if FLT_EVAL_METHOD > 0
    /*
        Baseline 32-bit x86 evaluates float expressions in x87 extended
        precision.  Converting through double retains the exact product of a
        float and 32767, which matches that scalar result before truncation.
    */
    for (; i <= samples - 4; i += 4)
    {
        __m128 value = fb_sfxSimdClampFloat(_mm_loadu_ps(src + i));
        __m128 high_float = _mm_movehl_ps(value, value);
        __m128d low = _mm_cvtps_pd(value);
        __m128d high = _mm_cvtps_pd(high_float);
        __m128i low_integer;
        __m128i high_integer;
        __m128i integer;
        __m128i packed;

        low_integer = _mm_cvttpd_epi32(_mm_mul_pd(low, scale));
        high_integer = _mm_cvttpd_epi32(_mm_mul_pd(high, scale));
        integer = _mm_unpacklo_epi64(low_integer, high_integer);
        packed = _mm_packs_epi32(integer, integer);
        _mm_storel_epi64((__m128i *)(dst + i), packed);
    }
#else
    for (; i <= samples - 8; i += 8)
    {
        __m128 low = fb_sfxSimdClampFloat(_mm_loadu_ps(src + i));
        __m128 high = fb_sfxSimdClampFloat(_mm_loadu_ps(src + i + 4));
        __m128i low_integer;
        __m128i high_integer;
        __m128i packed;

        low_integer = _mm_cvttps_epi32(_mm_mul_ps(low, scale));
        high_integer = _mm_cvttps_epi32(_mm_mul_ps(high, scale));
        packed = _mm_packs_epi32(low_integer, high_integer);
        _mm_storeu_si128((__m128i *)(dst + i), packed);
    }
#endif

    for (; i < samples; ++i)
        dst[i] = fb_sfxFloatToS16(src[i]);
}

/* ------------------------------------------------------------------------- */
/* Float to signed 32-bit PCM                                                */
/* ------------------------------------------------------------------------- */

void fb_sfxConvertFloatToS32SIMD(const float *src, int *dst, int samples)
{
    const __m128d scale = _mm_set1_pd(2147483647.0);
    int i = 0;

    if (!src || !dst || samples <= 0)
        return;

    for (; i <= samples - 4; i += 4)
    {
        __m128 value = fb_sfxSimdClampFloat(_mm_loadu_ps(src + i));
        __m128 high_float = _mm_movehl_ps(value, value);
        __m128d low = _mm_cvtps_pd(value);
        __m128d high = _mm_cvtps_pd(high_float);
        __m128i low_integer;
        __m128i high_integer;
        __m128i packed;

        low_integer = _mm_cvttpd_epi32(_mm_mul_pd(low, scale));
        high_integer = _mm_cvttpd_epi32(_mm_mul_pd(high, scale));
        packed = _mm_unpacklo_epi64(low_integer, high_integer);
        _mm_storeu_si128((__m128i *)(dst + i), packed);
    }

    for (; i < samples; ++i)
        dst[i] = fb_sfxFloatToS32(src[i]);
}

/* ------------------------------------------------------------------------- */
/* Signed 16-bit PCM to float                                                */
/* ------------------------------------------------------------------------- */

void fb_sfxConvertS16ToFloatSIMD(const short *src, float *dst, int samples)
{
    const __m128 scale = _mm_set1_ps(1.0f / 32768.0f);
    const __m128 positive_full_scale = _mm_set1_ps(32767.0f);
    const __m128 one = _mm_set1_ps(1.0f);
    int i = 0;

    if (!src || !dst || samples <= 0)
        return;

    for (; i <= samples - 8; i += 8)
    {
        __m128i packed = _mm_loadu_si128((const __m128i *)(src + i));
        __m128i sign = _mm_srai_epi16(packed, 15);
        __m128i low_integer = _mm_unpacklo_epi16(packed, sign);
        __m128i high_integer = _mm_unpackhi_epi16(packed, sign);
        __m128 low = _mm_cvtepi32_ps(low_integer);
        __m128 high = _mm_cvtepi32_ps(high_integer);
        __m128 low_full_scale = _mm_cmpeq_ps(low, positive_full_scale);
        __m128 high_full_scale = _mm_cmpeq_ps(high, positive_full_scale);

        low = _mm_mul_ps(low, scale);
        high = _mm_mul_ps(high, scale);
        low = _mm_or_ps(_mm_and_ps(low_full_scale, one),
            _mm_andnot_ps(low_full_scale, low));
        high = _mm_or_ps(_mm_and_ps(high_full_scale, one),
            _mm_andnot_ps(high_full_scale, high));
        _mm_storeu_ps(dst + i, low);
        _mm_storeu_ps(dst + i + 4, high);
    }

    for (; i < samples; ++i)
        dst[i] = fb_sfxS16ToFloat(src[i]);
}

#endif

/* end of sfx_simd_sse2.inc */
