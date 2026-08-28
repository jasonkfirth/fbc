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
        - accumulate mono voices into an interleaved stereo mix
        - generate simple sustained waveforms in four-frame groups
        - clamp completed float mixer buffers
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

/* ------------------------------------------------------------------------- */
/* Mixer accumulation                                                        */
/* ------------------------------------------------------------------------- */

static void fb_sfxSseAccumulateStereo(__m128 samples,
                                      float *dst,
                                      __m128 gains)
{
    __m128 low = _mm_unpacklo_ps(samples, samples);
    __m128 high = _mm_unpackhi_ps(samples, samples);

    low = _mm_mul_ps(low, gains);
    high = _mm_mul_ps(high, gains);
    _mm_storeu_ps(dst, _mm_add_ps(_mm_loadu_ps(dst), low));
    _mm_storeu_ps(dst + 4, _mm_add_ps(_mm_loadu_ps(dst + 4), high));
}

void fb_sfxMixMonoToStereoSIMD(const float *src,
                               float *dst,
                               int frames,
                               float left_gain,
                               float right_gain)
{
    const __m128 gains = _mm_set_ps(right_gain, left_gain,
                                    right_gain, left_gain);
    int frame = 0;

    if (!src || !dst || frames <= 0)
        return;

    for (; frame <= frames - 4; frame += 4)
    {
        fb_sfxSseAccumulateStereo(_mm_loadu_ps(src + frame),
                                  dst + frame * 2,
                                  gains);
    }

    for (; frame < frames; frame++)
    {
        float sample = src[frame];

        dst[frame * 2] += sample * left_gain;
        dst[frame * 2 + 1] += sample * right_gain;
    }
}

/* ------------------------------------------------------------------------- */
/* Simple oscillator mixer                                                   */
/* ------------------------------------------------------------------------- */

static __m128 fb_sfxSseWaveform(int waveform, __m128 phase)
{
    const __m128 one = _mm_set1_ps(1.0f);
    const __m128 two = _mm_set1_ps(2.0f);
    const __m128 four = _mm_set1_ps(4.0f);

    if (waveform == FB_SFX_WAVE_SQUARE)
    {
        __m128 low = _mm_cmplt_ps(phase, _mm_set1_ps(0.5f));
        __m128 negative_one = _mm_set1_ps(-1.0f);

        return _mm_or_ps(_mm_and_ps(low, one),
                         _mm_andnot_ps(low, negative_one));
    }

    if (waveform == FB_SFX_WAVE_TRIANGLE)
    {
        __m128 four_phase = _mm_mul_ps(phase, four);
        __m128 first = four_phase;
        __m128 middle = _mm_sub_ps(two, four_phase);
        __m128 last = _mm_sub_ps(four_phase, four);
        __m128 first_mask = _mm_cmplt_ps(phase, _mm_set1_ps(0.25f));
        __m128 middle_mask = _mm_cmplt_ps(phase, _mm_set1_ps(0.75f));
        __m128 upper = _mm_or_ps(_mm_and_ps(middle_mask, middle),
                                 _mm_andnot_ps(middle_mask, last));

        return _mm_or_ps(_mm_and_ps(first_mask, first),
                         _mm_andnot_ps(first_mask, upper));
    }

    if (waveform == FB_SFX_WAVE_SAW)
        return _mm_sub_ps(_mm_mul_ps(phase, two), one);

    return _mm_setzero_ps();
}

void fb_sfxMixWaveformToStereoSIMD(int waveform,
                                   float *phase,
                                   float phase_step,
                                   float *dst,
                                   int frames,
                                   float left_gain,
                                   float right_gain)
{
    const __m128 offsets = _mm_set_ps(3.0f, 2.0f, 1.0f, 0.0f);
    const __m128 step = _mm_set1_ps(phase_step);
    const __m128 gains = _mm_set_ps(right_gain, left_gain,
                                    right_gain, left_gain);
    float current_phase;
    int frame;

    if (!phase || !dst || frames <= 0 || phase_step <= 0.0f)
        return;

    current_phase = *phase;
    frame = 0;

    for (; frame <= frames - 4; frame += 4)
    {
        __m128 phases = _mm_add_ps(_mm_set1_ps(current_phase),
                                   _mm_mul_ps(offsets, step));
        __m128 whole = _mm_cvtepi32_ps(_mm_cvttps_epi32(phases));
        __m128 samples;

        phases = _mm_sub_ps(phases, whole);
        samples = fb_sfxSseWaveform(waveform, phases);
        fb_sfxSseAccumulateStereo(samples, dst + frame * 2, gains);

        current_phase += phase_step * 4.0f;
        current_phase -= (float)(int)current_phase;
    }

    for (; frame < frames; frame++)
    {
        float sample = fb_sfxOscillatorWaveSample(waveform, current_phase);

        dst[frame * 2] += sample * left_gain;
        dst[frame * 2 + 1] += sample * right_gain;
        current_phase += phase_step;
        current_phase -= (float)(int)current_phase;
    }

    *phase = current_phase;
}

/* ------------------------------------------------------------------------- */
/* Final mixer clamp                                                         */
/* ------------------------------------------------------------------------- */

void fb_sfxClampFloatBufferSIMD(float *buffer, int samples)
{
    const __m128 minimum = _mm_set1_ps(-1.0f);
    const __m128 maximum = _mm_set1_ps(1.0f);
    int index = 0;

    if (!buffer || samples <= 0)
        return;

    for (; index <= samples - 4; index += 4)
    {
        __m128 value = _mm_loadu_ps(buffer + index);
        __m128 ordered = _mm_cmpord_ps(value, value);
        __m128 clamped = _mm_max_ps(_mm_min_ps(value, maximum), minimum);

        value = _mm_or_ps(_mm_and_ps(ordered, clamped),
                          _mm_andnot_ps(ordered, value));
        _mm_storeu_ps(buffer + index, value);
    }

    for (; index < samples; index++)
    {
        float value = buffer[index];

        if (value > 1.0f)
            value = 1.0f;
        if (value < -1.0f)
            value = -1.0f;
        buffer[index] = value;
    }
}

#endif

/* end of sfx_simd_sse2.inc */
