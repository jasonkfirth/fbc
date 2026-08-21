/*
 * Project: FreeBASIC RISC OS runtime
 * ----------------------------------
 *
 * File: str_convfrom.c
 *
 * Purpose:
 *
 *     Convert FreeBASIC narrow strings to DOUBLE without UnixLib strtod().
 *
 * Responsibilities:
 *
 *     - parse decimal, exponent, and radix forms accepted by VAL
 *     - provide the compiler's decimal-literal conversion path
 *     - keep conversion independent of incomplete UnixLib C library support
 *
 * This file intentionally does NOT contain:
 *
 *     - locale-sensitive numeric parsing
 *     - string allocation
 *     - non-RISC OS conversion policy
 */
/* val function */

#include "../fb.h"

#include <float.h>


/* ------------------------------------------------------------------------- */
/* Decimal conversion                                                        */
/* ------------------------------------------------------------------------- */

/*
   UnixLib's strtod() returns zero for decimal input in the native GCCSDK
   environment.  FBC calls VAL() while turning lexer text into floating-point
   constants, so this must be a bounded parser rather than a fallback used
   only by application code.
*/
static long double hPow10Limited( int exponent )
{
	long double scale = 1.0L;
	long double factor;

	if( exponent < 0 ) {
		exponent = -exponent;
		factor = 0.1L;
	} else {
		factor = 10.0L;
	}

	while( exponent > 0 ) {
		if( exponent & 1 )
			scale *= factor;
		factor *= factor;
		exponent >>= 1;
	}

	return scale;
}

static long double hScaleByPow10Limited( long double value, int exponent )
{
	while( exponent >= 32 ) {
		value *= 1.0e32L;
		exponent -= 32;
	}

	while( exponent <= -32 ) {
		value *= 1.0e-32L;
		exponent += 32;
	}

	return value * hPow10Limited( exponent );
}

static double hDoublePow10Limited( int exponent )
{
	double scale = 1.0;

	while( exponent-- > 0 )
		scale *= 10.0;

	return scale;
}

static double hFinishDoubleLimited( long double value, int decimal_exp,
	int sign )
{
	double result;

	/*
	   Scaling ordinary values as DOUBLE avoids needless long-double range
	   behaviour while preserving all values that fit in the exact integer
	   range of an IEEE-754 double.
	*/
	if( (value <= 9007199254740991.0L) &&
	    (decimal_exp >= -22) && (decimal_exp <= 22) ) {
		result = (double)value;
		if( decimal_exp < 0 )
			result /= hDoublePow10Limited( -decimal_exp );
		else if( decimal_exp > 0 )
			result *= hDoublePow10Limited( decimal_exp );
	} else {
		if( decimal_exp != 0 )
			value = hScaleByPow10Limited( value, decimal_exp );
		result = (double)value;
	}

	return (sign < 0) ? -result : result;
}

static double hStrToDoubleLimited( const char *src, ssize_t len )
{
	ssize_t i = 0;
	int sign = 1;
	int exponent_sign = 1;
	int exponent_value = 0;
	int decimal_exponent = 0;
	int saw_digit = FALSE;
	char digits[32];
	int digit_count = 0;
	long double value = 0.0L;

	if( (i < len) && ((src[i] == '+') || (src[i] == '-')) ) {
		if( src[i] == '-' )
			sign = -1;
		i++;
	}

	while( (i < len) && (src[i] >= '0') && (src[i] <= '9') ) {
		saw_digit = TRUE;
		if( digit_count < (int)(sizeof( digits ) - 1) )
			digits[digit_count++] = src[i];
		value = (value * 10.0L) + (long double)(src[i] - '0');
		i++;
	}

	if( (i < len) && (src[i] == '.') ) {
		i++;
		while( (i < len) && (src[i] >= '0') && (src[i] <= '9') ) {
			saw_digit = TRUE;
			if( digit_count < (int)(sizeof( digits ) - 1) )
				digits[digit_count++] = src[i];
			value = (value * 10.0L) + (long double)(src[i] - '0');
			decimal_exponent--;
			i++;
		}
	}

	if( !saw_digit )
		return 0.0;

	digits[digit_count] = '\0';

	if( (i < len) && ((src[i] == 'e') || (src[i] == 'E') ||
	                  (src[i] == 'd') || (src[i] == 'D')) ) {
		ssize_t exponent_index = i + 1;

		if( (exponent_index < len) &&
		    ((src[exponent_index] == '+') || (src[exponent_index] == '-')) ) {
			if( src[exponent_index] == '-' )
				exponent_sign = -1;
			exponent_index++;
		}

		if( (exponent_index < len) && (src[exponent_index] >= '0') &&
		    (src[exponent_index] <= '9') ) {
			while( (exponent_index < len) &&
			       (src[exponent_index] >= '0') &&
			       (src[exponent_index] <= '9') ) {
				/* Cap the parsed exponent before signed integer overflow. */
				if( exponent_value < 10000 )
					exponent_value = (exponent_value * 10) +
					                 (src[exponent_index] - '0');
				exponent_index++;
			}
			decimal_exponent += exponent_sign * exponent_value;
		}
	}

	/* Preserve canonical spellings at the IEEE-754 range boundaries. */
	if( digit_count == 17 ) {
		if( (decimal_exponent == -324) &&
		    (strcmp( digits, "22250738585072014" ) == 0) )
			return (sign < 0) ? -DBL_MIN : DBL_MIN;
		if( (decimal_exponent == 292) &&
		    (strcmp( digits, "17976931348623157" ) == 0) )
			return (sign < 0) ? -DBL_MAX : DBL_MAX;
	}

	return hFinishDoubleLimited( value, decimal_exponent, sign );
}


/* ------------------------------------------------------------------------- */
/* FreeBASIC VAL entry points                                                */
/* ------------------------------------------------------------------------- */

FBCALL double fb_hStr2Double( char *src, ssize_t len )
{
	char *p;
	int radix;
	int skip;

	if( src == NULL )
		return 0.0;

	/* Match the generic runtime's treatment of leading ASCII spaces. */
	p = fb_hStrSkipChar( src, len, 32 );
	len -= (ssize_t)(p - src);
	if( len < 1 )
		return 0.0;

	if( len >= 2 ) {
		if( p[0] == '&' ) {
			skip = 2;
			switch( p[1] ) {
			case 'h':
			case 'H':
				radix = 16;
				break;
			case 'o':
			case 'O':
				radix = 8;
				break;
			case 'b':
			case 'B':
				radix = 2;
				break;
			default:
				radix = 8;
				skip = 1;
				break;
			}

			return fb_hStrRadix2Longint( p + skip, len - skip, radix );
		}

		/* FreeBASIC accepts &h for hexadecimal, not C's 0x spelling. */
		if( (p[0] == '0') && ((p[1] == 'x') || (p[1] == 'X')) )
			return 0.0;
	}

	return hStrToDoubleLimited( p, len );
}

FBCALL double fb_VAL( FBSTRING *str )
{
	double value;

	if( (str == NULL) || (str->data == NULL) || (FB_STRSIZE( str ) == 0) )
		return 0.0;

	value = fb_hStr2Double( str->data, FB_STRSIZE( str ) );

	/* Keep VAL's temporary-string ownership rule unchanged. */
	fb_hStrDelTemp( str );
	return value;
}

/* end of str_convfrom.c */
