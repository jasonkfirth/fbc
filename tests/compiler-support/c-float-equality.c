/*
 * Project: FreeBASIC C comparison regression tests
 * File: c-float-equality.c
 * Purpose: Compare old and new emitted operations under the native FP modes.
 * Responsibilities: Check quiet/signaling NaNs, signed zero, adjacent values,
 *     infinities, subnormals, exception flags and single operand evaluation.
 * This file intentionally does NOT redefine equality with a tolerance.
 */
#include <errno.h>
#include <fenv.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int fb_eq_base_d_eq( double left, double right );
int fb_eq_base_d_ne( double left, double right );
int fb_eq_new_d_eq( double left, double right );
int fb_eq_new_d_ne( double left, double right );
int fb_eq_base_s_eq( float left, float right );
int fb_eq_base_s_ne( float left, float right );
int fb_eq_new_s_eq( float left, float right );
int fb_eq_new_s_ne( float left, float right );
int fb_eq_base_d_bool( double value );
int fb_eq_new_d_bool( double value );
int fb_eq_base_d_effect( void );
int fb_eq_new_d_effect( void );
double fb_eq_operand_d( int which );

static double operands[2];
static int evaluations[2];

double fb_eq_operand_d( int which )
{
	if( which < 0 || which > 1 ) return 0;
	evaluations[which]++;
	return operands[which];
}

static int test_double( double left, double right, int unequal )
{
	int before, after, old_flags, new_flags, old_errno;
	errno = 0;
	if( feclearexcept( FE_ALL_EXCEPT ) != 0 ) return 11;
	before = unequal ? fb_eq_base_d_ne( left, right ) : fb_eq_base_d_eq( left, right );
	old_flags = fetestexcept( FE_ALL_EXCEPT );
	old_errno = errno;
	errno = 0;
	if( feclearexcept( FE_ALL_EXCEPT ) != 0 ) return 12;
	after = unequal ? fb_eq_new_d_ne( left, right ) : fb_eq_new_d_eq( left, right );
	new_flags = fetestexcept( FE_ALL_EXCEPT );
	return before == after && old_flags == new_flags && old_errno == errno ? 0 : 13;
}

static int test_single( float left, float right, int unequal )
{
	int before, after, old_flags, new_flags;
	if( feclearexcept( FE_ALL_EXCEPT ) != 0 ) return 21;
	before = unequal ? fb_eq_base_s_ne( left, right ) : fb_eq_base_s_eq( left, right );
	old_flags = fetestexcept( FE_ALL_EXCEPT );
	if( feclearexcept( FE_ALL_EXCEPT ) != 0 ) return 22;
	after = unequal ? fb_eq_new_s_ne( left, right ) : fb_eq_new_s_eq( left, right );
	new_flags = fetestexcept( FE_ALL_EXCEPT );
	return before == after && old_flags == new_flags ? 0 : 23;
}

int main( void )
{
	static const uint64_t double_bits[] = {
		UINT64_C(0), UINT64_C(0x8000000000000000), UINT64_C(0x3ff0000000000000),
		UINT64_C(0xbff0000000000000), UINT64_C(0x3ff0000000000001),
		UINT64_C(0x7fefffffffffffff), UINT64_C(0x7ff0000000000000),
		UINT64_C(0xfff0000000000000), UINT64_C(0x7ff8000000000042),
		UINT64_C(0x7ff0000000000042), UINT64_C(1)
	};
	static const uint32_t single_bits[] = {
		UINT32_C(0), UINT32_C(0x80000000), UINT32_C(0x3f800000),
		UINT32_C(0xbf800000), UINT32_C(0x3f800001), UINT32_C(0x7f7fffff),
		UINT32_C(0x7f800000), UINT32_C(0xff800000), UINT32_C(0x7fc00042),
		UINT32_C(0x7f800042), UINT32_C(1)
	};
	static const int modes[] = { FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO };
	size_t mode, left, right;
	int unequal, result, checks = 0;
	for( mode = 0; mode < sizeof(modes) / sizeof(modes[0]); mode++ ) {
		if( fesetround( modes[mode] ) != 0 ) return 31;
		for( left = 0; left < sizeof(double_bits) / sizeof(double_bits[0]); left++ ) {
			double lhs;
			memcpy( &lhs, &double_bits[left], sizeof(lhs) );
			for( right = 0; right < sizeof(double_bits) / sizeof(double_bits[0]); right++ ) {
				double rhs;
				memcpy( &rhs, &double_bits[right], sizeof(rhs) );
				for( unequal = 0; unequal <= 1; unequal++ ) {
					result = test_double( lhs, rhs, unequal );
					if( result != 0 ) return result;
					checks++;
				}
			}
			if( fb_eq_base_d_bool(lhs) != fb_eq_new_d_bool(lhs) ) return 32;
		}
		for( left = 0; left < sizeof(single_bits) / sizeof(single_bits[0]); left++ ) {
			float lhs;
			memcpy( &lhs, &single_bits[left], sizeof(lhs) );
			for( right = 0; right < sizeof(single_bits) / sizeof(single_bits[0]); right++ ) {
				float rhs;
				memcpy( &rhs, &single_bits[right], sizeof(rhs) );
				for( unequal = 0; unequal <= 1; unequal++ ) {
					result = test_single( lhs, rhs, unequal );
					if( result != 0 ) return result;
					checks++;
				}
			}
		}
	}
	operands[0] = 1;
	operands[1] = 1;
	if( fb_eq_base_d_effect() != -1 || evaluations[0] != 1 || evaluations[1] != 1 ) return 41;
	evaluations[0] = 0;
	evaluations[1] = 0;
	if( fb_eq_new_d_effect() != -1 || evaluations[0] != 1 || evaluations[1] != 1 ) return 42;
	if( fesetround( FE_TONEAREST ) != 0 ) return 43;
	printf( "C_FLOAT_EQUALITY_PASS checks=%d modes=4 single_evaluation=1\n", checks );
	return 0;
}

/* end of c-float-equality.c */
