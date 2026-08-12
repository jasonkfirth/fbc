/*
 * Project: FreeBASIC RISC OS runtime
 * ----------------------------------
 *
 * File: math_pow.c
 *
 * Purpose:
 *
 *     Normalize zero-to-negative powers before using UnixLib's IEEE routine.
 *
 * Responsibilities:
 *
 *     - return positive infinity for positive zero and negative exponents
 *     - preserve negative infinity for negative zero and negative odd integers
 *     - delegate every other case to UnixLib's underlying implementation
 *
 * This file intentionally does NOT replace UnixLib's general power algorithm.
 */

#include "../fb.h"

#include <math.h>

extern double __ieee754_pow( double base, double exponent );

double pow( double base, double exponent )
{
	if( (base == 0.0) && (exponent < 0.0) ) {
		if( signbit( base ) &&
		    (floor( exponent ) == exponent) &&
		    (fmod( fabs( exponent ), 2.0 ) == 1.0) )
			return -__builtin_inf();

		return __builtin_inf();
	}

	return __ieee754_pow( base, exponent );
}

/* end of math_pow.c */
