/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_convert.c

    Purpose:

        Implement shared sample conversion helpers used by platform
        audio drivers.

    Responsibilities:

        - clamp internal float PCM samples to the valid audio range
        - define NaN and infinity handling for driver conversion
        - convert float samples to signed 16-bit and signed 32-bit PCM
        - convert signed 16-bit PCM back to float samples
        - keep platform drivers from carrying subtly different clipping code
        - select safe architecture-specific buffer conversion kernels

    This file intentionally does NOT contain:

        - platform audio API calls
        - mixer logic
        - driver queue management
        - capture device handling
        - SIMD intrinsics or CPU feature probing
*/

#include <limits.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"
#include "sfx_simd.h"

#if (SHRT_MAX != 32767) || (SHRT_MIN != (-32767 - 1))
#error "sfxlib signed 16-bit conversion requires 16-bit short"
#endif

#if (INT_MAX != 2147483647) || (INT_MIN != (-2147483647 - 1))
#error "sfxlib signed 32-bit conversion requires 32-bit int"
#endif


/* ------------------------------------------------------------------------- */
/* Float sample normalization                                                */
/* ------------------------------------------------------------------------- */

/*
    Conversion policy

    The runtime mixer uses float PCM.  Platform drivers often need integer
    PCM, so all edge-case behavior is defined here:

        NaN       -> silence
        +Inf      -> positive full scale
        -Inf      -> negative full scale
        > 1.0     -> positive full scale
        < -1.0    -> negative full scale

    Signed 16-bit output intentionally maps -1.0 to -32767 rather than
    -32768.  Most existing sfxlib drivers already used that symmetric range,
    and preserving it avoids a one-code-value DC asymmetry between backends.
*/

float fb_sfxClampSample(float value)
{
    if (value != value)
        return 0.0f;

    if (value > 1.0f)
        return 1.0f;

    if (value < -1.0f)
        return -1.0f;

    return value;
}


/* ------------------------------------------------------------------------- */
/* Scalar conversion helpers                                                 */
/* ------------------------------------------------------------------------- */

short fb_sfxFloatToS16(float value)
{
    value = fb_sfxClampSample(value);
    return (short)(value * 32767.0f);
}

int fb_sfxFloatToS32(float value)
{
    double scaled;

    value = fb_sfxClampSample(value);
    scaled = (double)value * 2147483647.0;

    return (int)scaled;
}

float fb_sfxS16ToFloat(short value)
{
    if (value <= -32768)
        return -1.0f;

    if (value >= 32767)
        return 1.0f;

    return (float)value / 32768.0f;
}


/* ------------------------------------------------------------------------- */
/* Buffer conversion helpers                                                 */
/* ------------------------------------------------------------------------- */

void fb_sfxConvertFloatToS16(const float *src, short *dst, int samples)
{
    int i;

    if (!src || !dst || samples <= 0)
        return;

#ifdef FB_SFX_HAS_SIMD
    {
        unsigned int capabilities = fb_sfxSimdCapabilities();

        if ((capabilities & (FB_SFX_SIMD_SSE2 | FB_SFX_SIMD_NEON)) != 0u)
        {
            fb_sfxConvertFloatToS16SIMD(src, dst, samples);
            return;
        }
    }
#endif

    for (i = 0; i < samples; ++i)
        dst[i] = fb_sfxFloatToS16(src[i]);
}

void fb_sfxConvertFloatToS32(const float *src, int *dst, int samples)
{
    int i;

    if (!src || !dst || samples <= 0)
        return;

#if defined(__x86_64__) || defined(__aarch64__) || \
    defined(FB_SFX_X86_SIMD)
    if ((fb_sfxSimdCapabilities() &
        (FB_SFX_SIMD_SSE2 | FB_SFX_SIMD_NEON)) != 0u)
    {
        fb_sfxConvertFloatToS32SIMD(src, dst, samples);
        return;
    }
#endif

    for (i = 0; i < samples; ++i)
        dst[i] = fb_sfxFloatToS32(src[i]);
}

void fb_sfxConvertS16ToFloat(const short *src, float *dst, int samples)
{
    int i;

    if (!src || !dst || samples <= 0)
        return;

#ifdef FB_SFX_HAS_SIMD
    {
        unsigned int capabilities = fb_sfxSimdCapabilities();

        if ((capabilities & (FB_SFX_SIMD_SSE2 | FB_SFX_SIMD_NEON)) != 0u)
        {
            fb_sfxConvertS16ToFloatSIMD(src, dst, samples);
            return;
        }

#ifdef FB_SFX_X86_SIMD
        if ((capabilities & FB_SFX_SIMD_MMX_SSE) != 0u)
        {
            fb_sfxConvertS16ToFloatMMX(src, dst, samples);
            return;
        }
#endif
    }
#endif

    for (i = 0; i < samples; ++i)
        dst[i] = fb_sfxS16ToFloat(src[i]);
}


/* end of sfx_convert.c */
