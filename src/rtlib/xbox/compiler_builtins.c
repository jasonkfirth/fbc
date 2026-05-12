/*
    Project: FreeBASIC Xbox Runtime
    --------------------------------

    File: xbox/compiler_builtins.c

    Purpose:

        Provide the small integer helper routines that clang-generated
        32-bit Xbox code expects when lowering 64-bit arithmetic.

    Responsibilities:

        - signed and unsigned 64-bit integer division
        - signed and unsigned 64-bit integer modulo
        - unsigned 64-bit conversion from double

    This file intentionally does NOT contain:

        - FreeBASIC math-library entry points
        - floating-point arithmetic helpers beyond the compiler ABI helper
        - target-independent runtime code
*/

#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------------- */
/* 64-bit integer division helpers                                           */
/* ------------------------------------------------------------------------- */

static uint64_t fb_xbox_udivmod64( uint64_t numerator, uint64_t denominator, uint64_t *remainder )
{
	uint64_t quotient = 0;
	uint64_t current_remainder = 0;
	int bit;

	if( denominator == 0 ) {
		if( remainder != NULL ) {
			*remainder = numerator;
		}
		return 0;
	}

	for( bit = 63; bit >= 0; --bit ) {
		current_remainder <<= 1;
		current_remainder |= (numerator >> bit) & 1;

		if( current_remainder >= denominator ) {
			current_remainder -= denominator;
			quotient |= UINT64_C( 1 ) << bit;
		}
	}

	if( remainder != NULL ) {
		*remainder = current_remainder;
	}

	return quotient;
}

static uint64_t fb_xbox_abs64( int64_t value )
{
	if( value < 0 ) {
		return ((uint64_t)-(value + 1)) + 1;
	}

	return (uint64_t)value;
}

static int64_t fb_xbox_neg64( uint64_t value )
{
	return (int64_t)(~value + 1);
}

int64_t __divdi3( int64_t numerator, int64_t denominator )
{
	uint64_t quotient;

	quotient = fb_xbox_udivmod64( fb_xbox_abs64( numerator ), fb_xbox_abs64( denominator ), NULL );

	if( (numerator < 0) != (denominator < 0) ) {
		return fb_xbox_neg64( quotient );
	}

	return (int64_t)quotient;
}

uint64_t __udivdi3( uint64_t numerator, uint64_t denominator )
{
	return fb_xbox_udivmod64( numerator, denominator, NULL );
}

int64_t __moddi3( int64_t numerator, int64_t denominator )
{
	uint64_t result;

	fb_xbox_udivmod64( fb_xbox_abs64( numerator ), fb_xbox_abs64( denominator ), &result );

	if( numerator < 0 ) {
		return fb_xbox_neg64( result );
	}

	return (int64_t)result;
}

uint64_t __umoddi3( uint64_t numerator, uint64_t denominator )
{
	uint64_t result;

	fb_xbox_udivmod64( numerator, denominator, &result );

	return result;
}

/* ------------------------------------------------------------------------- */
/* Floating point conversion helper                                          */
/* ------------------------------------------------------------------------- */

uint64_t __fixunsdfdi( double value )
{
	uint64_t result = 0;
	uint64_t mask = UINT64_C( 1 ) << 63;
	double bit_value = 9223372036854775808.0; /* 2^63 */

	if( !(value > 0.0) ) {
		return 0;
	}

	if( value >= 18446744073709551616.0 ) { /* 2^64 */
		return UINT64_MAX;
	}

	while( mask != 0 ) {
		if( value >= bit_value ) {
			result |= mask;
			value -= bit_value;
		}

		mask >>= 1;
		bit_value *= 0.5;
	}

	return result;
}

/* end of xbox/compiler_builtins.c */
