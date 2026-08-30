/*
    FreeBASIC runtime Xbox floating-point formatting
    ------------------------------------------------

    File: xbox/str_ftoa.c

    Purpose:

        Format floating-point values for the Xbox runtime and its host-side
        compatibility builds.

    Responsibilities:

        - provide the native Xbox formatter where the C runtime is incomplete
        - preserve the host-side snprintf-based compatibility path
        - implement FreeBASIC's optional leading-space convention

    This file intentionally does NOT contain:

        - string descriptor allocation
        - PRINT or WRITE statement handling
        - floating-point parsing
*/

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

static int hUIntToStr( unsigned long long value, char *buffer )
{
	char temp[32];
	int len = 0;
	int i;

	if( value == 0 ) {
		buffer[0] = '0';
		buffer[1] = '\0';
		return 1;
	}

	while( value > 0 ) {
		temp[len++] = '0' + (value % 10);
		value /= 10;
	}

	for( i = 0; i < len; i++ )
		buffer[i] = temp[len - i - 1];

	buffer[len] = '\0';
	return len;
}

static char *hAppendExponent( char *p, int exponent )
{
	unsigned int abs_exponent;
	char digits[16];
	int len;
	int i;

	*p++ = 'e';
	if( exponent < 0 ) {
		*p++ = '-';
		abs_exponent = (unsigned int)-exponent;
	} else {
		*p++ = '+';
		abs_exponent = (unsigned int)exponent;
	}

	len = hUIntToStr( abs_exponent, digits );
	if( len < 2 )
		*p++ = '0';

	for( i = 0; i < len; i++ )
		*p++ = digits[i];

	return p;
}

static char *hFloat2StrXbox( double val, char *buffer, int digits, int mask )
{
	char digit_buffer[32];
	char *out = buffer;
	long double abs_val;
	long double scaled;
	unsigned long long rounded;
	unsigned long long overflow_limit;
	int exponent = 0;
	int digit_count;
	int decimal_pos;
	int last_digit;
	int use_exponent;
	int i;

	if( mask & FB_F2A_ADDBLANK )
		out++;

	if( val != val ) {
		strcpy( out, "nan" );
	} else {
		if( val < 0.0 ) {
			*out++ = '-';
			abs_val = -(long double)val;
		} else {
			abs_val = (long double)val;
		}

		if( abs_val > (long double)DBL_MAX ) {
			strcpy( out, "inf" );
		} else if( abs_val == 0.0L ) {
			strcpy( out, "0" );
		} else {
			long double normalized = abs_val;

			while( normalized >= 10.0L ) {
				normalized /= 10.0L;
				exponent++;
			}

			while( normalized < 1.0L ) {
				normalized *= 10.0L;
				exponent--;
			}

			if( digits < 1 )
				digits = 1;
			if( digits > 18 )
				digits = 18;

			scaled = hScaleByPow10( abs_val, digits - exponent - 1 );

			rounded = (unsigned long long)(scaled + 0.5L);
			overflow_limit = 1;
			for( i = 0; i < digits; i++ )
				overflow_limit *= 10;

			if( rounded >= overflow_limit ) {
				rounded /= 10;
				exponent++;
			}

			digit_count = hUIntToStr( rounded, digit_buffer );
			decimal_pos = exponent + 1;
			last_digit = digit_count - 1;
			use_exponent = (exponent < -4) || (exponent >= digits);

			if( use_exponent ) {
				while( (last_digit > 0) && (digit_buffer[last_digit] == '0') )
					last_digit--;

				*out++ = digit_buffer[0];
				if( last_digit > 0 ) {
					*out++ = '.';
					for( i = 1; i <= last_digit; i++ )
						*out++ = digit_buffer[i];
				}
				out = hAppendExponent( out, exponent );
				*out = '\0';
			} else {
				while( (last_digit >= decimal_pos) && (digit_buffer[last_digit] == '0') )
					last_digit--;

				if( decimal_pos <= 0 ) {
					*out++ = '0';
					*out++ = '.';
					for( i = 0; i < -decimal_pos; i++ )
						*out++ = '0';
					for( i = 0; i <= last_digit; i++ )
						*out++ = digit_buffer[i];
				} else {
					int digits_before_decimal = decimal_pos;

					for( i = 0; (i < digits_before_decimal) && (i <= last_digit); i++ )
						*out++ = digit_buffer[i];

					while( i < digits_before_decimal ) {
						*out++ = '0';
						i++;
					}

					if( last_digit >= digits_before_decimal ) {
						*out++ = '.';
						for( ; i <= last_digit; i++ )
							*out++ = digit_buffer[i];
					}
				}
				*out = '\0';
			}
		}
	}

	if( (mask & FB_F2A_ADDBLANK) > 0 ) {
		if( buffer[1] != '-' ) {
			buffer[0] = ' ';
			return &buffer[0];
		}
		return &buffer[1];
	}

	return buffer;
}
#endif

char *fb_hFloat2Str( double val, char *buffer, int digits, int mask )
{
#ifdef HOST_XBOX
	return hFloat2StrXbox( val, buffer, digits, mask );
#else
	ssize_t len, maxlen;
	char *p;
	char fmtstr[16];
	const char *fstr;

	if( mask & FB_F2A_ADDBLANK )
		p = &buffer[1];
	else
		p = buffer;

	switch( digits )
	{
	case 7:
		fstr = "%.7g";
		break;
	case 16:
		fstr = "%.16g";
		break;
	default:
		sprintf( fmtstr, "%%.%dg", digits );
		fstr = &fmtstr[0];
	}

	maxlen = 1+digits+6+1;

	len = snprintf( p, maxlen, fstr, val );

	if( len <= 0 || len >= maxlen )
	{
		buffer[0] = '\0';
		return NULL;
	}

	if( len > 0 )
	{
		/* skip the dot at end if any */
		if( len > 0 )
			if( p[len-1] == '.' )
				p[len-1] = '\0';
	}

	/* */
	if( (mask & FB_F2A_ADDBLANK) > 0 )
	{
		if( p[0] != '-' )
		{
			buffer[0] = ' ';
			return &buffer[0];
		}
		else
			return p;
	}
	else
		return p;
#endif
}

/* end of xbox/str_ftoa.c */
