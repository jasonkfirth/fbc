/*
    FreeBASIC runtime library
    -------------------------

    File: wince/softfloat64/math_exp_data.h

    Purpose:

        Describe the binary64 reduction tables shared by the Windows CE exp
        and pow implementations.

    Responsibilities:

        - define the reduction table dimensions
        - expose immutable coefficient and scale data

    This file intentionally does NOT contain:

        - table values
        - public math entry points
        - non-Windows CE target policy
*/

#ifndef FB_WINCE_SOFTFLOAT64_MATH_EXP_DATA_H
#define FB_WINCE_SOFTFLOAT64_MATH_EXP_DATA_H

#include <stdint.h>

#define EXP_TABLE_BITS 7
#define EXP_POLY_ORDER 5
#define EXP_USE_TOINT_NARROW 0
#define EXP2_POLY_ORDER 5

extern const struct fb_wince_exp_data {
	double invln2N;
	double shift;
	double negln2hiN;
	double negln2loN;
	double poly[4];
	double exp2_shift;
	double exp2_poly[EXP2_POLY_ORDER];
	uint64_t tab[2 * (1 << EXP_TABLE_BITS)];
} fb_wince_exp_data;

#endif

/* end of wince/softfloat64/math_exp_data.h */
