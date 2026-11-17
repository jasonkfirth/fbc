/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_convfrom.c
 * Purpose: valw function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <stdint.h>

FBCALL double fb_WstrToDouble( const FB_WCHAR *src, ssize_t len )
{
	const FB_WCHAR *p, *r;
	int radix;
	ssize_t i;
	size_t alloc_chars;
	long long radix_value;
	FB_WCHAR *q, c;
	double ret;

	if( (src == NULL) || (len <= 0) )
		return 0.0;

	/* skip white spc */
	p = fb_wstr_SkipChar( src, len, 32 );

	len -= fb_wstr_CalcDiff( src, p );
	if( len < 1 )
		return 0.0;

	r = p;
	if( (len >= 2) && (*r++ == L'&') )
	{
		switch( *r++ )
		{
			case L'h':
			case L'H':
				radix = 16;
				break;
			case L'o':
			case L'O':
				radix = 8;
				break;
			case L'b':
			case L'B':
				radix = 2;
				break;

			default: /* assume octal */
				radix = 8;
				r--;
				break;
		}

		radix_value = fb_WstrRadix2Longint( r, len - fb_wstr_CalcDiff( p, r ), radix );
		return (double)radix_value;
	}

	/* Workaround: wcstod() does not allow 'd' as an exponent specifier on 
	 * non-win32 platforms, so create a temporary buffer and replace any 
	 * 'd's with 'e'
	 */
	alloc_chars = (size_t)len;
	/* Include the terminator while keeping the allocation within pointer range. */
	if( alloc_chars >= (size_t)PTRDIFF_MAX / sizeof(FB_WCHAR) )
		return 0.0;
	++alloc_chars;
	q = malloc( alloc_chars * sizeof(FB_WCHAR) );
	if( q == NULL )
		return 0.0;
	for( i = 0; i < len; i++ )
	{
		c = p[i];
		if( c == L'd' || c == L'D' )
			++c;
		q[i]= c;
	}
	q[len] = L'\0';
	ret = wcstod( q, NULL );
	free( q );

	return ret;
}

FBCALL double fb_WstrVal ( const FB_WCHAR *str )
{
    double val;
	ssize_t len;

	if( str == NULL )
	    return 0.0;

	len = fb_wstr_Len( str );
	if( len == 0 )
		val = 0.0;
	else
		val = fb_WstrToDouble( str, len );

	return val;
}

/* end of strings/wide/strw_convfrom.c */
