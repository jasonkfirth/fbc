/*
    FreeBASIC runtime library
    -------------------------

    File: aros/softfloat64/math_pow_data.h

    Purpose:

        Describe the binary64 logarithm tables used by selected AROS pow
        implementations.

    Responsibilities:

        - define the logarithm table dimensions
        - expose immutable coefficients and reduction data

    This file intentionally does NOT contain:

        - table values
        - public math entry points
        - non-AROS target policy
*/

#ifndef FB_AROS_SOFTFLOAT64_MATH_POW_DATA_H
#define FB_AROS_SOFTFLOAT64_MATH_POW_DATA_H

#define POW_LOG_TABLE_BITS 7
#define POW_LOG_POLY_ORDER 8

extern const struct fb_aros_pow_log_data {
	double ln2hi;
	double ln2lo;
	double poly[POW_LOG_POLY_ORDER - 1];
	struct {
		double invc;
		double pad;
		double logc;
		double logctail;
	} tab[1 << POW_LOG_TABLE_BITS];
} fb_aros_pow_log_data;

#endif

/* end of aros/softfloat64/math_pow_data.h */
