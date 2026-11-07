/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_ftoa.c
 * Purpose: float to wstring, internal usage.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FB_WCHAR *fb_FloatExToWstr( double val, FB_WCHAR *buffer, int digits, int mask )
{
	FB_WCHAR *p;
	ssize_t len;

	if( mask & FB_F2A_ADDBLANK )
		p = &buffer[1];
	else
		p = buffer;

	swprintf( p, 16+8+1, FB_WCHAR_LITERAL("%.*g"), digits, val );

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
