/*
    FreeBASIC runtime library
    -------------------------

    File: aros/str_convfrom.c

    Purpose:

        Convert FreeBASIC strings to double accurately on AROS.

    Responsibilities:

        - preserve FreeBASIC radix and D-exponent syntax
        - round decimal input correctly on every supported AROS architecture
        - avoid AROS stdc's digit-by-digit fractional accumulation

    This file intentionally does NOT contain:

        - wide-string allocation or conversion
        - architecture-specific floating-point baselines
        - changes to the AROS C library

    The decimal conversion core below is adapted from musl libc's MIT-licensed
    floatscan implementation, Copyright (C) 2005-2020 Rich Felker, et al.  It
    operates on a bounded string cursor here instead of musl's internal FILE
    representation.  AROS stdc currently forms fractions by repeatedly adding
    digit * 0.1^n, which can return an adjacent double for ordinary input such
    as 1.234567890123456.
*/

#include "../fb.h"

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>

/* ------------------------------------------------------------------------- */
/* Decimal scanner configuration                                             */
/* ------------------------------------------------------------------------- */

#ifndef FB_AROS_DECIMAL_VALUE
#define FB_AROS_DECIMAL_VALUE long double
#define FB_AROS_DECIMAL_MANT_DIG LDBL_MANT_DIG
#define FB_AROS_DECIMAL_MAX_EXP LDBL_MAX_EXP
#define FB_AROS_DECIMAL_EPSILON LDBL_EPSILON
#define FB_AROS_DECIMAL_MAX LDBL_MAX
#define FB_AROS_DECIMAL_MIN LDBL_MIN
#define fb_hArosCopySign copysignl
#define fb_hArosScaleBinary scalbnl
#define fb_hArosRemainder fmodl
#define fb_hArosAbsolute fabsl
#endif

#if (FB_AROS_DECIMAL_MANT_DIG == 53) && \
    (FB_AROS_DECIMAL_MAX_EXP == 1024)

#define FB_AROS_LD_B1B_DIG 2
#define FB_AROS_LD_B1B_MAX 9007199, 254740991
#define FB_AROS_KMAX 128

#elif (FB_AROS_DECIMAL_MANT_DIG == 64) && \
      (FB_AROS_DECIMAL_MAX_EXP == 16384)

#define FB_AROS_LD_B1B_DIG 3
#define FB_AROS_LD_B1B_MAX 18, 446744073, 709551615
#define FB_AROS_KMAX 2048

#else
#error Unsupported AROS long double representation
#endif

#define FB_AROS_KMASK (FB_AROS_KMAX - 1)

typedef struct FB_AROS_NUMERIC_INPUT {
	const char *cursor;
	const char *end;
} FB_AROS_NUMERIC_INPUT;

static int hGetNumericCharacter( FB_AROS_NUMERIC_INPUT *input )
{
	int character;

	if( (input->cursor >= input->end) || (*input->cursor == '\0') )
		return -1;

	character = (unsigned char)*input->cursor;
	++input->cursor;
	return character;
}

/* ------------------------------------------------------------------------- */
/* Exponent parsing                                                          */
/* ------------------------------------------------------------------------- */

static long long hScanDecimalExponent( FB_AROS_NUMERIC_INPUT *input )
{
	const char *start;
	long long value;
	int character;
	int negative;

	start = input->cursor;
	character = hGetNumericCharacter( input );
	negative = FALSE;
	if( (character == '+') || (character == '-') )
	{
		negative = (character == '-');
		character = hGetNumericCharacter( input );
	}

	if( (character < '0') || (character > '9') )
	{
		input->cursor = start;
		return LLONG_MIN;
	}

	value = 0;
	do
	{
		if( value < (LLONG_MAX / 100) )
			value = (value * 10) + (character - '0');
		character = hGetNumericCharacter( input );
	} while( (character >= '0') && (character <= '9') );

	return negative ? -value : value;
}

/* ------------------------------------------------------------------------- */
/* Correctly rounded decimal conversion                                      */
/* ------------------------------------------------------------------------- */

static FB_AROS_DECIMAL_VALUE hScanDecimal( FB_AROS_NUMERIC_INPUT *input,
	int character, int sign )
{
	uint32_t chunks[FB_AROS_KMAX];
	static const uint32_t threshold[] = { FB_AROS_LD_B1B_MAX };
	static const int powers_of_ten[] = {
		10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000
	};
	int first_chunk;
	int chunk_count;
	int chunk_digit;
	int index;
	int radix_position;
	int binary_exponent;
	int maximum_exponent;
	int denormal;
	int significant_digits;
	int decimal_digits;
	int last_nonzero_digit;
	int have_digit;
	int have_radix;
	int shift;
	int precision_bits;
	int minimum_exponent;
	long long exponent_ten;
	long long logical_radix_position;
	FB_AROS_DECIMAL_VALUE value;
	FB_AROS_DECIMAL_VALUE fraction;
	FB_AROS_DECIMAL_VALUE bias;

	first_chunk = 0;
	chunk_count = 0;
	chunk_digit = 0;
	logical_radix_position = 0;
	decimal_digits = 0;
	last_nonzero_digit = 0;
	have_digit = FALSE;
	have_radix = FALSE;
	precision_bits = DBL_MANT_DIG;
	minimum_exponent = DBL_MIN_EXP - DBL_MANT_DIG;

	while( character == '0' )
	{
		have_digit = TRUE;
		character = hGetNumericCharacter( input );
	}
	if( character == '.' )
	{
		have_radix = TRUE;
		character = hGetNumericCharacter( input );
		while( character == '0' )
		{
			have_digit = TRUE;
			--logical_radix_position;
			character = hGetNumericCharacter( input );
		}
	}

	chunks[0] = 0;
	while( ((character >= '0') && (character <= '9')) ||
	       (character == '.') )
	{
		if( character == '.' )
		{
			if( have_radix )
				break;
			have_radix = TRUE;
			logical_radix_position = decimal_digits;
		}
		else if( chunk_count < (FB_AROS_KMAX - 3) )
		{
			++decimal_digits;
			if( character != '0' )
				last_nonzero_digit = decimal_digits;
			if( chunk_digit != 0 )
				chunks[chunk_count] = (chunks[chunk_count] * 10) +
					(uint32_t)(character - '0');
			else
				chunks[chunk_count] = (uint32_t)(character - '0');
			if( ++chunk_digit == 9 )
			{
				++chunk_count;
				chunk_digit = 0;
			}
			have_digit = TRUE;
		}
		else
		{
			++decimal_digits;
			if( character != '0' )
			{
				last_nonzero_digit = (FB_AROS_KMAX - 4) * 9;
				chunks[FB_AROS_KMAX - 4] |= 1;
			}
		}
		character = hGetNumericCharacter( input );
	}
	if( !have_radix )
		logical_radix_position = decimal_digits;

	if( have_digit && ((character | 32) == 'e') )
	{
		exponent_ten = hScanDecimalExponent( input );
		if( exponent_ten != LLONG_MIN )
			logical_radix_position += exponent_ten;
	}
	if( !have_digit )
		return (FB_AROS_DECIMAL_VALUE)0.0;

	if( chunks[0] == 0 )
		return sign * (FB_AROS_DECIMAL_VALUE)0.0;

	/* Fast paths also avoid unnecessary big-decimal work for integers. */
	if( (logical_radix_position == decimal_digits) &&
	    (decimal_digits < 10) )
		return sign * (FB_AROS_DECIMAL_VALUE)chunks[0];
	if( logical_radix_position > (-minimum_exponent / 2) )
	{
		errno = ERANGE;
		return sign * FB_AROS_DECIMAL_MAX * FB_AROS_DECIMAL_MAX;
	}
	if( logical_radix_position <
	    (minimum_exponent - (2 * FB_AROS_DECIMAL_MANT_DIG)) )
	{
		errno = ERANGE;
		return sign * FB_AROS_DECIMAL_MIN * FB_AROS_DECIMAL_MIN;
	}

	if( chunk_digit != 0 )
	{
		while( chunk_digit++ < 9 )
			chunks[chunk_count] *= 10;
		++chunk_count;
	}

	first_chunk = 0;
	significant_digits = chunk_count;
	binary_exponent = 0;
	radix_position = (int)logical_radix_position;

	if( (last_nonzero_digit < 9) &&
	    (last_nonzero_digit <= radix_position) && (radix_position < 18) )
	{
		if( radix_position == 9 )
			return sign * (FB_AROS_DECIMAL_VALUE)chunks[0];
		if( radix_position < 9 )
			return sign * (FB_AROS_DECIMAL_VALUE)chunks[0] /
				powers_of_ten[8 - radix_position];
		{
			int bit_limit = precision_bits - (3 * (radix_position - 9));

			if( (bit_limit > 30) || ((chunks[0] >> bit_limit) == 0) )
				return sign * (FB_AROS_DECIMAL_VALUE)chunks[0] *
					powers_of_ten[radix_position - 10];
		}
	}

	while( chunks[significant_digits - 1] == 0 )
		--significant_digits;

	if( (radix_position % 9) != 0 )
	{
		int radix_modulo = (radix_position >= 0) ?
			(radix_position % 9) : ((radix_position % 9) + 9);
		int power = powers_of_ten[8 - radix_modulo];
		uint32_t carry = 0;

		for( index = first_chunk; index != significant_digits; ++index )
		{
			uint32_t remainder = chunks[index] % (uint32_t)power;

			chunks[index] = (chunks[index] / (uint32_t)power) + carry;
			carry = (uint32_t)(1000000000 / power) * remainder;
			if( (index == first_chunk) && (chunks[index] == 0) )
			{
				first_chunk = (first_chunk + 1) & FB_AROS_KMASK;
				radix_position -= 9;
			}
		}
		if( carry != 0 )
			chunks[significant_digits++] = carry;
		radix_position += 9 - radix_modulo;
	}

	while( (radix_position < (9 * FB_AROS_LD_B1B_DIG)) ||
	       ((radix_position == (9 * FB_AROS_LD_B1B_DIG)) &&
	        (chunks[first_chunk] < threshold[0])) )
	{
		uint32_t carry = 0;

		binary_exponent -= 29;
		for( index = (significant_digits - 1) & FB_AROS_KMASK; ;
		     index = (index - 1) & FB_AROS_KMASK )
		{
			uint64_t temporary = ((uint64_t)chunks[index] << 29) + carry;

			if( temporary > 1000000000ULL )
			{
				carry = (uint32_t)(temporary / 1000000000ULL);
				chunks[index] = (uint32_t)(temporary % 1000000000ULL);
			}
			else
			{
				carry = 0;
				chunks[index] = (uint32_t)temporary;
			}
			if( (index == ((significant_digits - 1) & FB_AROS_KMASK)) &&
			    (index != first_chunk) && (chunks[index] == 0) )
				significant_digits = index;
			if( index == first_chunk )
				break;
		}
		if( carry != 0 )
		{
			radix_position += 9;
			first_chunk = (first_chunk - 1) & FB_AROS_KMASK;
			if( first_chunk == significant_digits )
			{
				significant_digits = (significant_digits - 1) & FB_AROS_KMASK;
				chunks[(significant_digits - 1) & FB_AROS_KMASK] |=
					chunks[significant_digits];
			}
			chunks[first_chunk] = carry;
		}
	}

	for( ;; )
	{
		uint32_t carry = 0;

		shift = 1;
		for( index = 0; index < FB_AROS_LD_B1B_DIG; ++index )
		{
			int chunk_index = (first_chunk + index) & FB_AROS_KMASK;

			if( (chunk_index == significant_digits) ||
			    (chunks[chunk_index] < threshold[index]) )
			{
				index = FB_AROS_LD_B1B_DIG;
				break;
			}
			if( chunks[chunk_index] > threshold[index] )
				break;
		}
		if( (index == FB_AROS_LD_B1B_DIG) &&
		    (radix_position == (9 * FB_AROS_LD_B1B_DIG)) )
			break;
		if( radix_position > (9 + (9 * FB_AROS_LD_B1B_DIG)) )
			shift = 9;
		binary_exponent += shift;
		for( index = first_chunk; index != significant_digits;
		     index = (index + 1) & FB_AROS_KMASK )
		{
			uint32_t remainder = chunks[index] & ((1U << shift) - 1U);

			chunks[index] = (chunks[index] >> shift) + carry;
			carry = (1000000000U >> shift) * remainder;
			if( (index == first_chunk) && (chunks[index] == 0) )
			{
				first_chunk = (first_chunk + 1) & FB_AROS_KMASK;
				radix_position -= 9;
			}
		}
		if( carry != 0 )
		{
			if( ((significant_digits + 1) & FB_AROS_KMASK) != first_chunk )
			{
				chunks[significant_digits] = carry;
				significant_digits = (significant_digits + 1) & FB_AROS_KMASK;
			}
			else
				chunks[(significant_digits - 1) & FB_AROS_KMASK] |= 1;
		}
	}

	value = (FB_AROS_DECIMAL_VALUE)0.0;
	for( index = 0; index < FB_AROS_LD_B1B_DIG; ++index )
	{
		int chunk_index = (first_chunk + index) & FB_AROS_KMASK;

		if( chunk_index == significant_digits )
		{
			significant_digits = (significant_digits + 1) & FB_AROS_KMASK;
			chunks[(significant_digits - 1) & FB_AROS_KMASK] = 0;
		}
		value = ((FB_AROS_DECIMAL_VALUE)1000000000.0 * value) +
			chunks[chunk_index];
	}

	value *= sign;
	maximum_exponent = -minimum_exponent - precision_bits + 3;
	denormal = FALSE;
	if( precision_bits > (FB_AROS_DECIMAL_MANT_DIG + binary_exponent -
	                     minimum_exponent) )
	{
		int available_bits = FB_AROS_DECIMAL_MANT_DIG + binary_exponent -
			minimum_exponent;

		if( available_bits < 0 )
			available_bits = 0;
		denormal = TRUE;
		precision_bits = available_bits;
	}

	fraction = (FB_AROS_DECIMAL_VALUE)0.0;
	bias = (FB_AROS_DECIMAL_VALUE)0.0;
	if( precision_bits < FB_AROS_DECIMAL_MANT_DIG )
	{
		bias = fb_hArosCopySign( fb_hArosScaleBinary( 1.0,
			(2 * FB_AROS_DECIMAL_MANT_DIG) - precision_bits - 1 ),
			value );
		fraction = fb_hArosRemainder( value,
			fb_hArosScaleBinary( 1.0,
				FB_AROS_DECIMAL_MANT_DIG - precision_bits ) );
		value -= fraction;
		value += bias;
	}

	if( ((first_chunk + index) & FB_AROS_KMASK) != significant_digits )
	{
		uint32_t tail = chunks[(first_chunk + index) & FB_AROS_KMASK];

		if( (tail < 500000000U) &&
		    ((tail != 0) ||
		     (((first_chunk + index + 1) & FB_AROS_KMASK) != significant_digits)) )
			fraction += (FB_AROS_DECIMAL_VALUE)0.25 * sign;
		else if( tail > 500000000U )
			fraction += (FB_AROS_DECIMAL_VALUE)0.75 * sign;
		else if( tail == 500000000U )
		{
			if( ((first_chunk + index + 1) & FB_AROS_KMASK) == significant_digits )
				fraction += (FB_AROS_DECIMAL_VALUE)0.5 * sign;
			else
				fraction += (FB_AROS_DECIMAL_VALUE)0.75 * sign;
		}
		if( ((FB_AROS_DECIMAL_MANT_DIG - precision_bits) >= 2) &&
		    (fb_hArosRemainder( fraction, 1.0 ) == 0.0) )
			fraction += (FB_AROS_DECIMAL_VALUE)1.0;
	}

	value += fraction;
	value -= bias;

	if( ((binary_exponent + FB_AROS_DECIMAL_MANT_DIG) & INT_MAX) >
	    (maximum_exponent - 5) )
	{
		if( fb_hArosAbsolute( value ) >=
		    (2.0 / FB_AROS_DECIMAL_EPSILON) )
		{
			if( denormal &&
			    (precision_bits == (FB_AROS_DECIMAL_MANT_DIG + binary_exponent -
			               minimum_exponent)) )
				denormal = FALSE;
			value *= (FB_AROS_DECIMAL_VALUE)0.5;
			++binary_exponent;
		}
		if( (binary_exponent + FB_AROS_DECIMAL_MANT_DIG > maximum_exponent) ||
		    (denormal && (fraction != (FB_AROS_DECIMAL_VALUE)0.0)) )
			errno = ERANGE;
	}

	value = fb_hArosScaleBinary( value, binary_exponent );
	if( value == (FB_AROS_DECIMAL_VALUE)0.0 )
		errno = ERANGE;
	return value;
}

/* ------------------------------------------------------------------------- */
/* FreeBASIC conversion entry points                                         */
/* ------------------------------------------------------------------------- */

static double hArosDecimalToDouble( const char *text, ssize_t length )
{
	FB_AROS_NUMERIC_INPUT input;
	int character;
	int sign;

	input.cursor = text;
	input.end = text + length;
	character = hGetNumericCharacter( &input );
	sign = 1;
	if( (character == '+') || (character == '-') )
	{
		if( character == '-' )
			sign = -1;
		character = hGetNumericCharacter( &input );
	}

	if( !(((character >= '0') && (character <= '9')) ||
	      (character == '.')) )
		return strtod( text, NULL );

	return (double)hScanDecimal( &input, character, sign );
}

FBCALL double fb_hStr2Double( char *source, ssize_t length )
{
	char *converted;
	char *text;
	double result;
	int radix;
	int skip;
	ssize_t index;

	text = fb_hStrSkipChar( source, length, 32 );
	length -= (ssize_t)(text - source);
	if( length < 1 )
		return 0.0;

	if( length >= 2 )
	{
		if( text[0] == '&' )
		{
			skip = 2;
			switch( text[1] )
			{
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
			return fb_hStrRadix2Longint( text + skip, length - skip, radix );
		}
		if( (text[0] == '0') && ((text[1] == 'x') || (text[1] == 'X')) )
			return 0.0;
	}

	converted = malloc( (size_t)length + 1 );
	if( converted == NULL )
		return 0.0;
	for( index = 0; index < length; ++index )
	{
		char character = text[index];

		if( (character == 'd') || (character == 'D') )
			++character;
		converted[index] = character;
	}
	converted[length] = '\0';

	result = hArosDecimalToDouble( converted, length );
	free( converted );
	return result;
}

FBCALL double fb_VAL( FBSTRING *string )
{
	double value;

	if( string == NULL )
		return 0.0;

	if( (string->data == NULL) || (FB_STRSIZE( string ) == 0) )
		value = 0.0;
	else
		value = fb_hStr2Double( string->data, FB_STRSIZE( string ) );

	fb_hStrDelTemp( string );
	return value;
}

/* end of aros/str_convfrom.c */
