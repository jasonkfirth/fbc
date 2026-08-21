/*
    FreeBASIC Windows CE runtime
    ----------------------------

    File: math_nearbyint.c

    Purpose:

        Supply the nearbyint() functions omitted by the Windows CE C
        runtime used by the CeGCC toolchain.

    Responsibilities:

        * round double-precision values to the nearest integer value
        * round single-precision values to the nearest integer value
        * resolve exact half-way cases using ties-to-even rounding
        * preserve exceptional and already-integral values

    This file intentionally does NOT contain:

        * general FreeBASIC numeric conversion logic
        * floating-point environment or alternate rounding-mode support
        * implementations for platforms whose C runtime supplies nearbyint()
*/

#include <math.h>

/* ------------------------------------------------------------------------- */
/* Double-precision rounding                                                  */
/* ------------------------------------------------------------------------- */

double nearbyint(double value)
{
    double fraction;
    double lower;

    /* Preserve NaN without calling other math functions with it. */
    if (value != value)
        return value;

    lower = floor(value);

    /*
        Infinity, signed zero, and values beyond the fractional precision of
        a double are already integral. Returning floor(value) also preserves
        the sign of negative zero on the CeGCC runtime.
    */
    if (lower == value)
        return lower;

    fraction = value - lower;

    if (fraction < 0.5)
        return lower;

    if (fraction > 0.5)
        return lower + 1.0;

    /*
        floor() makes lower the more-negative candidate for negative inputs.
        Selecting it only when it is even therefore implements ties-to-even
        symmetrically on both sides of zero.
    */
    if (fmod(lower, 2.0) == 0.0)
        return lower;

    return lower + 1.0;
}

/* ------------------------------------------------------------------------- */
/* Single-precision rounding                                                  */
/* ------------------------------------------------------------------------- */

float nearbyintf(float value)
{
    float fraction;
    float lower;

    if (value != value)
        return value;

    lower = floorf(value);

    if (lower == value)
        return lower;

    fraction = value - lower;

    if (fraction < 0.5f)
        return lower;

    if (fraction > 0.5f)
        return lower + 1.0f;

    if (fmod((double)lower, 2.0) == 0.0)
        return lower;

    return lower + 1.0f;
}

/* end of math_nearbyint.c */
