/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_convto_flt.c
 * Purpose: strw$ routines for float and double.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FB_WCHAR *fb_FloatToWstr ( float num )
{
	FB_WCHAR *dst, *d;
	ssize_t len;

	/* alloc temp string */
    dst = fb_wstr_AllocTemp( 7+8 );
	if( dst != NULL )
    {
		/* convert */
        FB_WSTR_FROM_FLOAT( dst, num );

		/* skip the dot at end if any */
		len = fb_wstr_Len( dst );
		if( len > 0 )
		{
			d = &dst[len-1];
			if( *d == _LC('.') )
				*d = _LC('\0');
        }
    }

	return dst;
}

FBCALL FB_WCHAR *fb_DoubleToWstr ( double num )
{
	FB_WCHAR *dst, *d;
	ssize_t len;

	/* alloc temp string */
    dst = fb_wstr_AllocTemp( 16+8 );
	if( dst != NULL )
	{
        /* convert */
        FB_WSTR_FROM_DOUBLE( dst, num );

		/* skip the dot at end if any */
		len = fb_wstr_Len( dst );
		if( len > 0 )
		{
			d = &dst[len-1];
			if( *d == _LC('.') )
				*d = _LC('\0');
        }
	}

	return dst;
}

/* end of strings/wide/strw_convto_flt.c */
