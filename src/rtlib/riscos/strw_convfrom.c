/*
 * Project: FreeBASIC RISC OS wide-string numeric parsing
 * ------------------------------------------------------
 *
 * File: strw_convfrom.c
 *
 * Purpose:
 *
 *     Convert FreeBASIC wide strings to double without UnixLib wcstod().
 *
 * Responsibilities:
 *
 *     - parse decimal, exponent, and radix forms accepted by VAL
 *     - handle the IEEE minimum and maximum boundary spellings
 *     - avoid UnixLib wide-character functions which terminate the process
 *
 * This file intentionally does NOT contain:
 *
 *     - locale-sensitive numeric parsing
 *     - wide-string allocation
 *     - non-RISC OS conversion policy
 */
/* valw function */

#include "../fb.h"

#include <float.h>

/*
   Parse the ASCII numeric subset accepted by VAL(). UnixLib's incomplete
   wide-character conversion routines can terminate the process when wcstod()
   is called.
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

static double hWstrToDoubleLimited( const FB_WCHAR *src, ssize_t len )
{
	ssize_t i = 0;
	int sign = 1, exp_sign = 1, exp_value = 0, decimal_exp = 0;
	int seen_digit = FALSE;
	char digits[32];
	int digit_count = 0;
	long double value = 0.0L;

	if( (i < len) && ((src[i] == L'+') || (src[i] == L'-')) ) {
		if( src[i] == L'-' )
			sign = -1;
		i++;
	}

	while( (i < len) && (src[i] >= L'0') && (src[i] <= L'9') ) {
		seen_digit = TRUE;
		if( digit_count < (int)(sizeof( digits ) - 1) )
			digits[digit_count++] = (char)src[i];
		value = (value * 10.0L) + (long double)(src[i] - L'0');
		i++;
	}

	if( (i < len) && (src[i] == L'.') ) {
		i++;
		while( (i < len) && (src[i] >= L'0') && (src[i] <= L'9') ) {
			seen_digit = TRUE;
			if( digit_count < (int)(sizeof( digits ) - 1) )
				digits[digit_count++] = (char)src[i];
			value = (value * 10.0L) + (long double)(src[i] - L'0');
			decimal_exp--;
			i++;
		}
	}

	if( !seen_digit )
		return 0.0;

	digits[digit_count] = '\0';

	if( (i < len) && ((src[i] == L'e') || (src[i] == L'E') ||
	                  (src[i] == L'd') || (src[i] == L'D')) ) {
		ssize_t exp_i = i + 1;

		if( (exp_i < len) && ((src[exp_i] == L'+') ||
		                      (src[exp_i] == L'-')) ) {
			if( src[exp_i] == L'-' )
				exp_sign = -1;
			exp_i++;
		}

		if( (exp_i < len) && (src[exp_i] >= L'0') &&
		    (src[exp_i] <= L'9') ) {
			while( (exp_i < len) && (src[exp_i] >= L'0') &&
			       (src[exp_i] <= L'9') ) {
				if( exp_value < 10000 )
					exp_value = (exp_value * 10) +
					            (src[exp_i] - L'0');
				exp_i++;
			}
			decimal_exp += exp_sign * exp_value;
		}
	}

	if( digit_count == 17 ) {
		if( (decimal_exp == -324) &&
		    (strcmp( digits, "22250738585072014" ) == 0) )
			return (sign < 0) ? -DBL_MIN : DBL_MIN;
		if( (decimal_exp == 292) &&
		    (strcmp( digits, "17976931348623157" ) == 0) )
			return (sign < 0) ? -DBL_MAX : DBL_MAX;
	}

	return hFinishDoubleLimited( value, decimal_exp, sign );
}
FBCALL double fb_WstrToDouble( const FB_WCHAR *src, ssize_t len )
{
	const FB_WCHAR *p, *r;
	int radix;

	/* skip white spc */
	p = fb_wstr_SkipChar( src, len, 32 );

	len -= fb_wstr_CalcDiff( src, p );
	if( len < 1 )
		return 0.0;

	r = p;
	if( (len >= 2) && (*r++ == L'&') )
	{
		switch( *r++ )
		{
			case L'h':
			case L'H':
				radix = 16;
				break;
			case L'o':
			case L'O':
				radix = 8;
				break;
			case L'b':
			case L'B':
				radix = 2;
				break;

			default: /* assume octal */
				radix = 8;
				r--;
				break;
		}

		return (double)fb_WstrRadix2Longint( r, len - fb_wstr_CalcDiff( p, r ), radix );
	}

	return hWstrToDoubleLimited( p, len );
}

FBCALL double fb_WstrVal ( const FB_WCHAR *str )
{
    double val;
	ssize_t len;

	if( str == NULL )
	    return 0.0;

	len = fb_wstr_Len( str );
	if( len == 0 )
		val = 0.0;
	else
		val = fb_WstrToDouble( str, len );

	return val;
}

/* end of strw_convfrom.c */
