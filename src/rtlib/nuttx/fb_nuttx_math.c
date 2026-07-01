/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_math.c

    Purpose:

        Provide small math/libc compatibility helpers for the NuttX
        generated-C smoke target.

    Responsibilities:

        - fill in tiny libm functions that generated C may reference
        - avoid pulling in large platform libraries while the port is young

    This file intentionally does NOT contain:

        - the full FreeBASIC math runtime
        - floating point environment management
        - locale-aware numeric parsing or formatting
*/

static double fb_nuttx_nan(double value)
{
    double zero;

    zero = value - value;

    return zero / zero;
}

double floor(double value)
{
    long long whole;

    /*
        The early loadable-module image keeps math support local to the module
        when possible.  Some ESP32-P4/NuttX libm paths are not safe to call
        from these test modules yet, so avoid routing ordinary BASIC INT()
        lowering through the exported OS math table.
    */
    if (value != value)
        return value;

    if (value >= 9223372036854775807.0)
        return value;

    if (value <= -9223372036854775807.0 - 1.0)
        return value;

    whole = (long long)value;

    if ((value < 0.0) && ((double)whole != value))
        whole--;

    return (double)whole;
}

float floorf(float value)
{
    return (float)floor((double)value);
}

double sqrt(double value)
{
    double scaled;
    double scale;
    double estimate;
    int i;

    /*
        Newton iteration is small and deterministic.  The scaling step keeps
        the starting value in a compact range so ordinary example/test values
        converge quickly without depending on frexp(), ldexp(), or libm.
    */
    if (value != value)
        return value;

    if (value < 0.0)
        return fb_nuttx_nan(value);

    if ((value == 0.0) || (value >= 1.7976931348623157e308))
        return value;

    scaled = value;
    scale = 1.0;

    while (scaled > 4.0) {
        scaled *= 0.25;
        scale *= 2.0;
    }

    while (scaled < 0.25) {
        scaled *= 4.0;
        scale *= 0.5;
    }

    estimate = (scaled >= 1.0) ? scaled : 1.0;

    for (i = 0; i < 24; i++)
        estimate = 0.5 * (estimate + (scaled / estimate));

    return estimate * scale;
}

float sqrtf(float value)
{
    return (float)sqrt((double)value);
}

double nearbyint(double value)
{
    long long whole;

    /*
        NuttX configurations used for the early RISC-V smoke target do not
        always provide nearbyint() from libm.  Generated C can still emit the
        call for intrinsic rounding tests, so provide the default round-to-
        nearest behaviour here.

        This does not attempt to honour alternate floating point rounding
        modes.  That belongs in a fuller libm integration, not in the seed
        microcontroller runtime.
    */
    if (value >= 0.0)
        whole = (long long)(value + 0.5);
    else
        whole = (long long)(value - 0.5);

    return (double)whole;
}

#if !defined(FB_NUTTX_USE_GENERIC_MATH_SGN) || \
    (FB_NUTTX_USE_GENERIC_MATH_SGN == 0)
int32 fb_SGNi(int32 value)
{
    if (value > 0)
        return 1;

    if (value < 0)
        return -1;

    return 0;
}

int32 fb_SGNl(int64_t value)
{
    if (value > 0)
        return 1;

    if (value < 0)
        return -1;

    return 0;
}

int32 fb_SGNd(double value)
{
    if (value > 0.0)
        return 1;

    if (value < 0.0)
        return -1;

    return 0;
}
#endif

/* end of fb_nuttx_math.c */
