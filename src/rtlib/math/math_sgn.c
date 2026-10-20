/*
 * Project: FreeBASIC runtime library
 * File: math/math_sgn.c
 * Purpose: SGN function.
 * Responsibilities: Numeric intrinsic implementations and random-number state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"


/*:::::*/
FBCALL int fb_SGNb ( char x )
{
	if( x == 0 )
		return 0;
	else if( x > 0 )
		return 1;
	else
		return -1;
}

/*:::::*/
FBCALL int fb_SGNi ( int x )
{
	if( x == 0 )
		return 0;
	else if( x > 0 )
		return 1;
	else
		return -1;
}

/*:::::*/
FBCALL int fb_SGNl ( long long int x )
{
	if( x == 0 )
		return 0ll;
	else if( x > 0ll )
		return 1;
	else
		return -1;
}

/*:::::*/
FBCALL int fb_SGNSingle ( float x )
{
	if( x == 0.0 )
		return 0;
	else if( x > 0.0 )
		return 1;
	else
		return -1;
}

/*:::::*/
FBCALL int fb_SGNDouble ( double x )
{
	if( x == 0.0 )
		return 0;
	else if( x > 0.0 )
		return 1;
	else
		return -1;
}

/* end of math/math_sgn.c */
