/*
 * Project: FreeBASIC RISC OS numeric bit conversion
 * -------------------------------------------------
 *
 * File: math_cvn.c
 *
 * Purpose:
 *
 *     Implement CV numeric conversions for the APCS double layout.
 *
 * Responsibilities:
 *
 *     - convert canonical 64-bit patterns to APCS doubles
 *     - convert APCS doubles to canonical 64-bit patterns
 *     - retain the normal single and integer conversion behavior
 *
 * This file intentionally does NOT contain:
 *
 *     - floating-point arithmetic
 *     - string packing
 *     - non-RISC OS representation policy
 */
/* CV# numeric routines */

#include "../fb.h"

#include "fb_double_riscos.h"

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
	return fb_riscos_DoubleFromBits( (uint64_t)ll );
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
	return (long long)fb_riscos_DoubleToBits( d );
}

/* end of math_cvn.c */
