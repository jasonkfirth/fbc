/*
 * Project: FreeBASIC runtime library
 * File: math/math_fix.c
 * Purpose: fix function for singles and doubles FIX( x ) = SGN( x ) * INT( ABS( x ) ).
 * Responsibilities: Numeric intrinsic implementations and random-number state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL float fb_FIXSingle( float x )
{
	return __builtin_floorf( __builtin_fabsf( x ) ) * fb_SGNSingle( x );
}

/*:::::*/
FBCALL double fb_FIXDouble( double x )
{
	return __builtin_floor( __builtin_fabs( x ) ) * fb_SGNDouble( x );
}

/* end of math/math_fix.c */
