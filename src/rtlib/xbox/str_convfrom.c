/* val function */

#include "../fb.h"

#ifdef HOST_XBOX
#include <float.h>

static long double hPow10( int exponent )
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

static long double hScaleByPow10( long double value, int exponent )
{
	while( exponent >= 32 ) {
		value *= 1.0e32L;
		exponent -= 32;
	}

	while( exponent <= -32 ) {
		value *= 1.0e-32L;
		exponent += 32;
	}

	return value * hPow10( exponent );
}

static double hDoublePow10( int exponent )
{
	double scale = 1.0;

	while( exponent-- > 0 )
		scale *= 10.0;

	return scale;
}

static double hFinishDoubleXbox( long double value, int decimal_exp, int sign )
{
	double result;

	if( (value <= 9007199254740991.0L) &&
	    (decimal_exp >= -22) && (decimal_exp <= 22) ) {
		result = (double)value;
		if( decimal_exp < 0 )
			result /= hDoublePow10( -decimal_exp );
		else if( decimal_exp > 0 )
			result *= hDoublePow10( decimal_exp );
	} else {
		if( decimal_exp != 0 )
			value = hScaleByPow10( value, decimal_exp );
		result = (double)value;
	}

	return (sign < 0) ? -result : result;
}

static double hStrToDoubleXbox( const char *src, ssize_t len )
{
	ssize_t i = 0;
	int sign = 1, exp_sign = 1, exp_value = 0, decimal_exp = 0;
	int seen_digit = FALSE;
	char digits[32];
	int digit_count = 0;
	long double value = 0.0L;

	if( (i < len) && ((src[i] == '+') || (src[i] == '-')) ) {
		if( src[i] == '-' )
			sign = -1;
		i++;
	}

	while( (i < len) && (src[i] >= '0') && (src[i] <= '9') ) {
		seen_digit = TRUE;
		if( digit_count < (int)(sizeof( digits ) - 1) )
			digits[digit_count++] = src[i];
		value = (value * 10.0L) + (long double)(src[i] - '0');
		i++;
	}

	if( (i < len) && (src[i] == '.') ) {
		i++;
		while( (i < len) && (src[i] >= '0') && (src[i] <= '9') ) {
			seen_digit = TRUE;
			if( digit_count < (int)(sizeof( digits ) - 1) )
				digits[digit_count++] = src[i];
			value = (value * 10.0L) + (long double)(src[i] - '0');
			decimal_exp--;
			i++;
		}
	}

	if( !seen_digit )
		return 0.0;

	digits[digit_count] = '\0';

	if( (i < len) && ((src[i] == 'e') || (src[i] == 'E') ||
	                  (src[i] == 'd') || (src[i] == 'D')) ) {
		ssize_t exp_i = i + 1;

		if( (exp_i < len) && ((src[exp_i] == '+') || (src[exp_i] == '-')) ) {
			if( src[exp_i] == '-' )
				exp_sign = -1;
			exp_i++;
		}

		if( (exp_i < len) && (src[exp_i] >= '0') && (src[exp_i] <= '9') ) {
			while( (exp_i < len) && (src[exp_i] >= '0') && (src[exp_i] <= '9') ) {
				if( exp_value < 10000 )
					exp_value = (exp_value * 10) + (src[exp_i] - '0');
				exp_i++;
			}
			decimal_exp += exp_sign * exp_value;
		}
	}

	if( digit_count == 17 ) {
		if( (decimal_exp == -324) && (strcmp( digits, "22250738585072014" ) == 0) )
			return (sign < 0) ? -DBL_MIN : DBL_MIN;
		if( (decimal_exp == 292) && (strcmp( digits, "17976931348623157" ) == 0) )
			return (sign < 0) ? -DBL_MAX : DBL_MAX;
	}

	return hFinishDoubleXbox( value, decimal_exp, sign );
}
#endif

FBCALL double fb_hStr2Double( char *src, ssize_t len )
{
	char *p, *q, c;
	int radix, i, skip;
	double ret;

	/* skip white spc */
	p = fb_hStrSkipChar( src, len, 32 );

	len -= (ssize_t)(p - src);
	if( len < 1 )
		return 0.0;

	else if( len >= 2 ) {
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

			default: /* assume octal */
				radix = 8;
				skip = 1;
				break;
			}

			return fb_hStrRadix2Longint( p + skip, len - skip, radix );

		} else if( p[0] == '0' ) {
			if( p[1] == 'x' || p[1] == 'X' ) {
				/* Filter out strings with 0x/0X prefix -- strtod() treats them as hex.
				   But we only want to support the &h prefix for that. */
				return 0.0; /* 0x would be parsed to the value zero */
			}
		}
	}

#ifdef HOST_XBOX
	return hStrToDoubleXbox( p, len );
#endif

	/* Workaround: strtod() does not allow 'd' as an exponent specifier on 
	 * non-win32 platforms, so create a temporary buffer and replace any 
	 * 'd's with 'e'.
	 * This would be bad for hex strings, but those should be handled above already.
	 */
	q = malloc( len + 1 );
	if( q == NULL )
		return 0.0;
	for( i = 0; i < len; i++ )
	{
		c = p[i];
		if( c == 'd' || c == 'D' ) 
			++c;
		q[i] = c;
	}
	q[len] = '\0';

	ret = strtod( q, NULL );
	free( q );

	return ret;
}

FBCALL double fb_VAL ( FBSTRING *str )
{
    double	val;

	if( str == NULL )
	    return 0.0;

	if( (str->data == NULL) || (FB_STRSIZE( str ) == 0) )
		val = 0.0;
	else
		val = fb_hStr2Double( str->data, FB_STRSIZE( str ) );

	/* del if temp */
	fb_hStrDelTemp( str );

	return val;
}
