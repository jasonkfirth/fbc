/*
    FreeBASIC runtime library
    -------------------------

    File: wince/softfloat64/math_private.h

    Purpose:

        Supply the private IEEE helpers used by the Windows CE exp and pow
        implementations.

    Responsibilities:

        - provide exact bit reinterpretation helpers
        - preserve floating-point exception behavior at range boundaries
        - constrain intermediate values to binary64 precision

    This file intentionally does NOT contain:

        - public runtime entry points
        - architecture selection policy
        - generic compiler or ABI policy
*/

#ifndef FB_WINCE_SOFTFLOAT64_MATH_PRIVATE_H
#define FB_WINCE_SOFTFLOAT64_MATH_PRIVATE_H

#include <math.h>
#include <stdint.h>

/* ------------------------------------------------------------------------- */
/* Configuration                                                             */
/* ------------------------------------------------------------------------- */

#define WANT_ROUNDING 1
#define TOINT_INTRINSICS 0

#ifndef __FP_FAST_FMA
#define __FP_FAST_FMA 0
#endif

#if defined(__GNUC__)
#define predict_false(value) __builtin_expect(!!(value), 0)
#else
#define predict_false(value) (value)
#endif

/* ------------------------------------------------------------------------- */
/* Binary64 representation                                                   */
/* ------------------------------------------------------------------------- */

static inline uint64_t asuint64( double value )
{
	union {
		double floating;
		uint64_t integer;
	} converted;

	converted.floating = value;
	return converted.integer;
}

static inline double asdouble( uint64_t value )
{
	union {
		uint64_t integer;
		double floating;
	} converted;

	converted.integer = value;
	return converted.floating;
}

static inline double eval_as_double( double value )
{
	volatile double rounded = value;
	return rounded;
}

/* ------------------------------------------------------------------------- */
/* Floating-point barriers and range results                                 */
/* ------------------------------------------------------------------------- */

static inline double fp_barrier( double value )
{
	volatile double result = value;
	return result;
}

static inline void fp_force_eval( double value )
{
	volatile double result = value;
	(void)result;
}

static inline double __math_xflow( uint32_t sign, double value )
{
	double signed_value = sign ? -value : value;
	return eval_as_double( fp_barrier( signed_value ) * value );
}

static inline double __math_uflow( uint32_t sign )
{
	return __math_xflow( sign, 0x1p-767 );
}

static inline double __math_oflow( uint32_t sign )
{
	return __math_xflow( sign, 0x1p769 );
}

static inline double __math_invalid( double value )
{
	return (value - value) / (value - value);
}

static inline int issignaling_inline( double value )
{
	(void)value;
	return 0;
}

#endif

/* end of wince/softfloat64/math_private.h */
