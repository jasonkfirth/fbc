/*
 * Project: FreeBASIC DOS qualification
 * File: math-control.c
 * Purpose: Verify the DOS tangent precision adapter and caller FPU state.
 * Responsibilities: Check binary64 accuracy and restoration of precision/rounding.
 * This file intentionally does NOT initialize BASIC or replace language tests.
 */

#include "fb.h"
#include <float.h>
#include <stdio.h>

int main( void )
{
	const unsigned int precisions[] = { PC_24, PC_53, PC_64 };
	const unsigned int rounding[] = { RC_NEAR, RC_DOWN, RC_UP, RC_CHOP };
	unsigned int original = _control87( 0, 0 );
	unsigned int before, after;
	size_t p, r;
	volatile double result;
	int failed = 0;

	_control87( PC_64 | RC_NEAR, MCW_PC | MCW_RC );
	result = fb_DosTan( 1.0 );
	/* Nearest binary64 representation of tan(1); the native DJGPP routine
	 * returns the adjacent lower value when run at x87 extended precision.
	 */
	if( result != 1.5574077246549023 ) {
		printf( "incorrect Double tangent: %.17g\n", result );
		failed = 1;
	}
	for( p = 0; p < sizeof(precisions) / sizeof(precisions[0]); ++p ) {
		for( r = 0; r < sizeof(rounding) / sizeof(rounding[0]); ++r ) {
			_control87( precisions[p] | rounding[r], MCW_PC | MCW_RC );
			before = _control87( 0, 0 );
			result = fb_DosTan( 0.25 );
			after = _control87( 0, 0 );
			if( before != after ) {
				printf( "FPU control word changed: %x -> %x\n", before, after );
				failed = 1;
			}
		}
	}
	_control87( original, MCW_PC | MCW_RC );
	if( !failed ) puts( "DOS_MATH_CONTROL=PASS" );
	return failed;
}

/* end of math-control.c */
