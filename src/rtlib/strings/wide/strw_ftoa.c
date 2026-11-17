/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_ftoa.c
 * Purpose: float to wstring, internal usage.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <limits.h>

FB_WCHAR *fb_FloatExToWstr( double val, FB_WCHAR *buffer, int digits, int mask )
{
	FB_WCHAR *p;
	ssize_t len;
	size_t maxlen;
	int result;

	if( buffer == NULL || digits < 0 || digits > INT_MAX - 8 ) {
		if( buffer != NULL )
			buffer[0] = FB_WCHAR_LITERAL('\0');
		return NULL;
	}
	maxlen = (size_t)digits + 8u;

	if( mask & FB_F2A_ADDBLANK )
		p = &buffer[1];
	else
		p = buffer;

#if defined(DISABLE_WCHAR)
	/* GCC cannot correlate the capacity with %.*g's requested precision. */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
	result = snprintf( p, maxlen, "%.*g", digits, val );
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
#else
	result = swprintf( p, maxlen, FB_WCHAR_LITERAL("%.*g"), digits, val );
#endif
	if( result < 0 ) {
		buffer[0] = FB_WCHAR_LITERAL('\0');
		return NULL;
	}

	len = fb_wstr_Len( p );

	if( len > 0 )
	{
		/* skip the dot at end if any */
		if( p[len-1] == FB_WCHAR_LITERAL('.') )
			p[len-1] = FB_WCHAR_LITERAL('\0');
	}

	/* */
	if( (mask & FB_F2A_ADDBLANK) > 0 )
	{
		if( *p != FB_WCHAR_LITERAL('-') )
		{
			*buffer = FB_WCHAR_LITERAL(' ');
			return buffer;
		}
		else
			return p;
	}
	else
		return p;

}

/* end of strings/wide/strw_ftoa.c */
