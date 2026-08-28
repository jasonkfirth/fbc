/*
    FreeBASIC Sound Library (sfxlib) NEON kernels
    ---------------------------------------------

    File: sfx_simd_neon.inc

    Purpose:

        Implement exact PCM conversion kernels shared by ARMv7 and AArch64.

    Responsibilities:

        - preserve the scalar NaN, infinity, and clipping policy
        - convert float PCM to signed 16-bit PCM
        - convert signed 16-bit PCM to float PCM
        - provide exact signed 32-bit conversion on AArch64
        - handle unaligned buffers and scalar tails

    This file intentionally does NOT contain:

        - CPU feature detection
        - platform audio-driver logic
        - an ARMv7 double-precision conversion substitute

    Inclusion contract:

        The including source must include sfx_simd.h and arm_neon.h first.
*/

#ifndef __FB_SFX_SIMD_NEON_INC__
#define __FB_SFX_SIMD_NEON_INC__

/* ------------------------------------------------------------------------- */
/* Shared sample normalization                                               */
/* ------------------------------------------------------------------------- */

static float32x4_t fb_sfxNeonClampFloat(float32x4_t value)
{
    const float32x4_t zero = vdupq_n_f32(0.0f);
    const float32x4_t minimum = vdupq_n_f32(-1.0f);
    const float32x4_t maximum = vdupq_n_f32(1.0f);
    uint32x4_t ordered = vceqq_f32(value, value);

    value = vbslq_f32(ordered, value, zero);
    value = vminq_f32(value, maximum);
    return vmaxq_f32(value, minimum);
}

/* ------------------------------------------------------------------------- */
/* Float to signed 16-bit PCM                                                */
/* ------------------------------------------------------------------------- */

void fb_sfxConvertFloatToS16SIMD(const float *src, short *dst, int samples)
{
    const float32x4_t scale = vdupq_n_f32(32767.0f);
    int i = 0;

    if (!src || !dst || samples <= 0)
        return;

    for (; i <= samples - 8; i += 8)
    {
        float32x4_t low = fb_sfxNeonClampFloat(vld1q_f32(src + i));
        float32x4_t high = fb_sfxNeonClampFloat(vld1q_f32(src + i + 4));
        int32x4_t low_integer;
        int32x4_t high_integer;
        int16x8_t packed;

        low_integer = vcvtq_s32_f32(vmulq_f32(low, scale));
        high_integer = vcvtq_s32_f32(vmulq_f32(high, scale));
        packed = vcombine_s16(vqmovn_s32(low_integer),
            vqmovn_s32(high_integer));
        vst1q_s16(dst + i, packed);
    }

    for (; i < samples; ++i)
        dst[i] = fb_sfxFloatToS16(src[i]);
}

/* ------------------------------------------------------------------------- */
/* Signed 16-bit PCM to float                                                */
/* ------------------------------------------------------------------------- */

void fb_sfxConvertS16ToFloatSIMD(const short *src, float *dst, int samples)
{
    const float32x4_t scale = vdupq_n_f32(1.0f / 32768.0f);
    const int32x4_t positive_full_scale = vdupq_n_s32(32767);
    const float32x4_t one = vdupq_n_f32(1.0f);
    int i = 0;

    if (!src || !dst || samples <= 0)
        return;

    for (; i <= samples - 8; i += 8)
    {
        int16x8_t packed = vld1q_s16(src + i);
        int32x4_t low_integer = vmovl_s16(vget_low_s16(packed));
        int32x4_t high_integer = vmovl_s16(vget_high_s16(packed));
        uint32x4_t low_full_scale = vceqq_s32(low_integer,
            positive_full_scale);
        uint32x4_t high_full_scale = vceqq_s32(high_integer,
            positive_full_scale);
        float32x4_t low = vmulq_f32(vcvtq_f32_s32(low_integer), scale);
        float32x4_t high = vmulq_f32(vcvtq_f32_s32(high_integer), scale);

        low = vbslq_f32(low_full_scale, one, low);
        high = vbslq_f32(high_full_scale, one, high);
        vst1q_f32(dst + i, low);
        vst1q_f32(dst + i + 4, high);
    }

    for (; i < samples; ++i)
        dst[i] = fb_sfxS16ToFloat(src[i]);
}

#if defined(__aarch64__)

/* ------------------------------------------------------------------------- */
/* Float to signed 32-bit PCM                                                */
/* ------------------------------------------------------------------------- */

void fb_sfxConvertFloatToS32SIMD(const float *src, int *dst, int samples)
{
    const float64x2_t scale = vdupq_n_f64(2147483647.0);
    int i = 0;

    if (!src || !dst || samples <= 0)
        return;

    for (; i <= samples - 4; i += 4)
    {
        float32x4_t value = fb_sfxNeonClampFloat(vld1q_f32(src + i));
        float64x2_t low = vcvt_f64_f32(vget_low_f32(value));
        float64x2_t high = vcvt_high_f64_f32(value);
        int64x2_t low_integer;
        int64x2_t high_integer;
        int32x4_t packed;

        low_integer = vcvtq_s64_f64(vmulq_f64(low, scale));
        high_integer = vcvtq_s64_f64(vmulq_f64(high, scale));
        packed = vcombine_s32(vmovn_s64(low_integer),
            vmovn_s64(high_integer));
        vst1q_s32(dst + i, packed);
    }

    for (; i < samples; ++i)
        dst[i] = fb_sfxFloatToS32(src[i]);
}

#endif

#endif

/* end of sfx_simd_neon.inc */
