/*
 * Project: FreeBASIC runtime library
 * File: math/math_frac.c
 * Purpose: frac( x ) = x - fix( x ) - returns the fractional part of a float.
 * Responsibilities: Numeric intrinsic implementations and random-number state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL float fb_FRACf( float x )
{
	return x - fb_FIXSingle( x );
}

FBCALL double fb_FRACd( double x )
{
	return x - fb_FIXDouble( x );
}

/* end of math/math_frac.c */
