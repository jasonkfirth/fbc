/*
    Project: FreeBASIC Xbox Runtime
    --------------------------------

    File: xbox/math.c

    Purpose:

        Provide Xbox-local math entry points where the bundled nxdk C
        library is not accurate enough for FreeBASIC's runtime and tests.

    Responsibilities:

        - keep double pow() calculations in double precision
        - provide inverse sine/cosine entry points that agree with the
          compiler's constant-folded results
        - keep the fixes local to the Xbox target

    This file intentionally does NOT contain:

        - generic FreeBASIC math runtime code
        - compiler constant-folding logic
        - graphics or sound backend helpers
*/

#include <math.h>

/* ------------------------------------------------------------------------- */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------- */

static double hNan( void )
{
	volatile double zero = 0.0;

	return zero / zero;
}

static double hInfinity( void )
{
	volatile double zero = 0.0;

	return 1.0 / zero;
}

static int hIsIntegralDouble( double value )
{
	long long integer_value;

	if( value != value ) {
		return 0;
	}

	/*
	    This helper is only used to decide whether a negative base may be
	    raised to a real exponent.  Values outside the exact integer range of
	    a double cannot usefully be classified as odd or even here.
	*/
	if( (value < -9007199254740992.0) || (value > 9007199254740992.0) ) {
		return 0;
	}

	integer_value = (long long)value;

	return ((double)integer_value) == value;
}

static double hPowIntegral( double x, long long exponent )
{
	unsigned long long remaining;
	double factor = x;
	double result = 1.0;
	int invert_result = 0;

	if( exponent < 0 ) {
		remaining = (unsigned long long)(-(exponent + 1)) + 1;
		invert_result = 1;
	} else {
		remaining = (unsigned long long)exponent;
	}

	while( remaining != 0 ) {
		if( (remaining & 1) != 0 ) {
			result *= factor;
		}

		remaining >>= 1;

		if( remaining != 0 ) {
			factor *= factor;
		}
	}

	if( invert_result ) {
		return 1.0 / result;
	}

	return result;
}

static double hPowPositive( double x, double y )
{
	double result;

	/*
	    nxdk already uses x87 for exp() and log(), but routing pow() through
	    both functions rounds the intermediate value enough to miss fbc's
	    double-precision constant-folding checks.  The x87 fyl2x instruction
	    computes y * log2(x) directly on the FPU stack, then the rest mirrors
	    nxdk's exp() sequence for 2^n.
	*/
	__asm__ __volatile__ (
		"fyl2x;"
		"fld %%st(0);"
		"frndint;"
		"fld1;"
		"fscale;"
		"fxch %%st(2);"
		"fsubp %%st(1);"
		"f2xm1;"
		"fld1;"
		"faddp;"
		"fmulp" : "=t"(result) : "0"(x), "u"(y) : "st(1)" );

	return result;
}

/* ------------------------------------------------------------------------- */
/* Inverse trigonometry                                                      */
/* ------------------------------------------------------------------------- */

double asin( double x )
{
	if( (x < -1.0) || (x > 1.0) ) {
		return hNan();
	}

	return atan2( x, sqrt( 1.0 - (x * x) ) );
}

float asinf( float x )
{
	return (float)asin( (double)x );
}

long double asinl( long double x )
{
	return (long double)asin( (double)x );
}

double acos( double x )
{
	if( (x < -1.0) || (x > 1.0) ) {
		return hNan();
	}

	return atan2( sqrt( 1.0 - (x * x) ), x );
}

float acosf( float x )
{
	return (float)acos( (double)x );
}

long double acosl( long double x )
{
	return (long double)acos( (double)x );
}

/* ------------------------------------------------------------------------- */
/* Power functions                                                           */
/* ------------------------------------------------------------------------- */

double pow( double x, double y )
{
	if( y == 0.0 ) {
		return 1.0;
	}

	if( x == 1.0 ) {
		return 1.0;
	}

	if( x == 0.0 ) {
		if( y < 0.0 ) {
			return hInfinity();
		}

		return 0.0;
	}

	if( hIsIntegralDouble( y ) ) {
		return hPowIntegral( x, (long long)y );
	}

	if( x < 0.0 ) {
		return hNan();
	}

	return hPowPositive( x, y );
}

float powf( float x, float y )
{
	return (float)pow( (double)x, (double)y );
}

long double powl( long double x, long double y )
{
	return (long double)pow( (double)x, (double)y );
}

/* end of xbox/math.c */
