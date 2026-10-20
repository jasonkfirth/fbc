/*
 * Project: FreeBASIC runtime library
 * File: math/math_cvn.c
 * Purpose: CV# numeric routines.
 * Responsibilities: Numeric intrinsic implementations and random-number state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

#define hDoCVn(from, to_t, size) do {                         \
	if( (size) == sizeof(from) && (size) == sizeof(to_t) )    \
	{                                                         \
		to_t to;                                              \
		memcpy( &to, &from, size );                           \
		return to;                                            \
	}                                                         \
	else                                                      \
	{                                                         \
		return (to_t)0;                                       \
	}                                                         \
 } while (0)


FBCALL double fb_CVDFROMLONGINT( long long ll )
{
	hDoCVn( ll, double, 8 );
}

FBCALL float fb_CVSFROML( int l )
{
	hDoCVn( l, float, 4 );
}

FBCALL int fb_CVLFROMS( float f )
{
	hDoCVn( f, int, 4 );
}

FBCALL long long fb_CVLONGINTFROMD( double d )
{
	hDoCVn( d, long long, 8 );
}

/* end of math/math_cvn.c */
