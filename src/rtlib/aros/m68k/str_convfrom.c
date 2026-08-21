/*
    FreeBASIC runtime library
    -------------------------

    File: aros/m68k/str_convfrom.c

    Purpose:

        Select the double-precision AROS decimal scanner on soft-float m68k.

    Responsibilities:

        - retain correctly-rounded binary64 conversion
        - avoid unsupported or impractically slow extended-precision helpers
        - reuse the shared AROS numeric syntax and entry points

    This file intentionally does NOT contain:

        - a second decimal-scanner implementation
        - a generic m68k ABI baseline
        - non-AROS target policy

    The AROS 68000 port uses software floating point.  Its compiler advertises
    80-bit long double, but the extended helper path stalls on ordinary INPUT
    values.  FreeBASIC's public result is binary64, so scanning directly at
    DBL_MANT_DIG preserves the required rounding without that dependency.
*/

#define FB_AROS_DECIMAL_VALUE double
#define FB_AROS_DECIMAL_MANT_DIG DBL_MANT_DIG
#define FB_AROS_DECIMAL_MAX_EXP DBL_MAX_EXP
#define FB_AROS_DECIMAL_EPSILON DBL_EPSILON
#define FB_AROS_DECIMAL_MAX DBL_MAX
#define FB_AROS_DECIMAL_MIN DBL_MIN
#define fb_hArosCopySign copysign
#define fb_hArosScaleBinary scalbn
#define fb_hArosRemainder fmod
#define fb_hArosAbsolute fabs

#include "../str_convfrom.c"

/* end of aros/m68k/str_convfrom.c */
