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
        - accumulate mono voices into an interleaved stereo mix
        - generate simple sustained waveforms in four-frame groups
        - clamp completed float mixer buffers
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

/* ------------------------------------------------------------------------- */
/* Mixer accumulation                                                        */
/* ------------------------------------------------------------------------- */

static void fb_sfxNeonAccumulateStereo(float32x4_t samples,
                                       float *dst,
                                       float32x4_t gains)
{
    float32x4x2_t stereo = vzipq_f32(samples, samples);
    float32x4_t low = vmulq_f32(stereo.val[0], gains);
    float32x4_t high = vmulq_f32(stereo.val[1], gains);

    vst1q_f32(dst, vaddq_f32(vld1q_f32(dst), low));
    vst1q_f32(dst + 4, vaddq_f32(vld1q_f32(dst + 4), high));
}

void fb_sfxMixMonoToStereoSIMD(const float *src,
                               float *dst,
                               int frames,
                               float left_gain,
                               float right_gain)
{
    const float gain_values[4] = {
        left_gain, right_gain, left_gain, right_gain
    };
    const float32x4_t gains = vld1q_f32(gain_values);
    int frame = 0;

    if (!src || !dst || frames <= 0)
        return;

    for (; frame <= frames - 4; frame += 4)
    {
        fb_sfxNeonAccumulateStereo(vld1q_f32(src + frame),
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

static float32x4_t fb_sfxNeonWaveform(int waveform, float32x4_t phase)
{
    const float32x4_t one = vdupq_n_f32(1.0f);
    const float32x4_t two = vdupq_n_f32(2.0f);
    const float32x4_t four = vdupq_n_f32(4.0f);

    if (waveform == FB_SFX_WAVE_SQUARE)
    {
        uint32x4_t low = vcltq_f32(phase, vdupq_n_f32(0.5f));

        return vbslq_f32(low, one, vdupq_n_f32(-1.0f));
    }

    if (waveform == FB_SFX_WAVE_TRIANGLE)
    {
        float32x4_t four_phase = vmulq_f32(phase, four);
        float32x4_t first = four_phase;
        float32x4_t middle = vsubq_f32(two, four_phase);
        float32x4_t last = vsubq_f32(four_phase, four);
        uint32x4_t first_mask = vcltq_f32(phase, vdupq_n_f32(0.25f));
        uint32x4_t middle_mask = vcltq_f32(phase, vdupq_n_f32(0.75f));
        float32x4_t upper = vbslq_f32(middle_mask, middle, last);

        return vbslq_f32(first_mask, first, upper);
    }

    if (waveform == FB_SFX_WAVE_SAW)
        return vsubq_f32(vmulq_f32(phase, two), one);

    return vdupq_n_f32(0.0f);
}

void fb_sfxMixWaveformToStereoSIMD(int waveform,
                                   float *phase,
                                   float phase_step,
                                   float *dst,
                                   int frames,
                                   float left_gain,
                                   float right_gain)
{
    const float offset_values[4] = { 0.0f, 1.0f, 2.0f, 3.0f };
    const float gain_values[4] = {
        left_gain, right_gain, left_gain, right_gain
    };
    const float32x4_t offsets = vld1q_f32(offset_values);
    const float32x4_t step = vdupq_n_f32(phase_step);
    const float32x4_t gains = vld1q_f32(gain_values);
    float current_phase;
    int frame;

    if (!phase || !dst || frames <= 0 || phase_step <= 0.0f)
        return;

    current_phase = *phase;
    frame = 0;

    for (; frame <= frames - 4; frame += 4)
    {
        float32x4_t phases = vaddq_f32(vdupq_n_f32(current_phase),
                                       vmulq_f32(offsets, step));
        int32x4_t whole_integer = vcvtq_s32_f32(phases);
        float32x4_t samples;

        phases = vsubq_f32(phases, vcvtq_f32_s32(whole_integer));
        samples = fb_sfxNeonWaveform(waveform, phases);
        fb_sfxNeonAccumulateStereo(samples, dst + frame * 2, gains);

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
    const float32x4_t minimum = vdupq_n_f32(-1.0f);
    const float32x4_t maximum = vdupq_n_f32(1.0f);
    int index = 0;

    if (!buffer || samples <= 0)
        return;

    for (; index <= samples - 4; index += 4)
    {
        float32x4_t value = vld1q_f32(buffer + index);
        uint32x4_t ordered = vceqq_f32(value, value);
        float32x4_t clamped = vmaxq_f32(vminq_f32(value, maximum), minimum);

        vst1q_f32(buffer + index, vbslq_f32(ordered, clamped, value));
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
