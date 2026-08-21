/*
    FreeBASIC runtime library
    -------------------------

    File: aros/m68k/float_conv.c

    Purpose:

        Supply missing IEEE special-value behavior for the AROS m68k
        single/double compiler helpers.

    Responsibilities:

        - convert binary32 and binary64 bit patterns in both directions
        - round finite values to nearest with ties to even
        - preserve zero signs, infinities, and representative NaN payloads
        - return infinity when a finite double exceeds the single range
        - canonicalize NaNs returned by the AROS m68k division helpers

    This file intentionally does NOT contain:

        - a generic m68k ISA or ABI baseline
        - AROS compiler-driver flags
        - floating-point arithmetic that would recurse into libgcc helpers

    AROS toolchain behavior:

        GCC's AROS 68000 soft-float libgcc helper wraps overflowing binary64
        exponents instead of producing binary32 infinity.  Its inverse helper
        then treats binary32 infinity as an ordinary exponent.  Generated
        programs call the standard __truncdfsf2 and __extendsfdf2 symbols.  The
        AROS m68k linker policy redirects those calls to these bit-exact
        replacements while leaving libgcc's grouped helper object available
        for its other operations.  Other m68k targets remain free to use their
        toolchain implementation.
*/

#include "../../fb.h"

typedef union FB_AROS_M68K_DOUBLE_BITS {
	double value;
	uint64_t bits;
} FB_AROS_M68K_DOUBLE_BITS;

typedef union FB_AROS_M68K_SINGLE_BITS {
	float value;
	uint32_t bits;
} FB_AROS_M68K_SINGLE_BITS;

extern double __real___divdf3( double dividend, double divisor );
extern float __real___divsf3( float dividend, float divisor );

static uint64_t fb_hArosRoundToEven( uint64_t significand, int shift )
{
	uint64_t halfway;
	uint64_t remainder;
	uint64_t result;

	if( shift >= 64 )
		return 0;

	result = significand >> shift;
	halfway = (uint64_t)1 << (shift - 1);
	remainder = significand & (((uint64_t)1 << shift) - 1);
	if( (remainder > halfway) ||
	    ((remainder == halfway) && ((result & 1) != 0)) )
	{
		++result;
	}

	return result;
}

double __wrap___divdf3( double dividend, double divisor )
{
	FB_AROS_M68K_DOUBLE_BITS result;

	result.value = __real___divdf3( dividend, divisor );
	if( (result.bits & UINT64_C( 0x7ff0000000000000 )) ==
	    UINT64_C( 0x7ff0000000000000 ) &&
	    (result.bits & UINT64_C( 0x000fffffffffffff )) != 0 )
	{
		result.bits = UINT64_C( 0x7ff8000000000000 );
	}

	return result.value;
}

float __wrap___divsf3( float dividend, float divisor )
{
	FB_AROS_M68K_SINGLE_BITS result;

	result.value = __real___divsf3( dividend, divisor );
	if( (result.bits & UINT32_C( 0x7f800000 )) ==
	    UINT32_C( 0x7f800000 ) &&
	    (result.bits & UINT32_C( 0x007fffff )) != 0 )
	{
		result.bits = UINT32_C( 0x7fc00000 );
	}

	return result.value;
}

double __wrap___extendsfdf2( float source )
{
	FB_AROS_M68K_DOUBLE_BITS output;
	FB_AROS_M68K_SINGLE_BITS input;
	uint32_t exponent;
	uint32_t fraction;
	uint64_t sign;
	int unbiased_exponent;

	input.value = source;
	sign = ((uint64_t)(input.bits & UINT32_C( 0x80000000 ))) << 32;
	exponent = (input.bits >> 23) & UINT32_C( 0xff );
	fraction = input.bits & UINT32_C( 0x007fffff );

	if( exponent == UINT32_C( 0xff ) )
	{
		output.bits = sign | UINT64_C( 0x7ff0000000000000 );
		if( fraction != 0 )
			output.bits |= ((uint64_t)fraction) << 29;
		return output.value;
	}

	if( exponent == 0 )
	{
		if( fraction == 0 )
		{
			output.bits = sign;
			return output.value;
		}

		unbiased_exponent = -126;
		while( (fraction & UINT32_C( 0x00800000 )) == 0 )
		{
			fraction <<= 1;
			--unbiased_exponent;
		}
		fraction &= UINT32_C( 0x007fffff );
	}
	else
	{
		unbiased_exponent = (int)exponent - 127;
	}

	output.bits = sign |
		((uint64_t)(unbiased_exponent + 1023) << 52) |
		((uint64_t)fraction << 29);
	return output.value;
}

float __wrap___truncdfsf2( double source )
{
	FB_AROS_M68K_DOUBLE_BITS input;
	FB_AROS_M68K_SINGLE_BITS output;
	uint64_t fraction;
	uint64_t rounded;
	uint32_t exponent;
	uint32_t sign;
	int unbiased_exponent;
	int shift;

	input.value = source;
	sign = (uint32_t)(input.bits >> 32) & UINT32_C( 0x80000000 );
	exponent = (uint32_t)(input.bits >> 52) & UINT32_C( 0x7ff );
	fraction = input.bits & UINT64_C( 0x000fffffffffffff );

	if( exponent == UINT32_C( 0x7ff ) )
	{
		output.bits = sign | UINT32_C( 0x7f800000 );
		if( fraction != 0 )
		{
			uint32_t payload = (uint32_t)(fraction >> 29);

			if( payload == 0 )
				payload = 1;
			output.bits |= payload;
		}
		return output.value;
	}

	if( exponent == 0 )
	{
		output.bits = sign;
		return output.value;
	}

	unbiased_exponent = (int)exponent - 1023;
	if( unbiased_exponent > 127 )
	{
		output.bits = sign | UINT32_C( 0x7f800000 );
		return output.value;
	}

	fraction |= UINT64_C( 0x0010000000000000 );
	if( unbiased_exponent >= -126 )
	{
		rounded = fb_hArosRoundToEven( fraction, 29 );
		if( rounded == UINT64_C( 0x01000000 ) )
		{
			rounded >>= 1;
			++unbiased_exponent;
			if( unbiased_exponent > 127 )
			{
				output.bits = sign | UINT32_C( 0x7f800000 );
				return output.value;
			}
		}

		output.bits = sign |
			((uint32_t)(unbiased_exponent + 127) << 23) |
			((uint32_t)rounded & UINT32_C( 0x007fffff ));
		return output.value;
	}

	if( unbiased_exponent < -150 )
	{
		output.bits = sign;
		return output.value;
	}

	shift = -97 - unbiased_exponent;
	rounded = fb_hArosRoundToEven( fraction, shift );
	output.bits = sign | (uint32_t)rounded;
	return output.value;
}

/* end of aros/m68k/float_conv.c */
