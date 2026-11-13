/*
    Project: FreeBASIC DOS Runtime
    -----------------------------

    File: drivers/dos/math_nearbyint.c

    Purpose:

        Provide the nearbyint entry points used by generated C when the
        DJGPP math library only exports rint and rintf.

    Responsibilities:

        - preserve DJGPP's active floating-point rounding mode
        - provide double- and single-precision nearbyint symbols

    This file intentionally does NOT contain:

        - FreeBASIC numeric conversion logic
        - a general floating-point environment implementation
*/

#include <math.h>

double nearbyint(double value)
{
    return rint(value);
}

float nearbyintf(float value)
{
    return rintf(value);
}

/* end of drivers/dos/math_nearbyint.c */
