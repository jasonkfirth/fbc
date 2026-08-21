/*
    FreeBASIC runtime library
    -------------------------

    File: aros/arm/aeabi_l2fp.c

    Purpose:

        Correct AROS ARM 64-bit integer-to-floating-point ABI helpers.

    Responsibilities:

        - return raw IEEE values through the EABI core-register convention
        - preserve exact conversion and round-to-nearest-even behavior
        - override only the four affected libaeabi entry points

    This file intentionally does NOT contain:

        - generic ARM architecture policy
        - AROS code for non-ARM targets
        - unrelated integer division or floating-point helpers

    These EABI helpers deliberately return integer bit patterns in core
    registers, including under the hard-float procedure-call standard.  AROS
    libaeabi normalizes its 64-bit input with __builtin_clzl(), but unsigned
    long is only 32 bits on this target.  The resulting exponent is 32 too
    large.  Strong runtime definitions use __builtin_clzll() and take
    precedence over the SDK archive.  Bit construction also avoids asking GCC
    to call one of these same helpers recursively for a C cast.
*/

#include <stdint.h>

/* ------------------------------------------------------------------------- */
/* Unsigned bit construction                                                 */
/* ------------------------------------------------------------------------- */

uint32_t __aeabi_ul2f( uint64_t value )
{
	uint64_t fraction;
	uint64_t halfway;
	uint64_t remainder;
	unsigned int exponent;
	unsigned int highest_bit;
	unsigned int shift;

	if( value == 0 )
		return 0;

	highest_bit = 63u - (unsigned int)__builtin_clzll( value );
	exponent = highest_bit + 127u;
	if( highest_bit <= 23u ) {
		fraction = value << (23u - highest_bit);
	} else {
		shift = highest_bit - 23u;
		fraction = value >> shift;
		remainder = value & (((uint64_t)1u << shift) - 1u);
		halfway = (uint64_t)1u << (shift - 1u);
		if( remainder > halfway ||
		    (remainder == halfway && (fraction & 1u) != 0) )
		{
			fraction++;
			if( fraction == ((uint64_t)1u << 24u) ) {
				fraction >>= 1u;
				exponent++;
			}
		}
	}

	return (exponent << 23u) | (uint32_t)(fraction & 0x7fffffu);
}

uint64_t __aeabi_ul2d( uint64_t value )
{
	uint64_t fraction;
	uint64_t halfway;
	uint64_t remainder;
	unsigned int exponent;
	unsigned int highest_bit;
	unsigned int shift;

	if( value == 0 )
		return 0;

	highest_bit = 63u - (unsigned int)__builtin_clzll( value );
	exponent = highest_bit + 1023u;
	if( highest_bit <= 52u ) {
		fraction = value << (52u - highest_bit);
	} else {
		shift = highest_bit - 52u;
		fraction = value >> shift;
		remainder = value & (((uint64_t)1u << shift) - 1u);
		halfway = (uint64_t)1u << (shift - 1u);
		if( remainder > halfway ||
		    (remainder == halfway && (fraction & 1u) != 0) )
		{
			fraction++;
			if( fraction == ((uint64_t)1u << 53u) ) {
				fraction >>= 1u;
				exponent++;
			}
		}
	}

	return ((uint64_t)exponent << 52u) |
	       (fraction & UINT64_C( 0x000fffffffffffff ));
}

/* ------------------------------------------------------------------------- */
/* Signed conversions                                                        */
/* ------------------------------------------------------------------------- */

uint32_t __aeabi_l2f( int64_t value )
{
	uint64_t magnitude;

	if( value >= 0 )
		return __aeabi_ul2f( (uint64_t)value );
	magnitude = (uint64_t)(-(value + 1)) + 1u;
	return UINT32_C( 0x80000000 ) | __aeabi_ul2f( magnitude );
}

uint64_t __aeabi_l2d( int64_t value )
{
	uint64_t magnitude;

	if( value >= 0 )
		return __aeabi_ul2d( (uint64_t)value );
	magnitude = (uint64_t)(-(value + 1)) + 1u;
	return UINT64_C( 0x8000000000000000 ) | __aeabi_ul2d( magnitude );
}

/* end of aros/arm/aeabi_l2fp.c */
